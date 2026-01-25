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
    void* (*getEngineClock)(void*); 

    // --- Task System ---
    void (*enqueueTask)(void*, const void* taskPtr); 
    void (*registerIntervalTask)(void*, const void* tickTaskPtr);

    // --- ECS: Entities and Components ---
    uint32_t (*createEntity)(void*);
    

    void (*registerComponent)(void*, const char* name, size_t size, size_t capacity);
    void (*attachComponent)(void*, uint32_t entity, const char* name, void* data);
    void (*removeComponent)(void*, uint32_t entity, const char* name);
    void* (*getComponent)(void*, uint32_t entity, const char* name);
    bool (*hasComponent)(void*, uint32_t entity, const char* name);
    void* (*getComponentData)(void*, const char* name);
    
    void (*registerGroup)(void*, const char** componentNames, size_t count);

    // --- ECS: Systems ---
    void (*registerSystem)(void*, const char* name, void (*cb)(float, void*), void* userData);
    void (*registerSystemInLoop)(void*, void* systemDescPtr);
    
    void (*updateParallel)(void* api, const char* name, void (*func)(uint32_t, void*, void*), void* userContext, size_t chunkSize);
    void (*updateParallelGroup)(void* api, 
                                const char** components, 
                                size_t compCount,
                                void (*func)(size_t, size_t, void*), 
                                void* userContext, 
                                size_t chunkSize);

    // --- Event Manager ---
    uint32_t (*registerEvent)(void*, const char* name);
    void (*pushEvent)(void*, uint32_t id, void* data, size_t size);
    void (*emitEvent)(void*, uint32_t id, void* data, size_t size);
    void (*subscribe)(void*, uint32_t id, void (*cb)(uint32_t, const void*, void*), void* user);

    // --- SQLite gateway ---
    bool (*setSQLString)(void* api, const char* key, const char* value);
    size_t (*getSQLString)(void* api, const char* key, char* outBuffer, size_t bufferSize);
    bool (*SQLExist)(void* api, const char* key);
    bool (*SQLExecute)(void* api, const char* sql);
};
