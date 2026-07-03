#include "editor.hpp"

#include <string>
#include <array>
#include <format>

#include <icon/IconsFontAwesome6.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "utils.hpp"

namespace tinyglrenderer {

void Editor::setup() {
    // ==========================================
    // Load base font and icon font
    // ==========================================
    auto& io                           = ImGui::GetIO();
    static const ImWchar* glyph_ranges = io.Fonts->GetGlyphRangesDefault();
    static const ImWchar icon_ranges[] = { 0xf000, 0xf3ff, 0 }; // range of FontAwesome6

    m_fonts["text"]       = io.Fonts->AddFontFromFileTTF("../asset/fonts/DejaVuSans.ttf", m_setting.fontSize, nullptr, glyph_ranges);
    m_fonts["small_text"] = io.Fonts->AddFontFromFileTTF("../asset/fonts/DejaVuSans.ttf", m_setting.smallFontSize, nullptr, glyph_ranges);
    m_fonts["large_text"] = io.Fonts->AddFontFromFileTTF("../asset/fonts/DejaVuSans.ttf", m_setting.largeFontSize, nullptr, glyph_ranges);
    m_fonts["icon"]       = io.Fonts->AddFontFromFileTTF("../asset/fonts/fa-solid-900.ttf", m_setting.iconSize, nullptr, icon_ranges);
    m_fonts["small_icon"] = io.Fonts->AddFontFromFileTTF("../asset/fonts/fa-solid-900.ttf", m_setting.smallIconSize, nullptr, icon_ranges);
    m_fonts["large_icon"] = io.Fonts->AddFontFromFileTTF("../asset/fonts/fa-solid-900.ttf", m_setting.largeIconSize, nullptr, icon_ranges);

    io.FontDefault = m_fonts["text"];

    // ==========================================
    // Config ImGui style
    // ==========================================
    ImGuiStyle& style = ImGui::GetStyle();
    // rounding
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 2.0f;
    style.PopupRounding     = 2.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 0.0f;
    // border size
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize  = 0.0f;
    style.PopupBorderSize  = 1.0f;
    // spacing
    style.ItemSpacing      = ImVec2(6.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);

    // ==========================================
    // Config ImGui background colors
    // ==========================================
    ImVec4* colors                = style.Colors;
    colors[ImGuiCol_Text]         = ImVec4(0.88f, 0.88f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    colors[ImGuiCol_WindowBg]     = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_ChildBg]      = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_PopupBg]      = ImVec4(0.14f, 0.14f, 0.16f, 0.98f);
    colors[ImGuiCol_Border]       = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    // frame background
    colors[ImGuiCol_FrameBg]        = ImVec4(0.16f, 0.16f, 0.19f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.26f, 0.26f, 0.32f, 1.00f);
    // title background
    colors[ImGuiCol_TitleBg]          = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);
    colors[ImGuiCol_TitleBgActive]    = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.09f, 0.09f, 0.11f, 0.70f);

    // ==========================================
    // Config theme colors
    // ==========================================
    ImVec4 bg_window, bg_child, bg_frame, bg_title, text_normal;
    ImVec4 accent, hovered, active, header_select;
    switch (m_setting.currTheme) {
        case EditorTheme::PURPLE_THEME: { // 1. VSCode Dark+ (默认经典暗紫/深灰)
            bg_window     = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
            bg_child      = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
            bg_frame      = ImVec4(0.16f, 0.16f, 0.19f, 1.00f);
            bg_title      = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);
            text_normal   = ImVec4(0.88f, 0.88f, 0.92f, 1.00f);
            accent        = ImVec4(0.41f, 0.26f, 0.65f, 1.00f); // 现代紫
            hovered       = ImVec4(0.50f, 0.33f, 0.78f, 1.00f);
            active        = ImVec4(0.31f, 0.18f, 0.50f, 1.00f);
            header_select = ImVec4(0.26f, 0.20f, 0.38f, 1.00f);
        } break;
        case EditorTheme::PINK_THEME: {                         // 2. One Dark Pro (VSCode 流行榜首，优雅冷蓝/高级极客)
            bg_window     = ImVec4(0.16f, 0.17f, 0.20f, 1.00f); // 带有微弱蓝绿相间的深灰
            bg_child      = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
            bg_frame      = ImVec4(0.21f, 0.23f, 0.27f, 1.00f);
            bg_title      = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);
            text_normal   = ImVec4(0.67f, 0.72f, 0.80f, 1.00f); // 柔和的粉蓝白
            accent        = ImVec4(0.38f, 0.53f, 0.80f, 1.00f); // 标志性 OneDark 科技蓝
            hovered       = ImVec4(0.48f, 0.65f, 0.93f, 1.00f);
            active        = ImVec4(0.28f, 0.41f, 0.68f, 1.00f);
            header_select = ImVec4(0.22f, 0.28f, 0.39f, 1.00f);
        } break;
        case EditorTheme::DRACULA_THEME: {                      // 3. Dracula (吸血鬼主题，高饱和度赛博霓虹粉/绿)
            bg_window     = ImVec4(0.16f, 0.16f, 0.21f, 1.00f); // 标志性冷黛蓝底
            bg_child      = ImVec4(0.11f, 0.11f, 0.16f, 1.00f);
            bg_frame      = ImVec4(0.24f, 0.24f, 0.33f, 1.00f);
            bg_title      = ImVec4(0.13f, 0.13f, 0.18f, 1.00f);
            text_normal   = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
            accent        = ImVec4(0.74f, 0.49f, 0.86f, 1.00f); // 经典的蕾丝魅惑紫
            hovered       = ImVec4(1.00f, 0.49f, 0.77f, 1.00f); // 悬浮变身吸血鬼粉红
            active        = ImVec4(0.54f, 0.35f, 0.66f, 1.00f);
            header_select = ImVec4(0.27f, 0.26f, 0.44f, 1.00f);
        } break;
        case EditorTheme::TOKYO_THEME: {                        // 4. Tokyo Night (东京暗夜，深邃幽蓝与荧光青绿色)
            bg_window     = ImVec4(0.10f, 0.11f, 0.15f, 1.00f); // 极具质感的深邃星空底
            bg_child      = ImVec4(0.07f, 0.08f, 0.11f, 1.00f);
            bg_frame      = ImVec4(0.15f, 0.17f, 0.24f, 1.00f);
            bg_title      = ImVec4(0.09f, 0.09f, 0.13f, 1.00f);
            text_normal   = ImVec4(0.78f, 0.82f, 0.93f, 1.00f);
            accent        = ImVec4(0.26f, 0.65f, 0.93f, 1.00f); // 冰蓝色
            hovered       = ImVec4(0.44f, 0.83f, 0.81f, 1.00f); // 悬浮变成高亮荧光青
            active        = ImVec4(0.18f, 0.48f, 0.71f, 1.00f);
            header_select = ImVec4(0.18f, 0.24f, 0.35f, 1.00f);
        } break;
        default: {                                              // 5. Monokai Pro (经典老牌黑客，岩石枯灰与焦糖金黄)
            bg_window     = ImVec4(0.16f, 0.16f, 0.16f, 1.00f); // 纯正炭黑岩石质感
            bg_child      = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            bg_frame      = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
            bg_title      = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
            text_normal   = ImVec4(0.93f, 0.93f, 0.92f, 1.00f);
            accent        = ImVec4(0.93f, 0.72f, 0.38f, 1.00f); // Monokai 标志性浅焦糖金黄
            hovered       = ImVec4(0.89f, 0.41f, 0.47f, 1.00f); // 悬浮触发高级哑光红
            active        = ImVec4(0.73f, 0.55f, 0.26f, 1.00f);
            header_select = ImVec4(0.27f, 0.24f, 0.20f, 1.00f);
        } break;
    }

    // ==========================================
    // Apply layout backgrounds to ImGui
    // ==========================================
    colors[ImGuiCol_Text]         = text_normal;
    colors[ImGuiCol_TextDisabled] = ImVec4(text_normal.x * 0.5f, text_normal.y * 0.5f, text_normal.z * 0.5f, 1.00f);
    colors[ImGuiCol_WindowBg]     = bg_window;
    colors[ImGuiCol_ChildBg]      = bg_child;
    colors[ImGuiCol_PopupBg]      = ImVec4(bg_window.x + 0.03f, bg_window.y + 0.03f, bg_window.z + 0.03f, 0.98f);
    colors[ImGuiCol_Border]       = ImVec4(bg_window.x + 0.07f, bg_window.y + 0.07f, bg_window.z + 0.09f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg]        = bg_frame;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(bg_frame.x + 0.06f, bg_frame.y + 0.06f, bg_frame.z + 0.07f, 1.00f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(bg_frame.x + 0.10f, bg_frame.y + 0.10f, bg_frame.z + 0.13f, 1.00f);

    colors[ImGuiCol_TitleBg]          = bg_title;
    colors[ImGuiCol_TitleBgActive]    = bg_window;
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(bg_title.x, bg_title.y, bg_title.z, 0.70f);

    // ==========================================
    // Apply reactive components theme colors
    // ==========================================
    // button
    colors[ImGuiCol_Button]        = bg_frame;
    colors[ImGuiCol_ButtonHovered] = hovered;
    colors[ImGuiCol_ButtonActive]  = active;
    // tree node
    colors[ImGuiCol_Header]        = header_select;
    colors[ImGuiCol_HeaderHovered] = accent;
    colors[ImGuiCol_HeaderActive]  = active;
    // checkbox
    colors[ImGuiCol_CheckMark] = (m_setting.currTheme == EditorTheme::DEFAULT_THEME) ? hovered : accent;
    // slider
    colors[ImGuiCol_SliderGrab]       = ImVec4(bg_frame.x + 0.15f, bg_frame.y + 0.15f, bg_frame.z + 0.18f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = accent;
    // tabs
    colors[ImGuiCol_Tab]                = bg_title;
    colors[ImGuiCol_TabHovered]         = hovered;
    colors[ImGuiCol_TabActive]          = bg_window;
    colors[ImGuiCol_TabUnfocused]       = bg_title;
    colors[ImGuiCol_TabUnfocusedActive] = bg_child;
    // scrollbar
    colors[ImGuiCol_ScrollbarBg]          = bg_child;
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(bg_frame.x + 0.08f, bg_frame.y + 0.08f, bg_frame.z + 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(bg_frame.x + 0.16f, bg_frame.y + 0.16f, bg_frame.z + 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = accent;
}

void Editor::shutdown() {}

void Editor::draw(Scene& scene, ResourceManager& manager, const DisplayInfo& info) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    drawCustomTitleBar();
    drawResourcePanel(scene, manager);
    drawActivityBar();
    drawSideBar(scene);
    drawHUD(scene, info);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

std::string Editor::hover(float x, float y) {
    const bool sb_active  = m_setting.currSBTab != SideBarTab::SB_TAB_NONE;
    const float width     = m_setting.width;
    const float height    = m_setting.height;
    const float tb_height = m_setting.titleBarHeight;
    const float ab_width  = m_setting.activityBarWidth;
    const float ab_height = height - tb_height;
    const float sb_width  = m_setting.sideBarWidth;
    const float sb_height = ab_height;
    const float rw_height = sb_height;

    std::unordered_map<std::string, glm::vec4> name2Rects = {
        { "titlebar", { 0.0f, 0.0f, width, tb_height } },
        { "activitybar", { 0.0f, 0.0f, ab_width, ab_height } },
        { "sidebar", sb_active ? glm::vec4(0.0f, 0.0f, -1.0f, -1.0f) : glm::vec4(ab_width, 0.0f, sb_width, sb_height) },
        { "renderwindow", sb_active ? glm::vec4(ab_width, 0.0f, width - ab_width, rw_height) : glm::vec4(ab_width + sb_width, 0.0f, width - ab_width - sb_width, rw_height) }
    };

    for (auto& [name, rect] : name2Rects) { // rect.w is height
        if (rect.x <= x && x <= rect.x + rect.w && rect.y <= y && y <= rect.y + rect.w) { return name; }
    }
    return "none";
}

void Editor::drawCustomTitleBar() {
    bool maximized       = m_funcs["titlebar_is_maximized"]();
    float buttonWidth    = 45.0f; // button width of title bar is hard coded
    const int numButtons = 3;
    auto label2Callback  = std::vector<std::pair<std::string, std::string>>{
        { std::string(ICON_FA_MINUS) + std::string("##TitleBar_Minimize"),                                       "titlebar_minimize"         },
        { std::string(maximized ? ICON_FA_WINDOW_RESTORE : ICON_FA_SQUARE) + std::string("##TitleBar_Maximize"), "titlebar_maximize_restore" },
        { std::string(ICON_FA_XMARK) + std::string("##TitleBar_Close"),                                          "titlebar_close"            },
    };
    const ImVec2 titleBarPos  = ImVec2(0.0f, 0.0f);
    const ImVec2 titleBarSize = ImVec2(m_setting.width, m_setting.titleBarHeight);
    const ImVec2 buttonSize   = ImVec2(buttonWidth, m_setting.titleBarHeight);
    const ImVec2 padding      = ImVec2(m_setting.padding, 0.0f);
    const float borderSize    = m_setting.borderSize;
    const float textHeight    = ImGui::GetTextLineHeight();
    auto flags                = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

    ImGui::SetNextWindowPos(titleBarPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(titleBarSize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, borderSize);
    if (ImGui::Begin("##CustomTitleBar", nullptr, flags)) {
        ImGui::SetCursorPosY((titleBarSize.y - textHeight) * 0.5f); // set cursor position for later text draw
        ImGui::Text("TinyGLRenderer");
        ImGui::SetCursorPosY(0.0f);                                               // reset cursor y position
        ImGui::SetCursorPosX(titleBarSize.x - (buttonWidth * numButtons) - 1.0f); // set cursor position for later button draw

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);       // no rounding for title bar buttons
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // no spacing between buttons
        ImGui::PushFont(m_fonts["small_icon"]);
        for (auto& [buttonLabel, buttonCallback] : label2Callback) {
            if (buttonCallback == "titlebar_close") {
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.20f, 0.20f, 1.0f)); // when hovered on close button, turn it into a red X
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.15f, 0.15f, 1.0f));  // when clicked on close button, turn it into a darker red X
            }
            if (ImGui::Button(buttonLabel.c_str(), buttonSize)) { m_funcs[buttonCallback](); }
            if (buttonCallback == "titlebar_close") { ImGui::PopStyleColor(2); }
            ImGui::SameLine();
        }
        ImGui::PopFont();
        ImGui::PopStyleVar(2); // remove temporary rounding and spacing style
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void Editor::drawActivityBar() {
    const float borderSize = m_setting.borderSize;
    const float padding    = 0.0f;
    auto activityBarTab2IconAndTooltip = std::array<std::pair<SideBarTab, std::pair<const char*, const char*>>, 3> {{
        { SideBarTab::SB_TAB_RENDERER, { ICON_FA_GEAR, "Renderer Settings" } },
        { SideBarTab::SB_TAB_LIGHTS, { ICON_FA_LIGHTBULB, "Light Studio" } },
        { SideBarTab::SB_TAB_MODELS, { ICON_FA_CUBE, "Scene Entities" } },
    }};
    ImVec2 activityBarPos  = ImVec2(0.0f, m_setting.titleBarHeight);
    ImVec2 activityBarSize = ImVec2(m_setting.activityBarWidth, m_setting.height - m_setting.titleBarHeight);
    ImVec2 activityButtonSize = ImVec2(m_setting.activityBarWidth, m_setting.activityBarWidth);
    auto activityBarFlags  = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    ImGuiStyle& style      = ImGui::GetStyle();

    ImGui::SetNextWindowPos(activityBarPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(activityBarSize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, borderSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f)); // remove spacing between buttons
    ImGui::PushStyleColor(ImGuiCol_WindowBg, style.Colors[ImGuiCol_ChildBg]);
    if (ImGui::Begin("##ActivityBar", nullptr, activityBarFlags)) {
        for (auto [tab, icontooltip] : activityBarTab2IconAndTooltip) {
            char label[128];
            sprintf(label, "%s##SideActivityBar_%s", icontooltip.first, icontooltip.second);
            bool selected = (m_setting.currSBTab == tab);
            if (selected) { ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Header]); }
            ImGui::PushFont(m_fonts["large_icon"]); // use icon font to render icon
            if (ImGui::Button(label, activityButtonSize)) { m_setting.currSBTab = (selected) ? SideBarTab::SB_TAB_NONE : tab; }
            ImGui::PopFont(); // pop out icon font to restore default font, otherwise the tooltip will be rendered in icon font as well
            if (selected) { ImGui::PopStyleColor(); }
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", icontooltip.second); }  // show tooltip when cursor hover on the button
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

void Editor::drawSideBar(Scene& scene) {
    if (m_setting.currSBTab == SideBarTab::SB_TAB_NONE) { return; }

    auto sideBarTab2Name   = std::unordered_map<SideBarTab, const char*>{
        { SideBarTab::SB_TAB_RENDERER, "Renderer Settings##SideBarTab_Renderer" },
        { SideBarTab::SB_TAB_LIGHTS,   "Light Studio##SideBarTab_Lights"        },
        { SideBarTab::SB_TAB_MODELS,   "Scene Entities##SideBarTab_Models"      },
    };
    ImVec2 sideBarPos  = ImVec2(m_setting.activityBarWidth, m_setting.titleBarHeight);
    ImVec2 sideBarSize = ImVec2(m_setting.sideBarWidth, m_setting.height - m_setting.titleBarHeight);
    auto sideBarLabel  = sideBarTab2Name[m_setting.currSBTab];
    auto sideBarFlags  = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    ImGui::SetNextWindowPos(sideBarPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(sideBarSize, ImGuiCond_Always);
    if (ImGui::Begin(sideBarLabel, nullptr, sideBarFlags)) {
        switch (m_setting.currSBTab) {
            case SideBarTab::SB_TAB_RENDERER: {
                if (ImGui::CollapsingHeader("Pipeline Switches", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Checkbox("Deferred Rendering", &m_rendererSetting.deferred);
                    ImGui::Checkbox("Shadow Mapping", &m_rendererSetting.shadow);
                    ImGui::Checkbox("Environment IBL", &m_rendererSetting.ibl);
                    ImGui::Checkbox("Bloom Blur", &m_rendererSetting.bloom);
                    ImGui::Checkbox("Lensflare Effect", &m_rendererSetting.lensflare);
                    ImGui::Checkbox("Dirt Mask", &m_rendererSetting.dirtmask);
                    ImGui::Checkbox("Screen Space Reflection", &m_rendererSetting.ssr);
                    ImGui::Checkbox("Dual Screen Space Refraction", &m_rendererSetting.dssr);
                    ImGui::Checkbox("Screen Space Ambient Occlussion", &m_rendererSetting.ssao);
                    ImGui::Checkbox("Temporal Anti-Aliasing", &m_rendererSetting.taa);
                }
                ImGui::Separator();

                auto camera = scene.getCamera();
                if (camera && ImGui::CollapsingHeader("Active Camera Info", ImGuiTreeNodeFlags_DefaultOpen)) {
                    glm::vec3 eye    = camera->getEye();
                    glm::vec3 target = camera->getTarget();
                    float speed      = camera->getSpeed();

                    ImGui::Text("Position: (%.2f, %.2f, %.2f)", eye.x, eye.y, eye.z);
                    ImGui::Text("Target  : (%.2f, %.2f, %.2f)", target.x, target.y, target.z);
                    if (ImGui::DragFloat("Camera Speed", &speed, 0.1f, 0.1f, 50.0f, "%.1f")) {
                        camera->setSpeed(speed);
                    }
                    if (ImGui::Button("Reset Viewport Location")) {
                        camera->reset();
                    }
                }
            } break;
            case SideBarTab::SB_TAB_LIGHTS: {
                auto& lights = scene.getLights();
                if (lights.empty()) { 
                    ImGui::TextDisabled("No ambient/direct lights in scene.");
                } else {
                    for (size_t i = 0; i < lights.size(); ++i) {
                        ImGui::PushID(static_cast<int>(i)); // otherwise will cause duplicate ID error
                        
                        auto light = lights[i];
                        bool visible = light->isVisible();
                        if (ImGui::Checkbox("##light_enable", &visible)) {
                            if (visible != light->isVisible()) { light->setVisible(visible); }
                        }
                        ImGui::SameLine(); 

                        if (!visible) { 
                            ImGui::BeginDisabled();
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.5f,1.f)); 
                        }
                        std::string headerName = "Light Source #" + std::to_string(i);
                        if (ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                            glm::vec3 color = light->getColor();
                            if (ImGui::ColorEdit3("Color", glm::value_ptr(color))) {
                                light->setColor(color);
                            }
                            float intensity = light->getIntensity();
                            if (ImGui::DragFloat("Intensity", &intensity, 0.05f, 0.0f, 100.0f, "%.2f")) {
                                light->setIntensity(intensity);
                            }

                            auto dirLight = dynamic_cast<DirectionalLight*>(light.get());
                            if (dirLight) {
                                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Type: Directional");
                                glm::vec3 direction = dirLight->getDirection();
                                
                                if (ImGui::DragFloat3("Direction", glm::value_ptr(direction), 0.02f, -1.0f, 1.0f, "%.2f")) {
                                    dirLight->setDirection(direction);
                                    dirLight->setLightSpaceMatrix(scene.getBoundingBox());
                                }
                            } 

                            // TODO: Point/Spot light support
                        }
                        if (!visible) { 
                            ImGui::EndDisabled();
                            ImGui::PopStyleColor(1);
                        }

                        ImGui::Spacing();
                        ImGui::PopID();
                    }
                }
            } break;
            case SideBarTab::SB_TAB_MODELS: {
                auto& models = scene.getModels();
                if (models.empty()) { 
                    ImGui::TextDisabled("Empty world hierarchy."); 
                } else {
                    ImGui::Text("Hierarchy Tree:");
                    ImGui::BeginChild("##HierarchyTree", ImVec2(0, 120), true);
                    for (int i = 0; i < models.size(); ++i) {
                        ImGui::PushID(static_cast<int>(i)); // otherwise will cause duplicate ID error

                        bool visible = models[i]->isVisible();
                        if (ImGui::Checkbox("##model_enable", &visible)) {
                            models[i]->setVisible(visible); 
                        }
                        ImGui::SameLine(); 

                        if (!visible) { 
                            ImGui::BeginDisabled();
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.5f,1.f)); 
                        }
                        bool selected = (m_setting.currSBModelIndex == i);
                        if (ImGui::Selectable(models[i]->getName().c_str(), selected)) {
                            m_setting.currSBModelIndex = i;
                        }
                        if (!visible) { 
                            ImGui::EndDisabled();
                            ImGui::PopStyleColor(1);
                        }

                        ImGui::PopID();
                    }
                    ImGui::EndChild();
                    ImGui::Separator();

                    if (m_setting.currSBModelIndex >= 0 && m_setting.currSBModelIndex < models.size()) {
                        auto& model = models[m_setting.currSBModelIndex];
                        ImGui::Text("Inspector: %s", model->getName().c_str());                        
                        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                            glm::vec3 t = model->getTranslate();
                            glm::vec3 r = model->getRotate();
                            glm::vec3 s = model->getScale();
                            bool changed = false;
                            if (ImGui::DragFloat3("Position", glm::value_ptr(t), 0.05f)) { changed = true; }
                            if (ImGui::DragFloat3("Rotation", glm::value_ptr(r), 0.5f, -180.f, 180.f, "%.1f°")) { changed = true; }
                            if (ImGui::DragFloat3("Scale",    glm::value_ptr(s), 0.02f, 0.001f, 10.f)) { changed = true; }
                            if (changed) { model->setTransform(t, r, s);}
                        }
                        if (ImGui::CollapsingHeader("AABB Bound Info", ImGuiTreeNodeFlags_DefaultOpen)) {
                            auto& bounds = model->getBoundingBox();
                            ImGui::Text("Min: (%.2f, %.2f, %.2f)", bounds.first.x, bounds.first.y, bounds.first.z);
                            ImGui::Text("Max: (%.2f, %.2f, %.2f)", bounds.second.x, bounds.second.y, bounds.second.z);
                        }
                    }
                }
            } break;
        }
    }
    ImGui::End();
}

