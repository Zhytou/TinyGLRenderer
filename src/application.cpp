
#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <rapidjson/document.h>

#include <filesystem>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

#include "resourcemanager.hpp"
#include "utils.hpp"

namespace tinyglrenderer {
Application::Application(int width, int height, const std::string& title)
    : m_window(nullptr),
      m_editor(m_editorSetting, m_rendererSetting),
      m_renderer(m_rendererSetting) {
    if (width < 800 || height < 600) { throw std::runtime_error("Application::Application: Window size must be at least 800x600"); }

    // initialize glfw and glad
    setup(width, height, title);
    resize(width, height);

    // initialize static resources
    m_manager.initialize();

    // initialize editor
    m_editor.setup();

    // initialize renderer
    m_renderer.setup(m_manager);

    // print info
    std::cout << "ImGui Version: " << ImGui::GetVersion() << '\n'
              << "OpenGL Vendor: " << glGetString(GL_VENDOR) << '\n'          
              << "OpenGL Renderer: " << glGetString(GL_RENDERER) << '\n'
              << "OpenGL Version: " << glGetString(GL_VERSION) << '\n';
}

Application::~Application() {
    // clear renderer resources
    m_renderer.shutdown();

    // clear scene resources
    m_scene.destroy();

    // destroy all resources
    m_manager.destroy();

    // clear glfw resources
    shutdown();
}

void Application::setup(int width, int height, const std::string& title) {
    // global env setup
    setenv("GALLIUM_DRIVER", "d3d12", 1);
    setenv("MESA_LOADER_DRIVER_OVERRIDE", "d3d12", 1);
    setenv("MESA_D3D12_DEFAULT_ADAPTER_NAME", "RTX", 1);

    // glfw initialization
    if (!glfwInit()) { throw std::runtime_error("Application::setup: Failed to initialize GLFW library"); }
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // opengl major version 4
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5); // opengl minor version 5
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // no window decoration
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);        // allocate at least 24 depth bits for glfw swapbuffers, otherwise glenbale(GL_DEPTH_TEST) may not work
    glfwWindowHint(GLFW_STENCIL_BITS, 8);

    // glfw window creation
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) { throw std::runtime_error("Application::setup: Failed to create OpenGL context"); }

    // glfw context settings(make sure it is all set before glad initialization)
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    // glfw callbacks
    // set window pointer for later use in callback function
    glfwSetWindowUserPointer(m_window, this);
    // set callback function
    glfwSetScrollCallback(m_window, Application::scrollCallback);
    glfwSetMouseButtonCallback(m_window, Application::mouseButtonCallback);
    glfwSetWindowSizeCallback(m_window, Application::windowSizeCallback);
    glfwSetWindowPosCallback(m_window, Application::windowPosCallback);
    glfwSetCursorPosCallback(m_window, Application::cursorPosCallback);

    // glfw window size and position initialization
    glfwGetWindowSize(m_window, &m_restoredWinSize.first, &m_restoredWinSize.second);
    glfwGetWindowPos(m_window, &m_restoredWinPos.first, &m_restoredWinPos.second);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) { throw std::runtime_error("Application::setup: Failed to initialize OpenGL extensions loader"); }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(
        [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
            if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM) { std::cerr << "[OpenGL Error] " << message << std::endl; }
        },
        nullptr
    );

    // imgui initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    // initialize editor callback
    m_editor.setCallback("titlebar_minimize", [this]() {
        glfwIconifyWindow(m_window);
        return true;
    });
    m_editor.setCallback("titlebar_is_maximized", [this]() { return m_maximized; });
    m_editor.setCallback("titlebar_maximize_restore", [this]() {
        if (!m_maximized) {
            // glfwMaximizeWindow(m_window);
            glfwSetWindowPos(m_window, 0, 0);
            glfwSetWindowSize(m_window, m_maximizedWinSize.first, m_maximizedWinSize.second);
        } else {
            // glfwRestoreWindow(m_window);
            glfwSetWindowPos(m_window, m_restoredWinPos.first, m_restoredWinPos.second);
            glfwSetWindowSize(m_window, m_restoredWinSize.first, m_restoredWinSize.second);
        }
        m_maximized = !m_maximized;
        return true;
    });
    m_editor.setCallback("titlebar_close", [this]() {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        return true;
    });
}

void Application::shutdown() {
    // destroy imgui context
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // destroy glfw window
    if (m_window) { glfwDestroyWindow(m_window); }
    glfwTerminate();
}

void Application::load(const std::string& scenePath) {
    std::ifstream file{scenePath};
    if (!file.is_open()) { throw std::runtime_error("Application::load: Could not open file: " + scenePath); }
    std::stringstream buffer;
    buffer << file.rdbuf();

    m_scene.initialize(buffer.str(), m_manager);
    m_renderer.prepare(m_scene); // convert .hdr skybox into cubemap
}

