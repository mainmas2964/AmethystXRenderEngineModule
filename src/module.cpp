#include "headers/FractalCORE_gateway.h"
#include "headers/FractalCORE_wrapper.h"
#include "headers/Structs&Classes.h"
#include "RenderThread.h"
#include <iostream>

static RenderThread g_render;

extern "C" void onLoad(FractalCORE_Gateway* gateway) {
    static ModuleAPI moduleApi(gateway);
    std::cout << "--- AmethystXRenderEngineModule Initialized ---\n";

    g_render.start();

    g_render.scheduleGPU([]{
        std::cout << "GPU init task executed in render thread\n";
    });

    g_render.submitDrawList(std::vector<DrawCommand>{});
}

extern "C" void onUnload() {
    std::cout << "--- AmethystXRenderEngineModule Unloaded ---\n";
    g_render.stop();
}