void Editor::drawResourcePanel(Scene& scene, ResourceManager& manager) {
    float resourcePanelPosX    = (m_setting.currSBTab == SideBarTab::SB_TAB_NONE ? 0 : m_setting.sideBarWidth) + m_setting.activityBarWidth;
    float resourcePanelPosY    = m_setting.height - m_setting.resourcePanelHeight;
    ImVec2 resourcePanelSize   = ImVec2(m_setting.width - resourcePanelPosX, m_setting.resourcePanelHeight);
    auto resourcePanelTab2Name = std::array<std::pair<ResourcePanelTab, const char*>, 3>{{
        { ResourcePanelTab::RP_TAB_MESHES,   "Loaded Meshes##ResourcePanelTab_Meshes"     },
        { ResourcePanelTab::RP_TAB_SHADERS,  "Loaded Shaders##ResourcePanelTab_SHADERS"   },
        { ResourcePanelTab::RP_TAB_TEXTURES, "Loaded Textures##ResourcePanelTab_TEXTURES" },
    }};
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

    ImGui::SetNextWindowPos(ImVec2(resourcePanelPosX, resourcePanelPosY));
    ImGui::SetNextWindowSize(resourcePanelSize);
    ImGui::Begin("##ResourcePanel", nullptr, flags);

    // =========================================================================
    // Draw resource panel tabs
    // =========================================================================
    for (auto& [tab, label] : resourcePanelTab2Name) {
        bool selected = (m_setting.currRPTab == tab);
        if (selected) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Header]); }
        if (ImGui::Button(label)) { m_setting.currRPTab = tab; }
        if (selected) { ImGui::PopStyleColor(); }
        if (tab != ResourcePanelTab::RP_TAB_TEXTURES) { ImGui::SameLine(); } // only keep the same line if the tab button is not the last one, otherwise separator will cross the tab button
    }
    ImGui::Separator();

    // =========================================================================
    // Split the resource panel into two parts: left list (width30%), middle details (width40%) and left preview (width 30%)
    // =========================================================================
    float innerPanelWidth  = ImGui::GetContentRegionAvail().x;
    float innerPanelHeight = ImGui::GetContentRegionAvail().y;

    std::vector<std::string> currRPItemNames;
    switch (m_setting.currRPTab) {
        case ResourcePanelTab::RP_TAB_MESHES: {
            manager.getAllMeshNames(currRPItemNames);
        } break;
        case ResourcePanelTab::RP_TAB_SHADERS: {
            manager.getAllShaderNames(currRPItemNames);
        } break;
        case ResourcePanelTab::RP_TAB_TEXTURES: {
            manager.getAllTextureNames(currRPItemNames);
        } break;
    }

    // =========================================================================
    // Draw left list
    // =========================================================================
    ImGui::BeginChild("##LeftList", ImVec2(innerPanelWidth * 0.2f, innerPanelHeight), true);
    for (int i = 0; i < currRPItemNames.size(); ++i) {
        bool selected = (m_setting.currRPItemIndex == i);
        if (ImGui::Selectable(currRPItemNames[i].c_str(), selected)) {
            m_setting.currRPItemIndex = i;
        }
    }
    ImGui::EndChild();

    // =========================================================================
    // Draw middle details
    // =========================================================================
    ImGui::SameLine();
    ImGui::BeginChild("##MiddleDetails", ImVec2(innerPanelWidth * 0.3f, innerPanelHeight), true);
    if (!currRPItemNames.empty() && m_setting.currRPItemIndex < currRPItemNames.size()) {
        std::string name = currRPItemNames[m_setting.currRPItemIndex];
        switch(m_setting.currRPTab) {
            case ResourcePanelTab::RP_TAB_MESHES: {
                const auto& mesh = manager.getMesh(name);

                ImGui::Text("ref count: %ld", mesh.use_count() - 1);
                ImGui::Text("submesh count: %ld", mesh->getSubMeshCount());
                ImGui::Text("vertex count: %ld", mesh->getVertexCount());
                ImGui::TextWrapped("obj file path: %s", mesh->getFilePath().c_str());
            } break;
            case ResourcePanelTab::RP_TAB_TEXTURES: {
                const auto& texture = manager.getTexture(name);

                ImGui::Text("ref count: %ld", texture.use_count() - 1);
                ImGui::Text("texture type: %s", glMacro2Str(texture->getTarget()));
                ImGui::Text("internal format: %s", glMacro2Str(texture->getInternalFormat()));
                ImGui::Text("mip levels: %d", texture->getMipLevels());
                ImGui::Text("width: %d", texture->getWidth(0));
                ImGui::Text("height: %d", texture->getHeight(0));
                ImGui::Text("depth: %d", texture->getDepth(0));
            } break;
            default: { // ResourcePanelTab::RP_TAB_SHADERS
                const auto& shader = manager.getShader(name);

                ImGui::Text("ref count: %ld", shader.use_count() - 1);
                ImGui::TextWrapped("vert file path: %s", shader->getFilePath().first.c_str());
                ImGui::TextWrapped("frag file path: %s", shader->getFilePath().second.c_str());
            }
        }
    }
    ImGui::EndChild();

    // =========================================================================
    // Draw right preview
    // =========================================================================
    ImGui::SameLine();
    ImGui::BeginChild("##RightPreview", ImVec2(innerPanelWidth * 0.5f, innerPanelHeight), true);
    if (!currRPItemNames.empty() && m_setting.currRPItemIndex < currRPItemNames.size()) {
        std::string name = currRPItemNames[m_setting.currRPItemIndex];
        switch(m_setting.currRPTab) {
            case ResourcePanelTab::RP_TAB_MESHES: {
                const auto& mesh = manager.getMesh(name);
                
                ImGui::Text("OBJ File Preview");
                ImGui::Separator();
                const auto& textContent = mesh->getSource();
                ImVec2 textRegionSize = ImGui::GetContentRegionAvail();
                ImGui::InputTextMultiline("##TextShaderPreview", const_cast<char*>(textContent.c_str()), textContent.size() + 1, textRegionSize, ImGuiInputTextFlags_ReadOnly);
            } break;
            case ResourcePanelTab::RP_TAB_SHADERS: {
                const auto& shader = manager.getShader(name);

                if (m_setting.currRPShaderIndex == 0) {
                    if (ImGui::Button("GLSL File Preview(.vert)##ToggleBtn")) m_setting.currRPShaderIndex = 1; 
                } else {
                    if (ImGui::Button("GLSL File Preview(.frag)##ToggleBtn")) m_setting.currRPShaderIndex = 0; 
                }
                ImGui::Separator();
                const auto& [vertShaderSource, fragShaderSource] = shader->getSource();
                const auto& textContent = (m_setting.currRPShaderIndex == 0 ? vertShaderSource : fragShaderSource);
                ImVec2 textRegionSize = ImGui::GetContentRegionAvail();
                ImGui::InputTextMultiline("##TextShaderPreview", const_cast<char*>(textContent.c_str()), textContent.size() + 1, textRegionSize, ImGuiInputTextFlags_ReadOnly);
            } break;
            default: { // ResourcePanelTab::RP_TAB_TEXTURES
                const auto& texture = manager.getTexture(name);

                GLuint glTexID = texture->getID(); 
                GLuint glTarget = texture->getTarget();
                ImTextureID imguiTexID = (ImTextureID)(intptr_t)glTexID;

                if (glTarget != GL_TEXTURE_2D) {
                    ImGui::Text("[ No Preview For %s ]", glMacro2Str(glTarget));
                } else {
                    GLsizei width  = (float)texture->getWidth(0);
                    GLsizei height = (float)texture->getHeight(0);
                    ImVec2 availSize = ImGui::GetContentRegionAvail(); // maximum size for display
                    float scale = std::min(availSize.x / width, availSize.y / height);
                    ImVec2 displaySize = ImVec2(width * scale, height * scale);

                    ImVec2 paddingPos = ImVec2((availSize.x - displaySize.x) * 0.5f, (availSize.y - displaySize.y) * 0.5f);
                    if (paddingPos.x > 0.0f || paddingPos.y > 0.0f) {
                        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + paddingPos.x, ImGui::GetCursorPosY() + paddingPos.y));
                    }
                    ImVec2 uv0 = ImVec2(0.0f, 1.0f); 
                    ImVec2 uv1 = ImVec2(1.0f, 0.0f);
                    ImGui::Image(imguiTexID, displaySize, uv0, uv1);
                }
            }
        }
    } else {
        ImGui::TextUnformatted("[ No Preview ]");
    }
    ImGui::EndChild();

    ImGui::End();
}

