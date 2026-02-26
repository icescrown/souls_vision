//
// SoulsVision 外部渲染进程 - 使用Direct2D渲染精美UI
//

#define NOMINMAX
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <cmath>

#include "../shared_memory.h"
#include "../config.h"
#include "texture_loader.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace souls_vision;

// 窗口和渲染相关
HWND gHwnd = nullptr;
ID2D1Factory* gD2DFactory = nullptr;
ID2D1HwndRenderTarget* gRenderTarget = nullptr;
IDWriteFactory* gDWriteFactory = nullptr;
IDWriteTextFormat* gTextFormat = nullptr;
IDWriteTextFormat* gTextFormatSmall = nullptr;

// 纹理加载器
TextureLoader gTextureLoader;

// 状态
std::atomic<bool> gRunning(true);
SharedMemory gSharedMemory;
SharedData gCurrentData;
bool gDataAvailable = false;

// 颜色定义（用于文字）
const D2D1_COLOR_F COLOR_WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
const D2D1_COLOR_F COLOR_BLACK = { 0.0f, 0.0f, 0.0f, 1.0f };

// 效果条颜色（RGBA）
const D2D1_COLOR_F COLOR_POISON = { 0.39f, 0.44f, 0.0f, 1.0f };        // #647000
const D2D1_COLOR_F COLOR_SCARLET_ROT = { 0.46f, 0.15f, 0.0f, 1.0f };   // #752601
const D2D1_COLOR_F COLOR_HEMORRHAGE = { 0.37f, 0.04f, 0.04f, 1.0f };   // #5F0B0B
const D2D1_COLOR_F COLOR_DEATH_BLIGHT = { 0.1f, 0.1f, 0.1f, 1.0f };     // #1A1A1A
const D2D1_COLOR_F COLOR_FROSTBITE = { 0.18f, 0.42f, 0.55f, 1.0f };     // #2E6B8C
const D2D1_COLOR_F COLOR_SLEEP = { 0.31f, 0.18f, 0.49f, 1.0f };         // #4F2E7D
const D2D1_COLOR_F COLOR_MADNESS = { 0.56f, 0.31f, 0.04f, 1.0f };       // #8F4F0A

// 函数声明
bool InitializeWindow(HINSTANCE hInstance);
bool InitializeDirect2D();
void ShutdownDirect2D();
void LoadResources();
void UnloadResources();
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RenderThread();
void UpdateWindowPosition();
void DrawStatBar(ID2D1RenderTarget* rt, float x, float y, float width, float height,
                 float current, float max, const std::string& barTexture, const wchar_t* label, bool hideText);
void DrawEffectBar(ID2D1RenderTarget* rt, float x, float y, float width, float height,
                   const EffectInfo& effect, const std::string& iconTexture, D2D1_COLOR_F barColor);
D2D1_COLOR_F GetEffectColor(const std::wstring& effectName);
const wchar_t* GetEffectLabel(const std::wstring& effectName);

