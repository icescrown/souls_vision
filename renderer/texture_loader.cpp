//
// Direct2D 纹理加载器实现
//

#include "texture_loader.h"
#include <wincodec.h>
#include <shlwapi.h>
#include <iostream>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")

namespace souls_vision {

TextureLoader::TextureLoader() : renderTarget_(nullptr) {
    // 获取可执行文件所在目录
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    resourcePath_ = std::wstring(path) + L"\\sv_assets\\";
}

TextureLoader::~TextureLoader() {
    ReleaseAll();
}

bool TextureLoader::Initialize(ID2D1RenderTarget* renderTarget) {
    renderTarget_ = renderTarget;
    return renderTarget_ != nullptr;
}

bool TextureLoader::LoadTexture(const std::string& filename, const std::string& name) {
    if (!renderTarget_) {
        return false;
    }
    
    // 转换为宽字符路径
    std::wstring wfilename(resourcePath_);
    int len = MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, nullptr, 0);
    std::vector<wchar_t> wfile(len);
    MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, wfile.data(), len);
    wfilename += wfile.data();
    
    ID2D1Bitmap* bitmap = LoadBitmapFromFile(wfilename.c_str());
    if (!bitmap) {
        std::wcerr << L"Failed to load texture: " << wfilename << std::endl;
        return false;
    }
    
    D2D1_SIZE_F size = bitmap->GetSize();
    D2DTextureInfo info;
    info.bitmap = bitmap;
    info.width = static_cast<int>(size.width);
    info.height = static_cast<int>(size.height);
    info.name = name;
    
    textures_[name] = info;
    return true;
}

D2DTextureInfo* TextureLoader::GetTexture(const std::string& name) {
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool TextureLoader::LoadAllResources() {
    bool success = true;
    
    // 状态条资源
    success &= LoadTexture("BarBG.png", "bar_bg");
    success &= LoadTexture("Bar.png", "bar");
    success &= LoadTexture("BarEdge.png", "bar_edge");
    success &= LoadTexture("BarEdge2.png", "bar_edge2");
    success &= LoadTexture("BuddyWaku.png", "buddy_waku");
    success &= LoadTexture("ConditionWaku.png", "condition_waku");
    
    // 状态条颜色
    success &= LoadTexture("Red.png", "red");
    success &= LoadTexture("Blue.png", "blue");
    success &= LoadTexture("Green.png", "green");
    success &= LoadTexture("Yellow.png", "yellow");
    
    // 效果图标
    success &= LoadTexture("Poison.png", "poison");
    success &= LoadTexture("ScarletRot.png", "scarlet_rot");
    success &= LoadTexture("Hemorrhage.png", "hemorrhage");
    success &= LoadTexture("DeathBlight.png", "death_blight");
    success &= LoadTexture("Frostbite.png", "frostbite");
    success &= LoadTexture("Sleep.png", "sleep");
    success &= LoadTexture("Madness.png", "madness");
    
    // 伤害类型
    success &= LoadTexture("Magic.png", "magic");
    success &= LoadTexture("Fire.png", "fire");
    success &= LoadTexture("Lightning.png", "lightning");
    success &= LoadTexture("Holy.png", "holy");
    
    // 箭头
    success &= LoadTexture("RedArrow.png", "red_arrow");
    success &= LoadTexture("GreenArrow.png", "green_arrow");
    
    return success;
}

void TextureLoader::ReleaseAll() {
    for (auto& pair : textures_) {
        if (pair.second.bitmap) {
            pair.second.bitmap->Release();
            pair.second.bitmap = nullptr;
        }
    }
    textures_.clear();
}

ID2D1Bitmap* TextureLoader::LoadBitmapFromFile(const wchar_t* filename) {
    IWICImagingFactory* wicFactory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    ID2D1Bitmap* bitmap = nullptr;
    
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory,
        reinterpret_cast<LPVOID*>(&wicFactory)
    );
    
    if (FAILED(hr)) {
        return nullptr;
    }
    
    hr = wicFactory->CreateDecoderFromFilename(
        filename,
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder
    );
    
    if (FAILED(hr)) {
        wicFactory->Release();
        return nullptr;
    }
    
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        decoder->Release();
        wicFactory->Release();
        return nullptr;
    }
    
    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) {
        frame->Release();
        decoder->Release();
        wicFactory->Release();
        return nullptr;
    }
    
    hr = converter->Initialize(
        frame,
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0f,
        WICBitmapPaletteTypeMedianCut
    );
    
    if (FAILED(hr)) {
        converter->Release();
        frame->Release();
        decoder->Release();
        wicFactory->Release();
        return nullptr;
    }
    
    hr = renderTarget_->CreateBitmapFromWicBitmap(
        converter,
        nullptr,
        &bitmap
    );
    
    converter->Release();
    frame->Release();
    decoder->Release();
    wicFactory->Release();
    
    return bitmap;
}

} // namespace souls_vision