void Editor::drawHUD(Scene& scene, const DisplayInfo& info) {
    static size_t prevDrawCall = 0;
    size_t currDrawCall        = info.drawCall;
    const float padding        = m_setting.padding;
    ImVec2 hudPos              = ImVec2(m_setting.width - padding, m_setting.titleBarHeight + padding);
    ImVec2 hudPosPivot         = ImVec2(1.0f, 0.0f);
    ImGuiWindowFlags flags     = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove; // set the window unavailable to interact with the user

    ImGui::SetNextWindowPos(hudPos, ImGuiCond_Always, hudPosPivot); // set the window position to the top right corner of the screen
    ImGui::SetNextWindowBgAlpha(0.35f);                        // slightly transparent black background, not blocking 3D scene
    if (ImGui::Begin("HUD", nullptr, flags)) {
        ImGui::Text("PERFORMANCE");
        ImGui::Separator();
        ImGui::Text("FPS       : %.1f", info.framePerSecond);
        ImGui::Text("Frame Time: %.2f ms", info.deltaTime * 1000.0f);
        ImGui::Text("Draw Call: %ld draw calls", currDrawCall - prevDrawCall);

        ImGui::Spacing();
        ImGui::Text("STATE");
        ImGui::Separator();
        const auto& camera = scene.getCamera();
        glm::vec3 pos      = camera->getEye();
        glm::vec3 target   = camera->getTarget();
        glm::vec3 front    = glm::normalize(target - pos);
        ImGui::Text("Camera Position : (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        ImGui::Text("Camera FrontVec : (%.2f, %.2f, %.2f)", front.x, front.y, front.z);
        ImGui::Text("Deferred Rendering : %s", m_rendererSetting.deferred ? "On" : "Off");
        ImGui::Text("Shadow Mapping : %s", m_rendererSetting.shadow ? "On" : "Off");
        ImGui::Text("Environment IBL : %s", m_rendererSetting.ibl ? "On" : "Off");
        ImGui::Text("Bloom Blur : %s", m_rendererSetting.bloom ? "On" : "Off");
        ImGui::Text("Lensflare : %s", m_rendererSetting.lensflare ? "On" : "Off");
        ImGui::Text("Dirt Mask : %s", m_rendererSetting.dirtmask ? "On" : "Off");
        ImGui::Text("Screen Space Reflection : %s", m_rendererSetting.ssr ? "On" : "Off");
        ImGui::Text("Dual Screen Space Refraction : %s", m_rendererSetting.dssr ? "On" : "Off");
        ImGui::Text("Screen Space Ambient Occlusion : %s", m_rendererSetting.ssao ? "On" : "Off");
        ImGui::Text("Temporal Anti-Aliasing : %s", m_rendererSetting.taa ? "On" : "Off");
    }
    ImGui::End();

    prevDrawCall = currDrawCall;
}

}; // namespace tinyglrenderer
