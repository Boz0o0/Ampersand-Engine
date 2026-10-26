function(add_engine_module MODULE)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.hpp"
    )

    add_library(ampersand_${MODULE} STATIC ${SOURCES})

    target_include_directories(ampersand_${MODULE}
        PUBLIC
            "${PROJECT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_compile_features(ampersand_${MODULE}
        PUBLIC
            cxx_std_20
    )
endfunction()

if(NOT AMPERSAND_MODULE_CORE)
     if(AMPERSAND_MODULE_AUDIO OR AMPERSAND_MODULE_INPUT OR AMPERSAND_MODULE_NET
        OR AMPERSAND_MODULE_PHYSICS OR AMPERSAND_MODULE_RENDER OR AMPERSAND_MODULE_SCENE OR AMPERSAND_MODULE_SCRIPTING)
         message(FATAL_ERROR "AMPERSAND_MODULE_CORE must be enabled when another engine module is enabled")
     endif()
endif()

if(AMPERSAND_MODULE_CORE)
    add_engine_module(core)
endif()

if(AMPERSAND_MODULE_AUDIO)
    add_engine_module(audio)

    target_link_libraries(ampersand_audio
        PUBLIC
            ampersand_core
            SFML::Audio
    )
endif()

if(AMPERSAND_MODULE_INPUT)
    add_engine_module(input)

    target_link_libraries(ampersand_input
        PUBLIC
            ampersand_core
    )
endif()

if(AMPERSAND_MODULE_NET)
    add_engine_module(net)

    target_link_libraries(ampersand_net
        PUBLIC
            ampersand_core
        PRIVATE
            asio
    )
endif()

if(AMPERSAND_MODULE_PHYSICS)
    add_engine_module(physics)

    target_link_libraries(ampersand_physics
        PUBLIC
            ampersand_core
    )
endif()

if(AMPERSAND_MODULE_RENDER)
    if(NOT AMPERSAND_MODULE_INPUT)
        message(FATAL_ERROR "AMPERSAND_MODULE_INPUT must be enabled when the render module is enabled")
    endif()

    add_engine_module(render)

    target_link_libraries(ampersand_render
        PUBLIC
            ampersand_core
            ampersand_input
            SFML::Graphics
            SFML::Window
            SFML::System
    )
endif()

if (AMPERSAND_MODULE_SCRIPTING)
    add_engine_module(scripting)

    target_link_libraries(ampersand_scripting
        PUBLIC
            ampersand_core
            sol2::sol2
            lua_static
    )

    target_compile_definitions(ampersand_scripting PUBLIC SOL_ALL_SAFETIES_ON=1)

    if(MSVC)
        target_compile_options(ampersand_scripting PRIVATE /bigobj)
    endif()

    if (AMPERSAND_MODULE_INPUT)
        target_link_libraries(ampersand_scripting PRIVATE ampersand_input)
        target_compile_definitions(ampersand_scripting PRIVATE AMPERSAND_MODULE_INPUT)
    endif()
endif()

if (AMPERSAND_MODULE_SCENE)
    add_engine_module(scene)

    target_link_libraries(ampersand_scene
        PUBLIC
            ampersand_core
            nlohmann_json::nlohmann_json
    )

    if (AMPERSAND_MODULE_RENDER)
        target_link_libraries(ampersand_scene PUBLIC ampersand_render)
        target_compile_definitions(ampersand_scene PUBLIC AMPERSAND_MODULE_RENDER)
    endif()

    if (AMPERSAND_MODULE_PHYSICS)
        target_link_libraries(ampersand_scene PUBLIC ampersand_physics)
        target_compile_definitions(ampersand_scene PUBLIC AMPERSAND_MODULE_PHYSICS)
    endif()

    if (AMPERSAND_MODULE_AUDIO)
        target_link_libraries(ampersand_scene PUBLIC ampersand_audio)
        target_compile_definitions(ampersand_scene PUBLIC AMPERSAND_MODULE_AUDIO)
    endif()

    if (AMPERSAND_MODULE_INPUT)
        target_link_libraries(ampersand_scene PUBLIC ampersand_input)
        target_compile_definitions(ampersand_scene PUBLIC AMPERSAND_MODULE_INPUT)
    endif()

    if (AMPERSAND_MODULE_SCRIPTING)
        target_link_libraries(ampersand_scene PUBLIC ampersand_scripting)
        target_compile_definitions(ampersand_scene PUBLIC AMPERSAND_MODULE_SCRIPTING)
    endif()
endif()
