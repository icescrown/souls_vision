//
// 共享内存通信模块实现
//

#include "shared_memory.h"
#include <cstring>

namespace souls_vision {

SharedMemory::SharedMemory()
    : map_file_(nullptr)
    , mutex_(nullptr)
    , event_data_ready_(nullptr)
    , event_data_consumed_(nullptr)
    , shared_data_(nullptr)
    , is_host_(false)
    , connected_(false) {
}

SharedMemory::~SharedMemory() {
    Shutdown();
}

bool SharedMemory::InitializeAsHost() {
    is_host_ = true;
    
    if (!CreateSharedMemory()) {
        return false;
    }
    
    if (!CreateSyncObjects()) {
        return false;
    }
    
    // 初始化共享数据
    shared_data_->version = CURRENT_VERSION;
    shared_data_->data_valid = false;
    shared_data_->game_process_alive = true;
    shared_data_->renderer_process_alive = false;
    shared_data_->timestamp = GetCurrentTimestamp();
    
    connected_ = true;
    return true;
}

bool SharedMemory::InitializeAsClient() {
    is_host_ = false;
    
    if (!OpenSharedMemory()) {
        return false;
    }
    
    if (!OpenSyncObjects()) {
        return false;
    }
    
    // 检查版本兼容性
    if (shared_data_->version != CURRENT_VERSION) {
        SetError(L"版本不兼容");
        return false;
    }
    
    // 标记渲染进程存活
    shared_data_->renderer_process_alive = true;
    
    connected_ = true;
    return true;
}

void SharedMemory::Shutdown() {
    if (shared_data_) {
        if (is_host_) {
            shared_data_->game_process_alive = false;
        } else {
            shared_data_->renderer_process_alive = false;
        }
    }
    
    if (shared_data_) {
        UnmapViewOfFile(shared_data_);
        shared_data_ = nullptr;
    }
    
    if (map_file_) {
        CloseHandle(map_file_);
        map_file_ = nullptr;
    }
    
    if (mutex_) {
        CloseHandle(mutex_);
        mutex_ = nullptr;
    }
    
    if (event_data_ready_) {
        CloseHandle(event_data_ready_);
        event_data_ready_ = nullptr;
    }
    
    if (event_data_consumed_) {
        CloseHandle(event_data_consumed_);
        event_data_consumed_ = nullptr;
    }
    
    connected_ = false;
}

bool SharedMemory::WriteData(const SharedData& data) {
    if (!connected_ || !shared_data_) {
        return false;
    }
    
    DWORD wait_result = WaitForSingleObject(mutex_, 1000);
    if (wait_result != WAIT_OBJECT_0) {
        return false;
    }
    
    // 复制数据
    memcpy(shared_data_, &data, sizeof(SharedData));
    
    // 标记数据有效
    shared_data_->data_valid = true;
    shared_data_->game_process_alive = true;
    
    ReleaseMutex(mutex_);
    
    // 通知数据就绪
    SetEvent(event_data_ready_);
    
    return true;
}

bool SharedMemory::ReadData(SharedData& data) {
    if (!connected_ || !shared_data_) {
        return false;
    }
    
    // 检查数据是否有效
    if (!shared_data_->data_valid) {
        return false;
    }
    
    DWORD wait_result = WaitForSingleObject(mutex_, 1000);
    if (wait_result != WAIT_OBJECT_0) {
        return false;
    }
    
    // 复制数据
    memcpy(&data, shared_data_, sizeof(SharedData));
    
    ReleaseMutex(mutex_);
    
    return true;
}

bool SharedMemory::IsConnected() const {
    return connected_;
}

std::wstring SharedMemory::GetLastError() const {
    return last_error_;
}

bool SharedMemory::WaitForData(DWORD timeout_ms) {
    if (!event_data_ready_) {
        return false;
    }
    
    DWORD result = WaitForSingleObject(event_data_ready_, timeout_ms);
    return result == WAIT_OBJECT_0;
}

bool SharedMemory::NotifyDataConsumed() {
    if (!event_data_consumed_) {
        return false;
    }
    
    return SetEvent(event_data_consumed_);
}

uint64_t SharedMemory::GetCurrentTimestamp() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

bool SharedMemory::CreateSharedMemory() {
    map_file_ = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        static_cast<DWORD>(SHARED_MEMORY_SIZE),
        SHARED_MEMORY_NAME
    );
    
    if (!map_file_) {
        SetError(L"创建共享内存失败");
        return false;
    }
    
    shared_data_ = static_cast<SharedData*>(MapViewOfFile(
        map_file_,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_MEMORY_SIZE
    ));
    
    if (!shared_data_) {
        SetError(L"映射共享内存失败");
        return false;
    }
    
    // 清零内存
    ZeroMemory(shared_data_, SHARED_MEMORY_SIZE);
    
    return true;
}

bool SharedMemory::OpenSharedMemory() {
    map_file_ = OpenFileMappingW(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        SHARED_MEMORY_NAME
    );
    
    if (!map_file_) {
        SetError(L"打开共享内存失败，游戏进程可能未运行");
        return false;
    }
    
    shared_data_ = static_cast<SharedData*>(MapViewOfFile(
        map_file_,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_MEMORY_SIZE
    ));
    
    if (!shared_data_) {
        SetError(L"映射共享内存失败");
        return false;
    }
    
    return true;
}

bool SharedMemory::CreateSyncObjects() {
    // 创建互斥锁
    mutex_ = CreateMutexW(nullptr, FALSE, MUTEX_NAME);
    if (!mutex_) {
        SetError(L"创建互斥锁失败");
        return false;
    }
    
    // 创建数据就绪事件
    event_data_ready_ = CreateEventW(nullptr, FALSE, FALSE, EVENT_DATA_READY);
    if (!event_data_ready_) {
        SetError(L"创建数据就绪事件失败");
        return false;
    }
    
    // 创建数据消费事件
    event_data_consumed_ = CreateEventW(nullptr, FALSE, FALSE, EVENT_DATA_CONSUMED);
    if (!event_data_consumed_) {
        SetError(L"创建数据消费事件失败");
        return false;
    }
    
    return true;
}

bool SharedMemory::OpenSyncObjects() {
    // 打开互斥锁
    mutex_ = OpenMutexW(SYNCHRONIZE, FALSE, MUTEX_NAME);
    if (!mutex_) {
        SetError(L"打开互斥锁失败");
        return false;
    }
    
    // 打开数据就绪事件
    event_data_ready_ = OpenEventW(SYNCHRONIZE, FALSE, EVENT_DATA_READY);
    if (!event_data_ready_) {
        SetError(L"打开数据就绪事件失败");
        return false;
    }
    
    // 打开数据消费事件
    event_data_consumed_ = OpenEventW(SYNCHRONIZE, FALSE, EVENT_DATA_CONSUMED);
    if (!event_data_consumed_) {
        SetError(L"打开数据消费事件失败");
        return false;
    }
    
    return true;
}

void SharedMemory::SetError(const std::wstring& error) {
    last_error_ = error;
}

} // namespace souls_vision