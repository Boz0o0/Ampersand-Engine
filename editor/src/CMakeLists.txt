file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/editor/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(REMOVE_ITEM EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(ampersand_editor STATIC ${EDITOR_SOURCES})

target_include_directories(ampersand_editor
    PUBLIC
        "${PROJECT_SOURCE_DIR}/editor/include"
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

add_executable(ampersand_editor_app main.cpp)

set_target_properties(ampersand_editor_app
    PROPERTIES
        OUTPUT_NAME ampersand_editor
)

target_link_libraries(ampersand_editor_app
    PRIVATE
        ampersand_editor
)
