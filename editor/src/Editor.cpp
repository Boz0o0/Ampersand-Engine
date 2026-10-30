#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <editor/Editor.hpp>
#include <filesystem>
#include <nfd.hpp>
#include <stdexcept>
#include <utility>

namespace ampersand::editor {

Editor::Editor() { this->initEditor(); }

Editor::Editor(const std::string& projectPath) {
    this->initEditor();
    this->open(projectPath);
}

Editor::~Editor() { ImGui::SFML::Shutdown(this->_window); }

void Editor::run() {
    while (this->_window.isOpen()) {
        this->pollEvents();
        ImGui::SFML::Update(this->_window, this->_clock.restart());

        this->handleShortcuts();
        this->drawMenuBar();
        drawDockspace();
        this->drawErrorPopup();

        if (this->_project != std::nullopt) {
            drawFiles();
            drawHierarchy();
            drawViewport();
            drawInspector();
        }

        this->_window.clear();
        ImGui::SFML::Render(this->_window);
        this->_window.display();
    }
}

void Editor::initEditor() {
    this->_window.create(sf::VideoMode({1280, 720}), "Ampersand Engine");
    this->_window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(this->_window)) {
        throw std::runtime_error("ImGui could not be initialised.");
    }
    // NOLINTNEXTLINE(bugprone-signed-bitwise): ImGui flags are signed
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().IniFilename = nullptr;
}

void Editor::open(const std::string& path) {
    std::optional<Project> newProject = Project::open(path);
    if (newProject == std::nullopt) {
        this->_error = "'" + path + "' is not a project: " +
                       std::string(Project::kConfigPath) +
                       " is missing or is not a JSON object.";
        return;
    }
    this->switchLayout(*newProject);
    this->_project = std::move(newProject);
    this->_window.setTitle("Ampersand Engine - " +
                           this->_project->getProjectName());
}

void Editor::switchLayout(const Project& project) {
    const bool loaded = ImGui::GetCurrentContext()->SettingsLoaded;
    if (loaded && !this->_iniPath.empty()) {
        ImGui::SaveIniSettingsToDisk(this->_iniPath.c_str());
    }
    this->_iniPath = (project.getRoot() / "imgui.ini").string();
    ImGui::GetIO().IniFilename = this->_iniPath.c_str();
    if (loaded && std::filesystem::exists(this->_iniPath)) {
        ImGui::LoadIniSettingsFromDisk(this->_iniPath.c_str());
    }
}

void Editor::pickProject() {
    NFD::Guard guard;
    NFD::UniquePath outPath;
    const std::string defaultPath =
        this->_project ? this->_project->getRoot().string() : "";

    const nfdresult_t result = NFD::PickFolder(
        outPath, defaultPath.empty() ? nullptr : defaultPath.c_str());

    ImGui::GetIO().ClearInputMouse();

    if (result == NFD_OKAY) {
        this->open(outPath.get());
    } else if (result == NFD_ERROR) {
        this->_error = NFD::GetError();
    }
}

void Editor::pollEvents() {
    while (const auto event = this->_window.pollEvent()) {
        ImGui::SFML::ProcessEvent(this->_window, *event);
        if (event->is<sf::Event::Closed>()) {
            this->_window.close();
        }
    }
}

void Editor::handleShortcuts() {
    // NOLINTNEXTLINE(bugprone-signed-bitwise): ImGui chords are signed
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O,
                        ImGuiInputFlags_RouteGlobal)) {
        this->pickProject();
    }
}

void Editor::drawErrorPopup() {
    if (this->_error.empty()) {
        return;
    }
    ImGui::OpenPopup("Error");
    if (ImGui::BeginPopupModal("Error", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(this->_error.c_str());
        if (ImGui::Button("OK")) {
            this->_error.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Editor::drawDockspace() {
    const ImGuiID dockspaceId = ImGui::GetID("AmpersandDockspace");
    const ImGuiViewport& viewport = *ImGui::GetMainViewport();

    if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);

        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport.WorkSize);

        ImGuiID leftId = 0;
        ImGuiID rightId = 0;
        ImGuiID centerId = dockspaceId;
        ImGuiID bottomId = 0;

        ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Left, 0.20F, &leftId,
                                    &centerId);
        ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, 0.25F, &rightId,
                                    &centerId);
        ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Down, 0.25F, &bottomId,
                                    &centerId);

        ImGui::DockBuilderDockWindow("Hierarchy", leftId);
        ImGui::DockBuilderDockWindow("Inspector", rightId);
        ImGui::DockBuilderDockWindow("Viewport", centerId);
        ImGui::DockBuilderDockWindow("Files", bottomId);

        ImGui::DockBuilderFinish(dockspaceId);
    }
    ImGui::DockSpaceOverViewport(dockspaceId, &viewport,
                                 ImGuiDockNodeFlags_None);
}

void Editor::drawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                this->pickProject();
            }
            if (ImGui::MenuItem("New Scene")) {
            }
            if (ImGui::MenuItem("Build", "Ctrl+B")) {
            }
            if (ImGui::MenuItem("Quit")) {
                this->_window.close();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Documentation")) {
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

}  // namespace ampersand::editor
