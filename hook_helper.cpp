//
// Created by PC-SAMUEL on 22/11/2024.
//
#define NOMINMAX
#include "hook_helper.h"
#include "logger.h"

#include <MinHook.h>

namespace souls_vision {

void HookHelper::Hook() {
    MH_Initialize();
    Logger::Info("Hook system initialized (external process mode - no D3D12 hooks needed).");
}

void HookHelper::Unhook() {
    MH_Uninitialize();
    Logger::Info("Hook system uninitialized.");
}

void HookHelper::InitHooks() {
    // 外部进程方案不需要D3D12渲染hook
    // 所有渲染都在独立的进程中完成
}

} // namespace souls_vision
