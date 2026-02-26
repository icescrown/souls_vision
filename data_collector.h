//
// 数据收集模块 - 收集选中NPC的信息
//

#ifndef SOULS_VISION_DATA_COLLECTOR_H
#define SOULS_VISION_DATA_COLLECTOR_H

#include "shared_memory.h"
#include "structs/chr_ins.h"

namespace souls_vision {

class DataCollector {
public:
    // 收集所有数据到共享内存结构
    static void Collect(SharedData& data);

private:
    // 收集窗口信息
    static void CollectWindowInfo(SharedData& data);
    
    // 获取当前目标NPC
    static structs::ChrIns* GetTargetNpc();
    
    // 收集NPC状态数据
    static void CollectNpcStats(SharedData& data, structs::ChrIns* npc);
    
    // 收集NPC效果数据
    static void CollectNpcEffects(SharedData& data, structs::ChrIns* npc);
    
    // 收集NPC伤害类型抗性
    static void CollectNpcDamageTypes(SharedData& data, structs::ChrIns* npc);
    
    // 设置效果信息
    static void SetEffectInfo(EffectInfo& effect, const wchar_t* name, float current, float max, bool active);
};

} // namespace souls_vision

#endif // SOULS_VISION_DATA_COLLECTOR_H
