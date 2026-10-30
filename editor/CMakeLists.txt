CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG v1.91.9b-docking
    DOWNLOAD_ONLY YES
)

CPMAddPackage(
    NAME imgui-sfml
    GITHUB_REPOSITORY SFML/imgui-sfml
    GIT_TAG v3.0
    OPTIONS
        "IMGUI_DIR ${imgui_SOURCE_DIR}"
        "IMGUI_SFML_FIND_SFML OFF"
)

CPMAddPackage(
    NAME nfd
    GITHUB_REPOSITORY btzy/nativefiledialog-extended
    GIT_TAG v1.4.1
)

file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
list(REMOVE_ITEM EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(ampersand_editor STATIC ${EDITOR_SOURCES})

target_include_directories(ampersand_editor
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(ampersand_editor
    PUBLIC
        cxx_std_20
)

target_link_libraries(ampersand_editor
    PUBLIC
        ampersand_scene
        ImGui-SFML::ImGui-SFML
    PRIVATE
        nfd
)

add_executable(ampersand_editor_app src/main.cpp)

set_target_properties(ampersand_editor_app
    PROPERTIES
        OUTPUT_NAME ampersand_editor
)

target_link_libraries(ampersand_editor_app
    PRIVATE
        ampersand_editor
)
