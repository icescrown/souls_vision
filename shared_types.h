//
// Created by PC-SAMUEL on 24/11/2024.
//

#ifndef SOULS_VISION_SHARED_TYPES_H
#define SOULS_VISION_SHARED_TYPES_H

#include <string>

namespace souls_vision {

enum class BarType {
    HP,
    FP,
    Stamina,
    Stagger,
    Poison,
    ScarletRot,
    Hemorrhage,
    DeathBlight,
    Frostbite,
    Sleep,
    Madness
};

struct TextureInfo {
    void* textureResource = nullptr;
    int index = -1;
    int width = 0;
    int height = 0;
};

struct BarSettings {
    struct Vec2 {
        float x = 0;
        float y = 0;
    };
    Vec2 position;
    Vec2 size;
    float currentValue = 0;
    float maxValue = 1;
    bool hideText;
    std::string textureName;
};

struct ComponentConfig {
    bool visible;
    bool hideText;
};

struct Components {
    ComponentConfig hp = {true, false};
    ComponentConfig fp = {true, false};
    ComponentConfig stamina = {true, false};
    ComponentConfig stagger = {true, false};
    ComponentConfig poison = {true, false};
    ComponentConfig scarletRot = {true, false};
    ComponentConfig hemorrhage = {true, false};
    ComponentConfig deathBlight = {true, false};
    ComponentConfig frostbite = {true, false};
    ComponentConfig sleep = {true, false};
    ComponentConfig madness = {true, false};
    bool bestEffects = true;
    bool immuneEffects = true;
    bool dmgTypes = true;
    bool neutralDmgTypes = false;
};

struct BarConfig {
    BarType type;
    float currentValue;
    float maxValue;
    const char* textureName;
    unsigned int barColor = 0xFFFFFFFF;
    int decimals = 0;
    bool isEffect = false;
    bool condition = true;
};

struct BarToRender {
    BarSettings settings;
    TextureInfo textureInfo;
    BarConfig config;
    unsigned int barColor;
    int decimals = 0;
};

struct Size {
    int width;
    int height;
};

} // namespace souls_vision

#endif // SOULS_VISION_SHARED_TYPES_H
