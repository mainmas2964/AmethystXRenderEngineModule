#include "headers/FractalCORE_gateway.h"
#include "headers/FractalCORE_wrapper.h"
#include "headers/Structs&Classes.h"
#include <iostream>
#include <string>

// --- 1. Module-specific structures ---

// Component holding 2D position
struct PositionComponent {
    float x = 0.0f;
    float y = 0.0f;
};

// Event sent when a player/entity moves
struct PlayerMoveEvent {
    Entity entity;
    float newX;
    float newY;
};

// --- 2. System logic ---

// Update function for movement system (high-level C++ logic)
void MovementSystemUpdate(Entity e, PositionComponent& pos, float dt) {
    // Simple movement: advance X based on dt
    pos.x += dt * 10.0f;
}

// Event handler for PlayerMoveEvent
void OnPlayerMove(const PlayerMoveEvent& eventData, void* userData) {
    ModuleAPI* api = static_cast<ModuleAPI*>(userData);


    // Update the Position component via the module API if present
    PositionComponent* pos = api->getComponent<PositionComponent>(eventData.entity, "Position");
    if (pos) {
        pos->x = eventData.newX;
        pos->y = eventData.newY;
    }
}

// Immediate update helper used for mass updates (no dt version)
// This will be used with ModuleAPI::updateParallel which expects a
// function pointer of signature void(Entity, T&).
void ImmediateMove(Entity e, PositionComponent& pos) {
    // simple deterministic update to verify updates occurred
    pos.x += 1.0f;
    pos.y += 0.5f;
}

// --- 3. Module entry points ---

extern "C" {
    // Called by the core when the module is loaded
    void onLoad(FractalCORE_Gateway* gateway) {
        // Use a local static ModuleAPI to ensure it lives until module unload
        static ModuleAPI moduleApi(gateway);

        std::cout << "--- My Module Initialized ---" << std::endl;

        try {
            // Register component type
            moduleApi.registerComponent<PositionComponent>("Position");
            std::cout << "Registered Component: Position" << std::endl;

            // Register an always-running system that iterates over "Position"
            moduleApi.registerSystem<PositionComponent>(
                "Position",            // component name
                MovementSystemUpdate,   // update function
                TriggerType::Always,    // trigger: every frame
                0.0f,                   // timeInterval (unused)
                0                       // tickInterval (unused)
            );
            std::cout << "Registered System: Position_UpdateSystem (Always)" << std::endl;

            // Subscribe to PlayerMove events
            moduleApi.subscribe<PlayerMoveEvent>(
                "PlayerMove",
                OnPlayerMove,
                &moduleApi
            );
            std::cout << "Subscribed to Event: PlayerMove" << std::endl;

            // Create 10k entities with a Position component
            const size_t COUNT = 10000;
            std::cout << "Creating " << COUNT << " entities..." << std::endl;
            for (size_t i = 0; i < COUNT; ++i) {
                Entity e = moduleApi.createEntity();
                PositionComponent pc;
                pc.x = static_cast<float>(i);
                pc.y = 0.0f;
                moduleApi.attachComponent<PositionComponent>(e, "Position", pc);
                if (((i + 1) % 1000) == 0) {
                    std::cout << "Created " << (i + 1) << " entities." << std::endl;
                }
            }

            std::cout << "All entities created. Running 5 immediate parallel update passes..." << std::endl;
            // Run a few immediate parallel update passes to advance positions.
            for (int pass = 0; pass < 5; ++pass) {
                moduleApi.updateParallel<PositionComponent>("Position", ImmediateMove, 256);
                std::cout << "Completed update pass " << (pass + 1) << "/5" << std::endl;
            }

            std::cout << "Mass creation and updates finished." << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Module Initialization Error: " << e.what() << std::endl;
        }
    }

    // Called by the core when the module is unloaded (optional)
    void onUnload() {
        std::cout << "--- My Module Unloaded ---" << std::endl;
        // Free any dynamic resources here if needed (EventContexts, SystemModuleContexts)
    }
}