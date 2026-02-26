//
// 共享内存通信模块 - 用于游戏进程和外部渲染进程通信
//

#ifndef SOULS_VISION_SHARED_MEMORY_H
#define SOULS_VISION_SHARED_MEMORY_H

#include <windows.h>
#include <string>
#include <atomic>
#include <chrono>

namespace souls_vision {

// 共享内存名称
constexpr const wchar_t* SHARED_MEMORY_NAME = L"SoulsVision_SharedMemory";
constexpr const wchar_t* MUTEX_NAME = L"SoulsVision_Mutex";
constexpr const wchar_t* EVENT_DATA_READY = L"SoulsVision_DataReady";
constexpr const wchar_t* EVENT_DATA_CONSUMED = L"SoulsVision_DataConsumed";

// 最大效果数量
constexpr int MAX_EFFECTS = 16;
constexpr int MAX_EFFECT_NAME_LEN = 32;
constexpr int MAX_DMG_TYPES = 4;  // 火、魔法、雷电、神圣

// 状态条类型
enum class SharedBarType : int {
    HP = 0,
    FP,
    STAMINA,
    STAGGER,
    POISON,
    SCARLET_ROT,
    HEMORRHAGE,
    DEATH_BLIGHT,
    FROSTBITE,
    SLEEP,
    MADNESS,
    COUNT
};

// 伤害类型
enum class DamageType : int {
    FIRE = 0,
    MAGIC,
    LIGHTNING,
    HOLY,
    COUNT
};

// 效果信息结构
struct EffectInfo {
    wchar_t name[MAX_EFFECT_NAME_LEN];
    float current_value;
    float max_value;
    bool active;
};

// 伤害类型信息
struct DamageTypeInfo {
    wchar_t name[MAX_EFFECT_NAME_LEN];
    float resistance;  // 抗性值，<1.0表示弱点，>1.0表示抗性
    bool isWeakness;   // 是否为弱点
    bool isImmune;     // 是否免疫
};

// 共享数据结构 - 游戏进程写入，外部进程读取
// 注意：这个结构会被多个进程共享，需要保持布局一致
#pragma pack(push, 8)
struct SharedData {
    // 版本号，用于兼容性检查
    uint32_t version;
    
    // 数据有效性标志
    volatile bool data_valid;
    
    // 游戏窗口信息
    HWND game_window;
    RECT window_rect;
    bool window_active;
    
    // NPC状态条数据
    float hp_current;
    float hp_max;
    float fp_current;
    float fp_max;
    float stamina_current;
    float stamina_max;
    float stagger_current;
    float stagger_max;
    
    // 状态效果
    EffectInfo effects[MAX_EFFECTS];
    int effect_count;
    
    // 伤害类型抗性
    bool show_dmg_types;
    DamageTypeInfo dmg_types[MAX_DMG_TYPES];
    int dmg_type_count;
    
    // 配置数据
    bool bars_visible[static_cast<int>(SharedBarType::COUNT)];
    bool bars_hide_text[static_cast<int>(SharedBarType::COUNT)];

    // 时间戳
    uint64_t timestamp;
    
    // 进程存活标志
    volatile bool game_process_alive;
    volatile bool renderer_process_alive;
};
#pragma pack(pop)

// 共享内存大小
constexpr size_t SHARED_MEMORY_SIZE = sizeof(SharedData);

// 当前数据版本
constexpr uint32_t CURRENT_VERSION = 2;  // 版本更新，因为结构有变化

class SharedMemory {
public:
    SharedMemory();
    ~SharedMemory();
    
    // 初始化共享内存（游戏进程调用）
    bool InitializeAsHost();
    
    // 初始化共享内存（外部渲染进程调用）
    bool InitializeAsClient();
    
    // 关闭共享内存
    void Shutdown();
    
    // 写入数据（游戏进程调用）
    bool WriteData(const SharedData& data);
    
    // 读取数据（外部渲染进程调用）
    bool ReadData(SharedData& data);
    
    // 检查连接状态
    bool IsConnected() const;
    
    // 获取最后错误
    std::wstring GetLastError() const;
    
    // 等待数据就绪（外部渲染进程调用，带超时）
    bool WaitForData(DWORD timeout_ms = 1000);
    
    // 通知数据已消费（外部渲染进程调用）
    bool NotifyDataConsumed();

private:
    HANDLE map_file_;
    HANDLE mutex_;
    HANDLE event_data_ready_;
    HANDLE event_data_consumed_;
    SharedData* shared_data_;
    std::wstring last_error_;
    bool is_host_;
    bool connected_;
    
    // 内部方法
    uint64_t GetCurrentTimestamp();
    bool CreateSharedMemory();
    bool OpenSharedMemory();
    bool CreateSyncObjects();
    bool OpenSyncObjects();
    void SetError(const std::wstring& error);
};

} // namespace souls_vision

#endif // SOULS_VISION_SHARED_MEMORY_H