int main() {
    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    int nCmdShow = SW_SHOWDEFAULT;

    // 初始化COM（用于WIC）
    CoInitialize(nullptr);

    // 加载配置文件
    std::string configPath = "souls_vision_config.toml";
    Config::LoadConfig(configPath);

    // 初始化共享内存
    if (!gSharedMemory.InitializeAsClient()) {
        MessageBoxW(nullptr, L"无法连接到游戏进程，请确保游戏正在运行。", L"SoulsVision Renderer", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }
    
    // 初始化窗口
    if (!InitializeWindow(hInstance)) {
        MessageBoxW(nullptr, L"创建窗口失败", L"SoulsVision Renderer", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }
    
    // 初始化Direct2D
    if (!InitializeDirect2D()) {
        MessageBoxW(nullptr, L"初始化Direct2D失败", L"SoulsVision Renderer", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }
    
    // 加载资源
    LoadResources();
    
    // 启动渲染线程
    std::thread renderThread(RenderThread);
    
    // 消息循环
    MSG msg;
    while (gRunning) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                gRunning = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // 等待渲染线程结束
    renderThread.join();
    
    // 清理
    UnloadResources();
    ShutdownDirect2D();
    CoUninitialize();
    
    return 0;
}

bool InitializeWindow(HINSTANCE hInstance) {
    // 注册窗口类
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"SoulsVisionRenderer";
    
    if (!RegisterClassExW(&wc)) {
        return false;
    }
    
    // 创建透明窗口
    gHwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"SoulsVisionRenderer",
        L"SoulsVision Renderer",
        WS_POPUP,
        0, 0, 1920, 1080,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    
    if (!gHwnd) {
        return false;
    }
    
    // 设置窗口透明度和点击穿透
    SetLayeredWindowAttributes(gHwnd, RGB(0, 0, 0), 255, LWA_COLORKEY);
    
    ShowWindow(gHwnd, SW_SHOWDEFAULT);
    UpdateWindow(gHwnd);
    
    return true;
}

bool InitializeDirect2D() {
    // 创建D2D工厂
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &gD2DFactory);
    if (FAILED(hr)) {
        return false;
    }
    
    // 获取窗口大小
    RECT rc;
    GetClientRect(gHwnd, &rc);
    
    // 创建渲染目标
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    hr = gD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        ),
        D2D1::HwndRenderTargetProperties(gHwnd, size),
        &gRenderTarget
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    // 设置透明背景
    gRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    
    // 创建DWrite工厂
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), 
                             reinterpret_cast<IUnknown**>(&gDWriteFactory));
    if (FAILED(hr)) {
        return false;
    }
    
    // 创建文本格式（使用配置中的字体大小）
    float fontSize = Config::fontSize > 0 ? Config::fontSize : 18.0f;
    hr = gDWriteFactory->CreateTextFormat(
        L"Microsoft YaHei",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"zh-CN",
        &gTextFormat
    );

    if (FAILED(hr)) {
        return false;
    }

    gTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    gTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    hr = gDWriteFactory->CreateTextFormat(
        L"Microsoft YaHei",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize * 0.85f,  // 小字体为正常字体的85%
        L"zh-CN",
        &gTextFormatSmall
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    gTextFormatSmall->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    gTextFormatSmall->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    
    return true;
}

void ShutdownDirect2D() {
    if (gTextFormatSmall) {
        gTextFormatSmall->Release();
        gTextFormatSmall = nullptr;
    }
    
    if (gTextFormat) {
        gTextFormat->Release();
        gTextFormat = nullptr;
    }
    
    if (gDWriteFactory) {
        gDWriteFactory->Release();
        gDWriteFactory = nullptr;
    }
    
    if (gRenderTarget) {
        gRenderTarget->Release();
        gRenderTarget = nullptr;
    }
    
    if (gD2DFactory) {
        gD2DFactory->Release();
        gD2DFactory = nullptr;
    }
}

void LoadResources() {
    gTextureLoader.Initialize(gRenderTarget);
    gTextureLoader.LoadAllResources();
}

