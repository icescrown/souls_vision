//
// Created by PC-SAMUEL on 22/11/2024.
//

#ifndef SOULS_VISION_HOOK_HELPER_H
#define SOULS_VISION_HOOK_HELPER_H

#include <windows.h>
#include <vector>
#include <memory>
#include <functional>

namespace souls_vision {

class HookHelper {
public:
    HookHelper() = default;
    ~HookHelper() = default;

    static void Hook();
    static void Unhook();

private:
    static void InitHooks();
};

} // namespace souls_vision

#endif //SOULS_VISION_HOOK_HELPER_H
