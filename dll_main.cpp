#define NOMINMAX
#include "globals.h"
#include "game_handler.h"
#include "hook_helper.h"
#include "logger.h"
#include "config.h"
#include "shared_memory.h"
#include "data_collector.h"

#include <thread>
#include <iostream>
#include <chrono>
#include <filesystem>

using namespace souls_vision;

DWORD WINAPI Setup(LPVOID lpParam);
void Cleanup();
void MainThread();
void DataCollectionThread();
std::string GetDllPath(HMODULE hModule);
std::string GetDllDirectory(HMODULE hModule);
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);
Size GetWindowSize(HWND hwnd = nullptr);
bool LaunchRendererProcess();

// 全局变量
SharedMemory gSharedMemory;
std::atomic<bool> gDataCollectionRunning(false);
HANDLE gRendererProcess = nullptr;

DWORD WINAPI Setup(LPVOID lpParam) {
    auto hModule = static_cast<HMODULE>(lpParam);
    gModule = hModule;

    gDllPath = GetDllDirectory(hModule);
    gConfigFilePath = gDllPath + "\\sv_config.toml";

    std::string logFilePath = gDllPath + "\\souls_vision.log";
    Logger::Initialize(logFilePath);
    Logger::Info("Starting SoulsVision (External Process Mode)...");

    while (!gGameWindow) {
        gGameWindow = FindWindowW(nullptr, gWindowClass);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    gGameWindowSize = GetWindowSize(gGameWindow);

    Config::LoadConfig(gConfigFilePath);
    Config::SaveConfig(gConfigFilePath);
    if (Config::debug) {
        Logger::InitializeDebug();
    }

    // 初始化共享内存
    if (!gSharedMemory.InitializeAsHost()) {
        Logger::Error("Failed to initialize shared memory");
        return 1;
    }
    Logger::Info("Shared memory initialized");

    // 启动外部渲染进程
    if (!LaunchRendererProcess()) {
        Logger::Error("Failed to launch renderer process");
        gSharedMemory.Shutdown();
        return 1;
    }
    Logger::Info("Renderer process launched");

    GameHandler::Initialize();
    while (!souls_vision::GameHandler::CSMenuManImp() || souls_vision::GameHandler::CSMenuManImp()->loadingScreenData.timer <= 0.0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(Config::delay));
    Logger::Info("Game initialized and loaded");

    HookHelper::Hook();

    // 启动数据收集线程
    gDataCollectionRunning = true;
    std::thread dataThread(DataCollectionThread);

    MainThread();

    // 停止数据收集
    gDataCollectionRunning = false;
    dataThread.join();

    Cleanup();
    FreeLibraryAndExitThread(hModule, 0);
}

void Cleanup() {
    Logger::Info("Shutting down SoulsVision...");
    
    gDataCollectionRunning = false;
    HookHelper::Unhook();
    
    // 关闭渲染进程
    if (gRendererProcess) {
        TerminateProcess(gRendererProcess, 0);
        CloseHandle(gRendererProcess);
        gRendererProcess = nullptr;
    }
    
    gSharedMemory.Shutdown();

    Logger::Info("SoulsVision shutdown complete");
    Logger::Shutdown();
}

void MainThread() {
    gRunning = true;

    auto lastWriteTime = std::filesystem::last_write_time(gConfigFilePath);

    Logger::Info("Main thread started");
    while (gRunning) {
        try {
            auto currentWriteTime = std::filesystem::last_write_time(gConfigFilePath);
            if (currentWriteTime != lastWriteTime) {
                lastWriteTime = currentWriteTime;

                Logger::Info("Config file updated. Reloading...");
                Config::LoadConfig(gConfigFilePath);
                Config::configUpdated = true;
            }
        } catch (const std::exception& e) {
            Logger::Error(std::string("Error checking config file: ") + e.what());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void DataCollectionThread() {
    Logger::Info("Data collection thread started");
    
    SharedData data = {};
    data.version = CURRENT_VERSION;
    
    // 初始化配置数据
    for (int i = 0; i < static_cast<int>(SharedBarType::COUNT); i++) {
        data.bars_visible[i] = true;
        data.bars_hide_text[i] = false;
    }
    
    while (gDataCollectionRunning && gRunning) {
        // 收集游戏数据
        DataCollector::Collect(data);
        
        // 更新配置数据
        data.bars_visible[static_cast<int>(SharedBarType::HP)] = Config::components.hp.visible;
        data.bars_visible[static_cast<int>(SharedBarType::FP)] = Config::components.fp.visible;
        data.bars_visible[static_cast<int>(SharedBarType::STAMINA)] = Config::components.stamina.visible;
        data.bars_visible[static_cast<int>(SharedBarType::STAGGER)] = Config::components.stagger.visible;
        data.bars_visible[static_cast<int>(SharedBarType::POISON)] = Config::components.poison.visible;
        data.bars_visible[static_cast<int>(SharedBarType::SCARLET_ROT)] = Config::components.scarletRot.visible;
        data.bars_visible[static_cast<int>(SharedBarType::HEMORRHAGE)] = Config::components.hemorrhage.visible;
        data.bars_visible[static_cast<int>(SharedBarType::DEATH_BLIGHT)] = Config::components.deathBlight.visible;
        data.bars_visible[static_cast<int>(SharedBarType::FROSTBITE)] = Config::components.frostbite.visible;
        data.bars_visible[static_cast<int>(SharedBarType::SLEEP)] = Config::components.sleep.visible;
        data.bars_visible[static_cast<int>(SharedBarType::MADNESS)] = Config::components.madness.visible;
        
        data.bars_hide_text[static_cast<int>(SharedBarType::HP)] = Config::components.hp.hideText;
        data.bars_hide_text[static_cast<int>(SharedBarType::FP)] = Config::components.fp.hideText;
        data.bars_hide_text[static_cast<int>(SharedBarType::STAMINA)] = Config::components.stamina.hideText;
        data.bars_hide_text[static_cast<int>(SharedBarType::STAGGER)] = Config::components.stagger.hideText;
        
        data.show_dmg_types = Config::components.dmgTypes;
        
        // 更新时间戳
        data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
        // 写入共享内存
        gSharedMemory.WriteData(data);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }
    
    Logger::Info("Data collection thread stopped");
}

bool LaunchRendererProcess() {
    std::wstring rendererPath = std::wstring(gDllPath.begin(), gDllPath.end());
    rendererPath += L"\\souls_vision_renderer.exe";
    
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    
    BOOL success = CreateProcessW(
        rendererPath.c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );
    
    if (success) {
        gRendererProcess = pi.hProcess;
        CloseHandle(pi.hThread);
        return true;
    }
    
    return false;
}

std::string GetDllPath(HMODULE hModule) {
    char path[MAX_PATH];
    if (GetModuleFileNameA(hModule, path, MAX_PATH) == 0) {
        return "";
    }

    return {path};
}

std::string GetDllDirectory(HMODULE hModule) {
    std::string path = GetDllPath(hModule);
    size_t lastSlashIndex = path.find_last_of("\\/");
    if (lastSlashIndex == std::string::npos) {
        return "";
    }

    return path.substr(0, lastSlashIndex);
}

Size GetWindowSize(HWND hwnd) {
    if (!hwnd) {
        hwnd = FindWindowW(nullptr, gWindowClass);
    }

    RECT rect;
    if (GetClientRect(hwnd, &rect)) {
        auto width = rect.right - rect.left;
        auto height = rect.bottom - rect.top;
        Logger::Info("Window size: " + std::to_string(width) + "x" + std::to_string(height));
        return {width, height};
    } else {
        Logger::Error("Failed to get window size");
        return {0, 0};
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, Setup, hModule, 0, nullptr);
    } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        gRunning = false;
    }

    return TRUE;
}