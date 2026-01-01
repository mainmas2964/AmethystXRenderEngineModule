#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

struct DrawCommand {
    uint64_t meshId;
    uint64_t materialId;
    glm::mat4 modelMatrix;
};

class RenderThread {
public:
    RenderThread();
    ~RenderThread();

    void start();
    void stop();

    void submitDrawList(std::vector<DrawCommand>&& list);
    void scheduleGPU(std::function<void()>&& task);

private:
    void loop();
    void initGLFW();
    void cleanupGLFW();
    void renderList(const std::vector<DrawCommand>& list);

    std::thread th_;
    std::atomic<bool> running_{false};

    std::mutex mtx_;
    std::condition_variable cv_;
    std::deque<std::vector<DrawCommand>> drawQueue_;
    std::deque<std::function<void()>> gpuTasks_;

    void* windowHandle_;
    unsigned int triangleVAO = 0;
    unsigned int triangleVBO = 0;
    unsigned int triangleProgram = 0;
    bool triangleReady = false;
};
