#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <editor/Editor.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
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
    while (this->_running) {
        this->pollEvents();
        ImGui::SFML::Update(this->_window, this->_clock.restart());

        this->handleShortcuts();
        this->drawMenuBar();
        drawDockspace();
        this->drawErrorPopup();
        this->drawInfoPopup();
        this->drawUnsavedModal();
        drawNewProjectModal();

        if (this->_project != std::nullopt) {
            drawNewSceneModal();
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

void Editor::create(const std::string& path, const std::string& name) {
    if (!Project::isValidName(name)) {
        this->_error = "Invalid project name '" + name + "'.";
        return;
    }
    if (std::filesystem::exists(path + "/" + name) &&
        !std::filesystem::is_empty(path + "/" + name) &&
        std::filesystem::is_directory(path + "/" + name)) {
        this->_error =
            "Project '" + name + "' already exists at '" + path + "'.";
        return;
    }
    std::optional<Project> newProject = Project::create(path, name);
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

void Editor::openScene(const std::filesystem::path& relative) {
    if (!this->_project) {
        return;
    }
    if (this->_project->openScene(relative)) {
        this->_selection.reset();
    } else {
        this->_error = "'" + relative.generic_string() +
                       "' is not a scene: it needs \"version\": 2 and an "
                       "\"entities\" array, and must not be a prefab.";
    }
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

void Editor::pickProject(bool isCreating) {
    NFD::Guard guard;
    NFD::UniquePath outPath;
    const std::string defaultPath =
        this->_project ? this->_project->getRoot().string() : "";

    const nfdresult_t result = NFD::PickFolder(
        outPath, defaultPath.empty() ? nullptr : defaultPath.c_str());

    ImGui::GetIO().ClearInputMouse();

    if (result == NFD_OKAY) {
        if (isCreating) {
            this->_newProjectPath = outPath.get();
        } else {
            this->confirmUnsaved(this->_project && this->_project->isDirty(),
                                 [this, path = std::string(outPath.get())] {
                                     this->open(path);
                                 });
        }
    } else if (result == NFD_ERROR) {
        this->_error = NFD::GetError();
    }
}

void Editor::createNewScene(const std::string& name) {
    if (this->_project == std::nullopt) {
        return;
    }
    if (auto error = this->_project->createScene(name)) {
        this->_error = std::move(*error);
    }
}

bool Editor::buildProject() {
    if (this->_project == std::nullopt) {
        return true;
    }

    std::optional<std::string> error = this->_project->build();
    if (error) {
        this->_error = *error;
    } else {
        this->_info = "Project built successfully.";
    }
    this->_project->refreshFiles();
    return error == std::nullopt;
}

void Editor::pollEvents() {
    while (const auto event = this->_window.pollEvent()) {
        ImGui::SFML::ProcessEvent(this->_window, *event);
        if (event->is<sf::Event::Closed>()) {
            this->confirmUnsaved(this->_project && this->_project->isDirty(),
                                 [this] { this->_running = false; });
        }
    }
}

void Editor::handleShortcuts() {
    // NOLINTNEXTLINE(bugprone-signed-bitwise): ImGui chords are signed
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O,
                        ImGuiInputFlags_RouteGlobal)) {
        this->pickProject();
    }
    // NOLINTNEXTLINE(bugprone-signed-bitwise): ImGui chords are signed
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N,
                        ImGuiInputFlags_RouteGlobal)) {
        this->_newSceneModal = true;
    }
    // NOLINTNEXTLINE(bugprone-signed-bitwise): ImGui chords are signed
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_B,
                        ImGuiInputFlags_RouteGlobal)) {
        this->buildProject();
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

void Editor::drawInfoPopup() {
    if (this->_info.empty()) {
        return;
    }
    ImGui::OpenPopup("Info");
    if (ImGui::BeginPopupModal("Info", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(this->_info.c_str());
        if (ImGui::Button("OK")) {
            this->_info.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Editor::confirmUnsaved(bool dirty, std::function<void()> action) {
    if (dirty) {
        this->_pendingAction = std::move(action);
    } else {
        action();
    }
}

void Editor::drawUnsavedModal() {
    if (!this->_pendingAction) {
        return;
    }
    ImGui::OpenPopup("Unsaved changes");
    if (!ImGui::BeginPopupModal("Unsaved changes", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    ImGui::TextUnformatted("Build the unsaved changes before continuing?");
    std::function<void()> action;
    if (ImGui::Button("Build")) {
        if (this->buildProject()) {
            action = std::move(this->_pendingAction);
        }
        this->_pendingAction = nullptr;
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard")) {
        action = std::move(this->_pendingAction);
        this->_pendingAction = nullptr;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        this->_pendingAction = nullptr;
    }
    if (!this->_pendingAction) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
    if (action) {
        action();
    }
}

void Editor::drawNewSceneModal() {
    if (!this->_newSceneModal) {
        return;
    }
    ImGui::OpenPopup("New Scene");
    if (ImGui::BeginPopupModal("New Scene", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter the name of the new scene:");

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        bool submitted = ImGui::InputText(".json", this->_textBuffer.data(),
                                          this->_textBuffer.size(),
                                          ImGuiInputTextFlags_EnterReturnsTrue);
        bool isEmpty = this->_textBuffer[0] == '\0';

        ImGui::BeginDisabled(isEmpty);
        if ((ImGui::Button("Create") || submitted) && !isEmpty) {
            std::string name = this->_textBuffer.data();
            this->createNewScene(name);
            this->_textBuffer.fill('\0');
            this->_newSceneModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            this->_textBuffer.fill('\0');
            this->_newSceneModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Editor::drawNewProjectModal() {
    if (this->_newProjectPath.empty()) {
        return;
    }
    ImGui::OpenPopup("New Project");
    if (ImGui::BeginPopupModal("New Project", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter the name of the new project:");

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        bool submitted = ImGui::InputText(
            "##projectName", this->_textBuffer.data(), this->_textBuffer.size(),
            ImGuiInputTextFlags_EnterReturnsTrue);
        bool isEmpty = this->_textBuffer[0] == '\0';

        ImGui::BeginDisabled(isEmpty);
        if ((ImGui::Button("Create") || submitted) && !isEmpty) {
            this->confirmUnsaved(
                this->_project && this->_project->isDirty(),
                [this, path = this->_newProjectPath,
                 name = std::string(this->_textBuffer.data())] {
                    this->create(path, name);
                });
            this->_textBuffer.fill('\0');
            this->_newProjectPath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            this->_textBuffer.fill('\0');
            this->_newProjectPath.clear();
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

        ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, 0.25F, &rightId,
                                    &centerId);
        ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Left, 0.20F, &leftId,
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
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    this->drawFileMenu();
    this->drawHelpMenu();
    ImGui::EndMainMenuBar();
}

void Editor::drawFileMenu() {
    if (!ImGui::BeginMenu("File")) {
        return;
    }
    if (ImGui::MenuItem("Open", "Ctrl+O")) {
        this->pickProject(false);
    }
    if (ImGui::MenuItem("Create project")) {
        this->pickProject(true);
    }
    if (ImGui::MenuItem("New Scene", "Ctrl+N", false,
                        this->_project.has_value())) {
        this->_newSceneModal = true;
    }
    if (ImGui::MenuItem("Build", "Ctrl+B", false, this->_project.has_value())) {
        this->buildProject();
    }
    if (ImGui::MenuItem("Quit")) {
        this->confirmUnsaved(this->_project && this->_project->isDirty(),
                             [this] { this->_running = false; });
    }
    ImGui::EndMenu();
}

void Editor::drawHelpMenu() {
    if (!ImGui::BeginMenu("Help")) {
        return;
    }
    if (ImGui::MenuItem("Documentation")) {
        this->_info =
            "Visit the Ampersand Engine documentation at : "
            "https://boz0o0.github.io/Ampersand-Engine/";
    }
    ImGui::EndMenu();
}

}  // namespace ampersand::editor
