#ifndef STRUCTUSES_AND_CLASSES_HEADER_
#define STRUCTUSES_AND_CLASSES_HEADER_
#include <stdint.h>
#include <functional>
#include <chrono>
#include <string>
#include <cstring>
struct Entity
{
    uint32_t id;
};
struct Task{
    std::function<void()> func;
    bool isBackTask = true;
};
struct TickTask{
    std::function<void()> func;
    std::chrono::milliseconds intervalMs;
    std::chrono::steady_clock::time_point lastExecutedMs;
    size_t id;
    size_t executionsRemaining;
    bool active;
    float timeAccumulator = 0.0f;
    bool isBackTask=true;

};
class Event {
    public:
    virtual ~Event() = default;
};
template<typename T>
class SystemECS;

enum class SystemMode {
    Serial,
    Parallel
};

enum class TriggerType {
    Always,
    TimeInterval,
    TickInterval
};
struct EventData {
    void* ptr;
    size_t size;
};
struct ComponentData {

    void* data = nullptr;
    size_t elementSize = 0;
    size_t capacity = 0;
    std::vector<uint32_t> dense;
    std::vector<uint32_t> sparse;

    ComponentData(size_t elemSize, size_t cap);

    ~ComponentData();
};

using SystemCallback = std::function<void(float)>;

struct System {
    std::string name;
    SystemCallback callback;
    bool enabled = true;
};

struct SystemDesc {
    std::string systemName;
    TriggerType trigger = TriggerType::Always;
    float timeInterval = 0.0f;
    size_t tickInterval = 0;
    bool enabled = true;
    float timeAcc = 0.0f;
    size_t tickAcc = 0;
};


class Clock;
#pragma once
class Module {
public:
    virtual ~Module() = default;
    virtual void onLoad() {}
    virtual void onUnload() {}
};
#endif