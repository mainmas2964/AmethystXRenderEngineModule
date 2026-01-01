#include "RenderThread.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[1024]; glGetShaderInfoLog(s, 1024, nullptr, buf);
        std::cerr << "Shader compile error: " << buf << std::endl;
    }
    return s;
}

RenderThread::RenderThread() : windowHandle_(nullptr) {}
RenderThread::~RenderThread() { stop(); }

void RenderThread::start() {
    if (running_) return;
    running_ = true;
    th_ = std::thread([this]{ loop(); });
}

void RenderThread::stop() {
    if (!running_) return;
    running_ = false;
    cv_.notify_all();
    if (th_.joinable()) th_.join();
}

void RenderThread::submitDrawList(std::vector<DrawCommand>&& list) {
    {
        std::lock_guard lk(mtx_);
        drawQueue_.emplace_back(std::move(list));
    }
    cv_.notify_one();
}

void RenderThread::scheduleGPU(std::function<void()>&& task) {
    {
        std::lock_guard lk(mtx_);
        gpuTasks_.emplace_back(std::move(task));
    }
    cv_.notify_one();
}

void RenderThread::initGLFW() {
    if (!glfwInit()) {
        std::cerr << "RenderThread: GLFW init failed\n";
        return;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    GLFWwindow* win = glfwCreateWindow(800, 600, "FractalHorizon", nullptr, nullptr);
    if (!win) {
        std::cerr << "RenderThread: GLFW create window failed\n";
        glfwTerminate();
        return;
    }
    windowHandle_ = win;
}

void RenderThread::cleanupGLFW() {
    if (triangleReady) {
        glDeleteBuffers(1, &triangleVBO);
        glDeleteVertexArrays(1, &triangleVAO);
        glDeleteProgram(triangleProgram);
        triangleReady = false;
    }
    if (windowHandle_) {
        glfwDestroyWindow(static_cast<GLFWwindow*>(windowHandle_));
        windowHandle_ = nullptr;
    }
    glfwTerminate();
}

void RenderThread::loop() {
    initGLFW();
    if (!windowHandle_) return;

    GLFWwindow* win = static_cast<GLFWwindow*>(windowHandle_);
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "RenderThread: failed to initialize GLAD\n";
        cleanupGLFW();
        return;
    }

    scheduleGPU([this]{
        const char* vs = R"(
        #version 330 core
	layout (location = 0) in vec3 aPos;   // the position variable has attribute position 0
	layout (location = 1) in vec3 aColor; // the color variable has attribute position 1
	out vec3 ourColor; // output a color to the fragment shader
	void main()
        {
    	gl_Position = vec4(aPos, 1.0);
    	ourColor = aColor; // set ourColor to the input color we got from the vertex data
	}

        )";

        const char* fs = R"(
        #version 330 core
	out vec4 FragColor;
	in vec3 ourColor;

	void main()
	{
	    FragColor = vec4(ourColor, 1.0);
	}

        )";
        unsigned int vsId = compileShader(GL_VERTEX_SHADER, vs);
        unsigned int fsId = compileShader(GL_FRAGMENT_SHADER, fs);
        unsigned int prog = glCreateProgram();
        glAttachShader(prog, vsId);
        glAttachShader(prog, fsId);
        glLinkProgram(prog);
        int ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            char buf[1024]; glGetProgramInfoLog(prog, 1024, nullptr, buf);
            std::cerr << "Program link error: " << buf << std::endl;
        }
        glDeleteShader(vsId); glDeleteShader(fsId);

        float triVerts[] = {
             // positions         // colors
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
     0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top
        };
        unsigned int vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(triVerts), triVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        // position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
	glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        triangleVAO = vao;
        triangleVBO = vbo;
        triangleProgram = prog;
        triangleReady = true;
    });

    while (running_ && !glfwWindowShouldClose(win)) {

        std::vector<DrawCommand> list;
        std::deque<std::function<void()>> tasks;

        {
            std::unique_lock lk(mtx_);
            cv_.wait_for(lk, std::chrono::milliseconds(1), [&]{ return !drawQueue_.empty() || !gpuTasks_.empty() || !running_; });
            if (!drawQueue_.empty()) { list = std::move(drawQueue_.front()); drawQueue_.pop_front(); }
            if (!gpuTasks_.empty()) { tasks.swap(gpuTasks_); }
        }

        for (auto &t : tasks) {
            try { t(); } catch (...) { std::cerr << "RenderThread: GPU task exception\n"; }
        }

        glViewport(0, 0, 800, 600);
        glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (triangleReady) {
            glUseProgram(triangleProgram);
            glBindVertexArray(triangleVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
            glUseProgram(0);
        }

        if (!list.empty()) renderList(list);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    cleanupGLFW();
}

void RenderThread::renderList(const std::vector<DrawCommand>& list) {
    (void)list;
}