void UnloadResources() {
    gTextureLoader.ReleaseAll();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_SIZE:
            if (gRenderTarget) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
                gRenderTarget->Resize(size);
            }
            return 0;
            
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void RenderThread() {
    while (gRunning) {
        // 等待数据就绪
        if (gSharedMemory.WaitForData(100)) {
            SharedData newData;
            if (gSharedMemory.ReadData(newData)) {
                memcpy(&gCurrentData, &newData, sizeof(SharedData));
                gDataAvailable = true;
                
                // 更新窗口位置
                UpdateWindowPosition();
                
                // 通知数据已消费
                gSharedMemory.NotifyDataConsumed();
            }
        }
        
        // 检查游戏进程是否存活
        if (gDataAvailable && !gCurrentData.game_process_alive) {
            gRunning = false;
            PostMessageW(gHwnd, WM_CLOSE, 0, 0);
            break;
        }
        
        // 渲染 - 只有当有目标NPC且数据有效时才显示
        if (gRenderTarget && gDataAvailable && gCurrentData.window_active && gCurrentData.data_valid) {
            gRenderTarget->BeginDraw();
            gRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));  // 透明背景

            // 获取窗口大小
            D2D1_SIZE_F size = gRenderTarget->GetSize();
            float windowWidth = size.width;
            float windowHeight = size.height;

            // 使用配置文件中的尺寸设置
            float barWidth = Config::statBarSettings.size.x;
            float barHeight = Config::statBarSettings.size.y;
            float effectBarHeight = Config::effectBarIconSize;
            float dmgTypeIconSize = Config::dmgTypeIconSize;
            float spacing = Config::statBarSpacing;

            // 计算状态条位置（使用配置文件中的位置）
            float barX = Config::statBarSettings.position.x;
            float barY = Config::statBarSettings.position.y;
            
            // 绘制HP条
            if (gCurrentData.bars_visible[static_cast<int>(SharedBarType::HP)]) {
                DrawStatBar(gRenderTarget, barX, barY, barWidth, barHeight,
                           gCurrentData.hp_current, gCurrentData.hp_max,
                           "red", L"HP", gCurrentData.bars_hide_text[static_cast<int>(SharedBarType::HP)]);
                barY += barHeight + spacing;
            }

            // 绘制FP条
            if (gCurrentData.bars_visible[static_cast<int>(SharedBarType::FP)]) {
                DrawStatBar(gRenderTarget, barX, barY, barWidth, barHeight,
                           gCurrentData.fp_current, gCurrentData.fp_max,
                           "blue", L"FP", gCurrentData.bars_hide_text[static_cast<int>(SharedBarType::FP)]);
                barY += barHeight + spacing;
            }

            // 绘制耐力条
            if (gCurrentData.bars_visible[static_cast<int>(SharedBarType::STAMINA)]) {
                DrawStatBar(gRenderTarget, barX, barY, barWidth, barHeight,
                           gCurrentData.stamina_current, gCurrentData.stamina_max,
                           "green", L"耐力", gCurrentData.bars_hide_text[static_cast<int>(SharedBarType::STAMINA)]);
                barY += barHeight + spacing;
            }

            // 绘制失衡条
            if (gCurrentData.bars_visible[static_cast<int>(SharedBarType::STAGGER)]) {
                DrawStatBar(gRenderTarget, barX, barY, barWidth, barHeight,
                           gCurrentData.stagger_current, gCurrentData.stagger_max,
                           "yellow", L"失衡", gCurrentData.bars_hide_text[static_cast<int>(SharedBarType::STAGGER)]);
                barY += barHeight + spacing + 10;
            }
            
            // 绘制效果条
            float effectY = barY;
            for (int i = 0; i < gCurrentData.effect_count && i < MAX_EFFECTS; i++) {
                const EffectInfo& effect = gCurrentData.effects[i];
                if (effect.active) {
                    std::wstring name(effect.name);
                    D2D1_COLOR_F color = GetEffectColor(name);
                    std::string iconName;
                    
                    if (name.find(L"Poison") != std::wstring::npos) iconName = "poison";
                    else if (name.find(L"ScarletRot") != std::wstring::npos) iconName = "scarlet_rot";
                    else if (name.find(L"Hemorrhage") != std::wstring::npos) iconName = "hemorrhage";
                    else if (name.find(L"DeathBlight") != std::wstring::npos) iconName = "death_blight";
                    else if (name.find(L"Frostbite") != std::wstring::npos) iconName = "frostbite";
                    else if (name.find(L"Sleep") != std::wstring::npos) iconName = "sleep";
                    else if (name.find(L"Madness") != std::wstring::npos) iconName = "madness";
                    else continue;
                    
                    DrawEffectBar(gRenderTarget, barX, effectY, barWidth, effectBarHeight,
                                 effect, iconName, color);
                    effectY += effectBarHeight + spacing;
                }
            }
            
            // 绘制伤害类型抗性图标（弱点）
            if (gCurrentData.show_dmg_types && gCurrentData.dmg_type_count > 0) {
                float iconSize = dmgTypeIconSize;
                float iconX = barX - iconSize * 1.5f;  // 在状态条左侧
                float iconY = Config::statBarSettings.position.y;  // 从HP条位置开始
                
                for (int i = 0; i < gCurrentData.dmg_type_count && i < MAX_DMG_TYPES; i++) {
                    const DamageTypeInfo& dmgType = gCurrentData.dmg_types[i];
                    if (!dmgType.isWeakness && !dmgType.isImmune) continue;  // 只显示弱点和免疫
                    
                    std::wstring name(dmgType.name);
                    std::string iconName;
                    if (name.find(L"Fire") != std::wstring::npos) iconName = "fire";
                    else if (name.find(L"Magic") != std::wstring::npos) iconName = "magic";
                    else if (name.find(L"Lightning") != std::wstring::npos) iconName = "lightning";
                    else if (name.find(L"Holy") != std::wstring::npos) iconName = "holy";
                    else continue;
                    
                    D2DTextureInfo* iconTex = gTextureLoader.GetTexture(iconName);
                    if (iconTex && iconTex->bitmap) {
                        D2D1_RECT_F iconRect = D2D1::RectF(iconX, iconY, iconX + iconSize, iconY + iconSize);
                        gRenderTarget->DrawBitmap(iconTex->bitmap, iconRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                                       D2D1::RectF(0, 0, (float)iconTex->width, (float)iconTex->height));

                        // 如果是弱点，绘制绿色箭头
                        if (dmgType.isWeakness) {
                            D2DTextureInfo* arrowTex = gTextureLoader.GetTexture("green_arrow");
                            if (arrowTex && arrowTex->bitmap) {
                                float arrowSize = iconSize * 0.6f;
                                D2D1_RECT_F arrowRect = D2D1::RectF(iconX + iconSize * 0.2f, iconY + iconSize,
                                                                    iconX + iconSize * 0.2f + arrowSize, iconY + iconSize + arrowSize);
                                gRenderTarget->DrawBitmap(arrowTex->bitmap, arrowRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                                               D2D1::RectF(0, 0, (float)arrowTex->width, (float)arrowTex->height));
                            }
                        }

                        iconY += iconSize + 5;
                    }
                }
            }
            
            gRenderTarget->EndDraw();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }
}

