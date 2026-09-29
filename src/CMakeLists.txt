function(add_engine_module MODULE)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.hpp"
    )

    add_library(${MODULE} STATIC ${SOURCES})

    target_include_directories(${MODULE}
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_compile_features(${MODULE}
        PUBLIC
            cxx_std_17
    )
endfunction()

add_engine_module(core)
add_engine_module(audio)
add_engine_module(input)
add_engine_module(net)
add_engine_module(physics)
add_engine_module(render)

target_link_libraries(audio
    PUBLIC
        core
)

target_link_libraries(input
    PUBLIC
        core
)

target_link_libraries(net
    PUBLIC
        core
)

target_link_libraries(physics
    PUBLIC
        core
)

target_link_libraries(render
    PUBLIC
        core
)