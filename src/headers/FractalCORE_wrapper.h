#pragma once
#include "FractalCORE_gateway.h"
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <string>
#include <functional>
#include <stdexcept>

// --- ABI Helpers ---

template<typename T>
struct EventContext {
    void (*originalHandler)(const T&, void*);
    void* originalUserData;
    uint32_t eventID;
};

class ModuleAPI {
private:
    FractalCORE_Gateway* gw;
    std::unordered_map<std::string, uint32_t> eventIDCache;

    uint32_t getEventId(const std::string& name) {
        if (eventIDCache.count(name)) return eventIDCache[name];
        uint32_t id = gw->registerEvent(gw->api, name);
        eventIDCache[name] = id;
        return id;
    }

public:
    ModuleAPI(FractalCORE_Gateway* gateway) : gw(gateway) {}

    // ------------------------------------
    // 1. Entity, Task, Time
    // ------------------------------------

    Entity createEntity() {
        return (gw && gw->createEntity) ? gw->createEntity(gw->api) : Entity{0};
    }
    
    void enqueueTask(const Task& task) {
        if (gw && gw->enqueueTask) gw->enqueueTask(gw->api, task);
    }
    
    void registerIntervalTask(const TickTask& tickTask) {
        if (gw && gw->registerIntervalTask) gw->registerIntervalTask(gw->api, tickTask);
    }

    float getDeltaTime() const {
        return (gw && gw->getDeltaTime) ? gw->getDeltaTime(gw->api) : 0.0f;
    }
    
    Clock& getEngineClock() {
        if (!gw || !gw->getEngineClock) throw std::runtime_error("ModuleAPI: Engine Clock unavailable");
        return gw->getEngineClock(gw->api);
    }
    
    void stop() {
        if (gw && gw->stop) gw->stop(gw->api);
    }

    // ------------------------------------
    // 2. Components (ECS)
    // ------------------------------------
     
    template<typename T>
    void registerComponent(const std::string& name, size_t capacity = 10000) {
        if (!gw || !gw->registerComponent) throw std::runtime_error("ModuleAPI: registerComponent null");
        gw->registerComponent(gw->api, name, sizeof(T), capacity);
    }
    
    template<typename T>
    void attachComponent(Entity e, const std::string& name, const T& data) {
        if (!gw || !gw->attachComponent) throw std::runtime_error("ModuleAPI: attachComponent null");
        gw->attachComponent(gw->api, e, name, (void*)&data);
    }

    void removeComponent(Entity e, const std::string& name) {
        if (gw && gw->removeComponent) gw->removeComponent(gw->api, e, name);
    }

    template<typename T>
    T* getComponent(Entity e, const std::string& name) {
        if (!gw || !gw->getComponent) return nullptr;
        return static_cast<T*>(gw->getComponent(gw->api, e, name));
    }
    
    bool hasComponent(Entity e, const std::string& name) {
        return (gw && gw->hasComponent) ? gw->hasComponent(gw->api, e, name) : false;
    }

    // ------------------------------------
    // 3. Systems (ECS)
    // ------------------------------------

    /**
     * @brief Registers a system.
     * Logic: Defines a C-style callback that is called by Core. 
     * Inside that callback, it triggers `updateParallel` to iterate components.
     */
template<typename T>
void registerSystem(const std::string& componentName,
                    void (*updateFunc)(Entity, T&, float),
                    TriggerType trigger = TriggerType::Always,
                    float timeInterval = 0.0f, 
                    size_t tickInterval = 0) {
    
    struct SystemModuleContext {
        std::string compName;
        void (*uFunc)(Entity, T&, float);
        FractalCORE_Gateway* gateway;
        float currentDt; // Добавляем поле для хранения текущего dt
    };
    
    auto* moduleContext = new SystemModuleContext{ componentName, updateFunc, gw, 0.0f };
    std::string systemName = componentName + "_UpdateSystem"; 

    // Трамплин для всей системы
    auto core_trampoline = [](float dt, void* userData) {
        auto* ctx = static_cast<SystemModuleContext*>(userData);
        ctx->currentDt = dt; // Сохраняем актуальный dt для этого кадра

        // Колбэк для каждой сущности
        auto entity_callback = [](Entity e, void* raw_data, void* userCtx) {
            auto* sCtx = static_cast<SystemModuleContext*>(userCtx);
            T& component = *static_cast<T*>(raw_data);
            sCtx->uFunc(e, component, sCtx->currentDt); // Вызываем логику модуля
        };

        // ВЫЗОВ С 5 АРГУМЕНТАМИ
        ctx->gateway->updateParallel(ctx->gateway->api, ctx->compName, entity_callback, ctx, 64);
    };

    gw->registerSystem(gw->api, systemName, core_trampoline, static_cast<void*>(moduleContext));

    SystemDesc desc;
    desc.systemName = systemName;
    desc.trigger = trigger;
    desc.timeInterval = timeInterval;
    desc.tickInterval = tickInterval;
    desc.enabled = true;
    
    gw->registerSystemInLoop(gw->api, desc);
}
    
    // Direct access to updateParallel
template<typename T>
void updateParallel(const std::string& componentName,
                    void (*func)(Entity, T&),
                    size_t chunkSize = 64) {
    if (!gw || !gw->updateParallel) return;
    
    // Вспомогательная структура, чтобы прокинуть указатель на простую функцию
    struct SimpleFuncCtx {
        void (*f)(Entity, T&);
    };
    auto* ctx = new SimpleFuncCtx{ func };

    auto wrapper_func = [](Entity e, void* data, void* userCtx) {
        auto* sCtx = static_cast<SimpleFuncCtx*>(userCtx);
        T& component = *static_cast<T*>(data);
        sCtx->f(e, component);
    };
    
    // Теперь здесь 5 аргументов:
    // 1. api, 2. name, 3. callback, 4. context, 5. chunkSize
    gw->updateParallel(gw->api, componentName, wrapper_func, ctx, chunkSize);
}

    // ------------------------------------
    // 4. Events
    // ------------------------------------

    template<typename T>
    void subscribe(const std::string& eventName, 
                   void (*handler)(const T&, void*), 
                   void* userData = nullptr) {
        
        if (!gw || !gw->subscribe) return;
            
        uint32_t eventID = getEventId(eventName);
        auto* context = new EventContext<T>{ handler, userData, eventID }; 
        
        auto core_invoker = [](uint32_t id, const EventData& data, void* contextPtr) {
            auto* context = static_cast<EventContext<T>*>(contextPtr);
            if (context && context->originalHandler) {
                // 'ptr' comes from Structs&Classes.h EventData struct
                const T& typedData = *static_cast<const T*>(data.ptr);
                context->originalHandler(typedData, context->originalUserData);
            }
        };

        gw->subscribe(gw->api, eventID, core_invoker, static_cast<void*>(context));
    }
    
    template<typename T>
    void emitEvent(const std::string& eventName, const T& data) {
        if (gw && gw->emitEvent) {
        
            gw->emitEvent(gw->api, getEventId(eventName), (void*)&data, sizeof(T));
        }
    }
    
    template<typename T>
    void pushEvent(const std::string& eventName, const T& data) {
        if (gw && gw->pushEvent) {
        
            gw->pushEvent(gw->api, getEventId(eventName), (void*)&data, sizeof(T));
        }
    }
};