void Application::run() {
    static float lastFrame  = static_cast<float>(glfwGetTime()); // in seconds

    while (!glfwWindowShouldClose(m_window)) {
        float currFrame = static_cast<float>(glfwGetTime()); // in seconds
        float deltaTime = currFrame - lastFrame;
        lastFrame       = currFrame;

        // Process input
        processInput(deltaTime);

        // Update the renderer state according to renderer setting
        m_renderer.update(m_scene, m_manager); // must be called before render, otherwise resource might be not reset or updated
        
        // Render the scene
        m_renderer.render(m_scene);

        // Get display info
        getDisplayInfo(deltaTime);

        // Draw user interface
        m_editor.draw(m_scene, m_manager, m_info);

        // Swap buffers and poll events
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Application::resize(int width, int height) {
    m_editorSetting.width  = float(width);
    m_editorSetting.height = float(height);

    m_rendererSetting.width  = float(width);
    m_rendererSetting.height = float(height);
}

void Application::processInput(float deltaTime) {
    auto& camera = m_scene.getCamera();

    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS) { camera->move(CameraMovement::UPWARD, deltaTime); }
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS) { camera->move(CameraMovement::DOWNWARD, deltaTime); }
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) { camera->move(CameraMovement::LEFT, deltaTime); }
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS) { camera->move(CameraMovement::RIGHT, deltaTime); }

    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) { camera->reset(); }
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { glfwSetWindowShouldClose(m_window, true); }
}

void Application::getDisplayInfo(float deltaTime) {
    m_info.framePerSecond = calculateFPS(deltaTime);
    m_info.deltaTime      = deltaTime;
    m_info.drawCall       = m_renderer.getDrawCall();

    return;
}

float Application::calculateFPS(float deltaTime) {
    static float timer    = 0.0f;
    static int frameCount = 0;
    static float fps      = 0.0f;

    timer += deltaTime;
    frameCount++;

    if (timer >= 0.5f) {
        float nfps = static_cast<float>(frameCount) / timer;

        if (fps == 0.0f) {
            fps = nfps;
        } else {
            fps = (fps * 0.95f + nfps * 0.05f);
        }

        timer      = 0.0f;
        frameCount = 0;
    }

    return fps;
}

void Application::scrollCallback(GLFWwindow* window, double scrollX, double scrollY) {
    if (ImGui::GetIO().WantCaptureMouse) { return; }
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) { return; }

    auto& camera = app->m_scene.getCamera();
    if (camera) { camera->zoom(static_cast<float>(scrollY)); }
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) { return; } // TODO: fix title bar dragging
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) { return; }
    if (button != GLFW_MOUSE_BUTTON_RIGHT && button != GLFW_MOUSE_BUTTON_LEFT) { return; }

    double cursorX, cursorY;
    glfwGetCursorPos(window, &cursorX, &cursorY);
    int winX, winY;
    glfwGetWindowPos(window, &winX, &winY);

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            app->m_mouseRightDragging    = true;
            app->m_rotatedCursorLocalPos = {cursorX, cursorY};
        } else if (action == GLFW_RELEASE) {
            app->m_mouseRightDragging = false;
        }
    } else { // button == GLFW_MOUSE_BUTTON_LEFT
        if (action == GLFW_PRESS) {
            // if (app->m_editor.hover(cursorX, cursorY) != "titlebar") { return; }
            app->m_mouseLeftDragging      = true;
            app->m_draggedCursorGlobalPos = {winX + cursorX, winY + cursorY};
            app->m_draggedWinPos          = {winX, winY};

            if (app->m_maximized) { // before dragging the window, restore the window if needed
                app->m_maximized = false;
                glfwSetWindowPos(window, app->m_restoredWinPos.first, app->m_restoredWinPos.second);
                glfwSetWindowSize(window, app->m_restoredWinSize.first, app->m_restoredWinSize.second);
            }
        } else if (action == GLFW_RELEASE) {
            app->m_mouseLeftDragging = false;
        }
    }
}

void Application::cursorPosCallback(GLFWwindow* window, double cursorX, double cursorY) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) { return; }
    if (!app->m_mouseRightDragging && !app->m_mouseLeftDragging) { return; }
    if (ImGui::GetIO().WantCaptureMouse) { return; }

    //================================================
    // Local cursor positions in window coordinates
    //================================================
    double lastX = app->m_rotatedCursorLocalPos.first;
    double lastY = app->m_rotatedCursorLocalPos.second;
    double currX = cursorX;
    double currY = cursorY;

    //================================================
    // Global cursor positions in screen coordinates
    //================================================
    double lastScreenX = app->m_draggedCursorGlobalPos.first;
    double lastScreenY = app->m_draggedCursorGlobalPos.second;
    double currScreenX = currX + app->m_draggedWinPos.first;
    double currScreenY = currY + app->m_draggedWinPos.second;

    if (app->m_mouseRightDragging) {
        auto& camera = app->m_scene.getCamera();
        if (camera) {
            float offsetX = static_cast<float>(currX - lastX);
            float offsetY = static_cast<float>(lastY - currY);
            camera->rotate(offsetX, offsetY);
        }
        app->m_rotatedCursorLocalPos = {currX, currY};
    }

    if (app->m_mouseLeftDragging) {
        int offsetX = static_cast<int>(currScreenX - lastScreenX);
        int offsetY = static_cast<int>(currScreenY - lastScreenY);
        glfwSetWindowPos(window, app->m_draggedWinPos.first + offsetX, app->m_draggedWinPos.second + offsetY);
    }
}

void Application::windowSizeCallback(GLFWwindow* window, int w, int h) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) { return; }

    //=======================================
    // Update application window size
    //======================================
    if (!app->m_maximized) {
        app->m_restoredWinSize = {w, h}; // update the restored window size if not maximized
    }

    //=======================================
    // Update editor layout and renderer viewport
    //======================================
    app->resize(w, h);
}

void Application::windowPosCallback(GLFWwindow* window, int x, int y) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) { return; }

    //=======================================
    // Update application window position
    //======================================
    if (!app->m_maximized) {
        app->m_restoredWinPos = {x, y}; // update the restored window position if not maximized
    }
}

} // namespace tinyglrenderer