void UpdateWindowPosition() {
    if (!gCurrentData.game_window || !IsWindow(gCurrentData.game_window)) {
        return;
    }
    
    RECT gameRect;
    if (GetWindowRect(gCurrentData.game_window, &gameRect)) {
        // 检查窗口是否最小化
        if (IsIconic(gCurrentData.game_window)) {
            ShowWindow(gHwnd, SW_HIDE);
            return;
        }
        
        // 获取客户端区域
        RECT clientRect;
        GetClientRect(gCurrentData.game_window, &clientRect);
        
        // 将客户端坐标转换为屏幕坐标
        POINT clientTopLeft = { 0, 0 };
        ClientToScreen(gCurrentData.game_window, &clientTopLeft);
        
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;
        
        // 设置覆盖窗口位置和大小
        SetWindowPos(gHwnd, HWND_TOPMOST, 
                     clientTopLeft.x, clientTopLeft.y, 
                     width, height, 
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

void DrawStatBar(ID2D1RenderTarget* rt, float x, float y, float width, float height,
                 float current, float max, const std::string& barTexture, const wchar_t* label, bool hideText) {
    if (max <= 0) max = 1;
    float ratio = std::min(1.0f, std::max(0.0f, current / max));
    
    // 获取纹理
    D2DTextureInfo* bgTex = gTextureLoader.GetTexture("bar_bg");
    D2DTextureInfo* barTex = gTextureLoader.GetTexture(barTexture);
    D2DTextureInfo* edgeTex = gTextureLoader.GetTexture("bar_edge2");
    D2DTextureInfo* frameTex = gTextureLoader.GetTexture("buddy_waku");
    
    if (!bgTex || !barTex || !edgeTex || !frameTex) {
        // 如果纹理未加载，使用纯色绘制
        ID2D1SolidColorBrush* brush = nullptr;
        rt->CreateSolidColorBrush(D2D1::ColorF(0.5f, 0.5f, 0.5f, 0.8f), &brush);
        if (brush) {
            rt->FillRectangle(D2D1::RectF(x, y, x + width, y + height), brush);
            brush->Release();
        }
        return;
    }
    
    // 计算内部条位置和大小（考虑边框）
    float paddingX = width * 0.029f;
    float paddingY = height * 0.125f;
    float barX = x + paddingX;
    float barY = y + paddingY;
    float barW = width * 0.938f;
    float barH = height * 0.725f;
    
    // 绘制背景（黑色遮罩）
    D2D1_RECT_F bgRect = D2D1::RectF(barX, barY, barX + barW, barY + barH);
    rt->DrawBitmap(bgTex->bitmap, bgRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                   D2D1::RectF(0, 0, (float)bgTex->width, (float)bgTex->height));
    
    // 绘制填充条（根据百分比裁剪）
    D2D1_RECT_F fillRect = D2D1::RectF(barX, barY, barX + barW * ratio, barY + barH);
    D2D1_RECT_F srcRect = D2D1::RectF(0, 0, (float)barTex->width * ratio, (float)barTex->height);
    rt->DrawBitmap(barTex->bitmap, fillRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
    
    // 绘制边缘光效
    if (ratio > 0.01f && ratio < 0.99f) {
        float edgeW = width * 0.2f;
        float edgeH = height * 0.75f;
        float edgeX = barX + barW * ratio - edgeW * 0.92f;
        float edgeY = barY - edgeH * 0.1f;
        D2D1_RECT_F edgeRect = D2D1::RectF(edgeX, edgeY, edgeX + edgeW, edgeY + edgeH);
        rt->DrawBitmap(edgeTex->bitmap, edgeRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                       D2D1::RectF(0, 0, (float)edgeTex->width, (float)edgeTex->height));
    }
    
    // 绘制外框
    D2D1_RECT_F frameRect = D2D1::RectF(x, y, x + width, y + height);
    rt->DrawBitmap(frameTex->bitmap, frameRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                   D2D1::RectF(0, 0, (float)frameTex->width, (float)frameTex->height));
    
    // 绘制文字
    if (!hideText && gTextFormat) {
        wchar_t text[64];
        swprintf_s(text, L"%.0f / %.0f", current, max);
        
        ID2D1SolidColorBrush* textBrush = nullptr;
        rt->CreateSolidColorBrush(COLOR_WHITE, &textBrush);
        if (textBrush) {
            D2D1_RECT_F textRect = D2D1::RectF(barX, barY, barX + barW, barY + barH);
            rt->DrawText(text, static_cast<UINT32>(wcslen(text)), gTextFormat,
                        &textRect, textBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
            textBrush->Release();
        }
    }
}

void DrawEffectBar(ID2D1RenderTarget* rt, float x, float y, float width, float height,
                   const EffectInfo& effect, const std::string& iconTexture, D2D1_COLOR_F barColor) {
    if (effect.max_value <= 0) return;
    float ratio = std::min(1.0f, std::max(0.0f, effect.current_value / effect.max_value));
    
    // 获取纹理
    D2DTextureInfo* bgTex = gTextureLoader.GetTexture("bar_bg");
    D2DTextureInfo* barTex = gTextureLoader.GetTexture("bar");
    D2DTextureInfo* edgeTex = gTextureLoader.GetTexture("bar_edge2");
    D2DTextureInfo* frameTex = gTextureLoader.GetTexture("condition_waku");
    D2DTextureInfo* iconTex = gTextureLoader.GetTexture(iconTexture);
    
    if (!bgTex || !barTex || !edgeTex || !frameTex) {
        return;
    }
    
    // 图标大小
    float iconSize = 32.0f;
    
    // 计算条位置和大小
    float barX = x + iconSize - iconSize * 0.14f;
    float barY = y + (iconSize / 2) - (height * 0.365f);
    float barW = (width - iconSize) * 0.95f;
    float barH = height * 0.73f;
    
    // 绘制背景
    D2D1_RECT_F bgRect = D2D1::RectF(barX, barY, barX + barW, barY + barH);
    rt->DrawBitmap(bgTex->bitmap, bgRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                   D2D1::RectF(0, 0, (float)bgTex->width, (float)bgTex->height));
    
    // 绘制填充条（带颜色着色）
    ID2D1SolidColorBrush* colorBrush = nullptr;
    rt->CreateSolidColorBrush(barColor, &colorBrush);
    if (colorBrush) {
        D2D1_RECT_F fillRect = D2D1::RectF(barX, barY, barX + barW * ratio, barY + barH);
        D2D1_RECT_F srcRect = D2D1::RectF(0, 0, (float)barTex->width * ratio, (float)barTex->height);
        // 使用颜色刷子作为不透明度蒙版
        rt->DrawBitmap(barTex->bitmap, fillRect, barColor.a, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
        colorBrush->Release();
    }
    
    // 绘制边缘光效
    if (ratio > 0.01f && ratio < 0.99f) {
        float edgeW = width * 0.15f;
        float edgeH = height * 0.75f;
        float edgeX = barX + barW * ratio - edgeW * 0.92f;
        float edgeY = barY - edgeH * 0.1f;
        D2D1_RECT_F edgeRect = D2D1::RectF(edgeX, edgeY, edgeX + edgeW, edgeY + edgeH);
        rt->DrawBitmap(edgeTex->bitmap, edgeRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                       D2D1::RectF(0, 0, (float)edgeTex->width, (float)edgeTex->height));
    }
    
    // 绘制外框
    float frameX = barX;
    float frameY = y + (iconSize / 2) - (height / 2);
    float frameW = width - iconSize;
    float frameH = height;
    D2D1_RECT_F frameRect = D2D1::RectF(frameX, frameY, frameX + frameW, frameY + frameH);
    rt->DrawBitmap(frameTex->bitmap, frameRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                   D2D1::RectF(0, 0, (float)frameTex->width, (float)frameTex->height));
    
    // 绘制图标
    if (iconTex) {
        D2D1_RECT_F iconRect = D2D1::RectF(x, y, x + iconSize, y + iconSize);
        rt->DrawBitmap(iconTex->bitmap, iconRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                       D2D1::RectF(0, 0, (float)iconTex->width, (float)iconTex->height));
    }
    
    // 绘制文字
    if (gTextFormatSmall) {
        wchar_t text[128];
        swprintf_s(text, L"%.0f / %.0f", effect.current_value, effect.max_value);
        
        ID2D1SolidColorBrush* textBrush = nullptr;
        rt->CreateSolidColorBrush(COLOR_WHITE, &textBrush);
        if (textBrush) {
            D2D1_RECT_F textRect = D2D1::RectF(barX, barY, barX + barW, barY + barH);
            rt->DrawText(text, static_cast<UINT32>(wcslen(text)), gTextFormatSmall,
                        &textRect, textBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
            textBrush->Release();
        }
    }
}

D2D1_COLOR_F GetEffectColor(const std::wstring& effectName) {
    if (effectName.find(L"Poison") != std::wstring::npos) return COLOR_POISON;
    if (effectName.find(L"ScarletRot") != std::wstring::npos) return COLOR_SCARLET_ROT;
    if (effectName.find(L"Hemorrhage") != std::wstring::npos) return COLOR_HEMORRHAGE;
    if (effectName.find(L"DeathBlight") != std::wstring::npos) return COLOR_DEATH_BLIGHT;
    if (effectName.find(L"Frostbite") != std::wstring::npos) return COLOR_FROSTBITE;
    if (effectName.find(L"Sleep") != std::wstring::npos) return COLOR_SLEEP;
    if (effectName.find(L"Madness") != std::wstring::npos) return COLOR_MADNESS;
    return COLOR_WHITE;
}

const wchar_t* GetEffectLabel(const std::wstring& effectName) {
    if (effectName.find(L"Poison") != std::wstring::npos) return L"中毒";
    if (effectName.find(L"ScarletRot") != std::wstring::npos) return L"猩红腐败";
    if (effectName.find(L"Hemorrhage") != std::wstring::npos) return L"出血";
    if (effectName.find(L"DeathBlight") != std::wstring::npos) return L"死亡凋零";
    if (effectName.find(L"Frostbite") != std::wstring::npos) return L"冻伤";
    if (effectName.find(L"Sleep") != std::wstring::npos) return L"睡眠";
    if (effectName.find(L"Madness") != std::wstring::npos) return L"发狂";
    return L"效果";
}
