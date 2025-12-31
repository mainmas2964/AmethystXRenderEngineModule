#pragma once
#include "Structs&Classes.h"
#include <string>
#include <functional>

// Forward declarations
class Clock;

struct FractalCORE_Gateway {
    void* api; 

    // --- Core Management ---
    void (*stop)(void*);
    float (*getDeltaTime)(void*);
    Clock& (*getEngineClock)(void*);

    // --- Task System ---
    void (*enqueueTask)(void*, const Task&);
    void (*registerIntervalTask)(void*, const TickTask&);

    // --- ECS: Entities & Components ---
    Entity (*createEntity)(void*);
    
    void (*registerComponent)(void*, const std::string&, size_t, size_t);
    void (*attachComponent)(void*, Entity, const std::string&, void*);
    void (*removeComponent)(void*, Entity, const std::string&);
    void* (*getComponent)(void*, Entity, const std::string&);
    bool (*hasComponent)(void*, Entity, const std::string&);
    ComponentData* (*getComponentData)(void*, const std::string&);

    // --- ECS: Systems ---
    // Registers a system. 
    // Param 3: Raw function pointer taking (dt, userData)
    // Param 4: User data context (passed back to the function pointer)
    void (*registerSystem)(void*, const std::string&, void (*)(float, void*), void*);
    
    // Registers the loop configuration (Trigger, Intervals)
    void (*registerSystemInLoop)(void*, SystemDesc&);
    
    // Parallel iteration wrapper
    // Takes a std::function because ECS::updateParallel requires it
    void (*updateParallel)(void* api, const std::string& name, void (*func)(Entity, void*, void*), void* userContext, size_t chunkSize);
    
    // --- Event Manager ---
    uint32_t (*registerEvent)(void*, const std::string&);
    void (*pushEvent)(void*, uint32_t, void*, size_t);
    void (*emitEvent)(void*, uint32_t, void*, size_t);
    
    // Subscribe callback takes (eventID, EventData, userData)
    void (*subscribe)(void*, uint32_t, void (*)(uint32_t, const EventData&, void*), void*);
};