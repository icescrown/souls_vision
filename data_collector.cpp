//
// 数据收集模块实现 - 收集选中NPC的信息
//

#include "data_collector.h"
#include "game_handler.h"
#include "globals.h"
#include "logger.h"
#include "structs/chr_ins.h"
#include "structs/chr_module_bag.h"
#include "structs/chr_stat_module.h"
#include "structs/chr_resist_module.h"
#include "structs/chr_super_armor_module.h"
#include "structs/world_chr_man_imp.h"
#include "structs/npc_param.h"

#include <vector>
#include <algorithm>

namespace souls_vision {

void DataCollector::Collect(SharedData& data) {
    CollectWindowInfo(data);
    
    // 获取目标NPC
    structs::ChrIns* targetNpc = GetTargetNpc();
    if (!targetNpc) {
        // 没有目标NPC时，标记数据无效
        data.data_valid = false;
        data.effect_count = 0;
        data.dmg_type_count = 0;
        return;
    }
    
    data.data_valid = true;
    CollectNpcStats(data, targetNpc);
    CollectNpcEffects(data, targetNpc);
    CollectNpcDamageTypes(data, targetNpc);
}

void DataCollector::CollectWindowInfo(SharedData& data) {
    data.game_window = gGameWindow;
    
    if (gGameWindow && IsWindow(gGameWindow)) {
        GetWindowRect(gGameWindow, &data.window_rect);
        data.window_active = (GetForegroundWindow() == gGameWindow);
    } else {
        data.window_active = false;
    }
}

structs::ChrIns* DataCollector::GetTargetNpc() {
    // 获取本地玩家
    structs::ChrIns* localPlayer = GameHandler::GetLocalPlayer();
    if (!localPlayer) {
        return nullptr;
    }
    
    // 检查是否有目标
    if (localPlayer->targetHandle == -1 || localPlayer->targetHandle == 0) {
        return nullptr;
    }
    
    // 通过handle获取目标NPC
    structs::ChrIns* targetNpc = GameHandler::GetChrInsFromHandle(&localPlayer->targetHandle);
    return targetNpc;
}

void DataCollector::CollectNpcStats(SharedData& data, structs::ChrIns* npc) {
    if (!npc) {
        data.hp_current = 0;
        data.hp_max = 0;
        data.fp_current = 0;
        data.fp_max = 0;
        data.stamina_current = 0;
        data.stamina_max = 0;
        data.stagger_current = 0;
        data.stagger_max = 0;
        return;
    }
    
    // 通过moduleBag访问模块
    structs::ChrModuleBag* moduleBag = npc->moduleBag;
    if (!moduleBag) {
        return;
    }
    
    // 收集基础属性
    structs::ChrStatModule* statModule = moduleBag->statModule;
    if (statModule) {
        data.hp_current = static_cast<float>(statModule->hp);
        data.hp_max = static_cast<float>(statModule->maxHp);
        data.fp_current = static_cast<float>(statModule->fp);
        data.fp_max = static_cast<float>(statModule->maxFp);
        data.stamina_current = static_cast<float>(statModule->stamina);
        data.stamina_max = static_cast<float>(statModule->maxStamina);
    }
    
    // 收集失衡值
    structs::ChrSuperArmorModule* superArmorModule = moduleBag->superArmorModule;
    if (superArmorModule) {
        data.stagger_current = superArmorModule->stagger;
        data.stagger_max = superArmorModule->maxStagger;
    }
}

void DataCollector::CollectNpcEffects(SharedData& data, structs::ChrIns* npc) {
    if (!npc) {
        data.effect_count = 0;
        return;
    }
    
    structs::ChrModuleBag* moduleBag = npc->moduleBag;
    if (!moduleBag) {
        data.effect_count = 0;
        return;
    }
    
    structs::ChrResistModule* resistModule = moduleBag->resistModule;
    if (!resistModule) {
        data.effect_count = 0;
        return;
    }
    
    int effectIndex = 0;
    
    // 中毒
    if (resistModule->poisonResist > 0 && effectIndex < MAX_EFFECTS) {
        SetEffectInfo(data.effects[effectIndex], L"Poison",
                     static_cast<float>(resistModule->poisonResist),
                     static_cast<float>(resistModule->maxPoisonResist),
                     resistModule->poisonResist < resistModule->maxPoisonResist);
        effectIndex++;
    }
    
    // 猩红腐败
    if (resistModule->scarletRotResist > 0 && effectIndex < MAX_EFFECTS) {
        SetEffectInfo(data.effects[effectIndex], L"ScarletRot",
                     static_cast<float>(resistModule->scarletRotResist),
                     static_cast<float>(resistModule->maxScarletRotResist),
                     resistModule->scarletRotResist < resistModule->maxScarletRotResist);
        effectIndex++;
    }
    
    // 出血
    if (resistModule->hemorrhageResist > 0 && effectIndex < MAX_EFFECTS) {
        SetEffectInfo(data.effects[effectIndex], L"Hemorrhage",
                     static_cast<float>(resistModule->hemorrhageResist),
                     static_cast<float>(resistModule->maxHemorrhageResist),
                     resistModule->hemorrhageResist < resistModule->maxHemorrhageResist);
        effectIndex++;
    }
    
    // 死亡凋零
    if (resistModule->deathBlightResist > 0 && effectIndex < MAX_EFFECTS) {
        SetEffectInfo(data.effects[effectIndex], L"DeathBlight",
                     static_cast<float>(resistModule->deathBlightResist),
                     static_cast<float>(resistModule->maxDeathBlightResist),
                     resistModule->deathBlightResist < resistModule->maxDeathBlightResist);
        effectIndex++;
    }
    
    // 冻伤
    if (resistModule->frostbiteResist > 0 && effectIndex < MAX_EFFECTS) {
        SetEffectInfo(data.effects[effectIndex], L"Frostbite",
                     static_cast<float>(resistModule->frostbiteResist),
                     static_cast<float>(resistModule->maxFrostbiteResist),
                     resistModule->frostbiteResist < resistModule->maxFrostbiteResist);
        effectIndex++;
    }
    
    // 睡眠
    if (resistModule->sleepResist > 0 && effectIndex < MAX_EFFECTS) {
        SetEffectInfo(data.effects[effectIndex], L"Sleep",
                     static_cast<float>(resistModule->sleepResist),
                     static_cast<float>(resistModule->maxSleepResist),
                     resistModule->sleepResist < resistModule->maxSleepResist);
        effectIndex++;
    }
    
    // 发狂
    if (resistModule->madnessResist > 0 && effectIndex < MAX_EFFECTS) {
        SetEffectInfo(data.effects[effectIndex], L"Madness",
                     static_cast<float>(resistModule->madnessResist),
                     static_cast<float>(resistModule->maxMadnessResist),
                     resistModule->madnessResist < resistModule->maxMadnessResist);
        effectIndex++;
    }
    
    data.effect_count = effectIndex;
}

void DataCollector::CollectNpcDamageTypes(SharedData& data, structs::ChrIns* npc) {
    // 清零伤害类型数据
    data.dmg_type_count = 0;
    for (int i = 0; i < MAX_DMG_TYPES; i++) {
        data.dmg_types[i].resistance = 1.0f;
        data.dmg_types[i].isWeakness = false;
        data.dmg_types[i].isImmune = false;
    }
    
    if (!npc) {
        return;
    }
    
    // 获取NPC参数
    structs::NpcParam* npcParam = GameHandler::GetNpcParam(npc->paramId);
    if (!npcParam) {
        return;
    }
    
    // 收集伤害类型抗性
    // 抗性值 < 1.0 表示弱点（受到更多伤害）
    // 抗性值 > 1.0 表示抗性（受到更少伤害）
    // 抗性值 = 0.0 表示免疫
    
    struct DmgTypeData {
        const wchar_t* name;
        float resistance;
        const char* textureName;
    };
    
    std::vector<DmgTypeData> dmgTypes = {
        {L"Fire", npcParam->fireDamageCutRate, "fire"},
        {L"Magic", npcParam->magicDamageCutRate, "magic"},
        {L"Lightning", npcParam->thunderDamageCutRate, "lightning"},
        {L"Holy", npcParam->darkDamageCutRate, "holy"}
    };
    
    // 按抗性值排序（弱点在前）
    std::sort(dmgTypes.begin(), dmgTypes.end(), [](const DmgTypeData& a, const DmgTypeData& b) {
        return a.resistance < b.resistance;
    });
    
    int index = 0;
    for (const auto& dmgType : dmgTypes) {
        if (index >= MAX_DMG_TYPES) break;
        
        wcsncpy_s(data.dmg_types[index].name, dmgType.name, MAX_EFFECT_NAME_LEN - 1);
        data.dmg_types[index].resistance = dmgType.resistance;
        data.dmg_types[index].isImmune = (dmgType.resistance <= 0.0f);
        data.dmg_types[index].isWeakness = (dmgType.resistance < 1.0f && dmgType.resistance > 0.0f);
        
        index++;
    }
    
    data.dmg_type_count = index;
    data.show_dmg_types = (index > 0);
}

void DataCollector::SetEffectInfo(EffectInfo& effect, const wchar_t* name, float current, float max, bool active) {
    wcsncpy_s(effect.name, name, MAX_EFFECT_NAME_LEN - 1);
    effect.name[MAX_EFFECT_NAME_LEN - 1] = L'\0';
    effect.current_value = current;
    effect.max_value = max;
    effect.active = active;
}

} // namespace souls_vision
