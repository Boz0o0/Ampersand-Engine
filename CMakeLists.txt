cmake_minimum_required(VERSION 3.24)

project(AmpersandEngine
    VERSION 0.5.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(AMPERSAND_MODULE_CORE "Build the Core module" ON)
option(AMPERSAND_MODULE_AUDIO "Build the Audio module" ON)
option(AMPERSAND_MODULE_INPUT "Build the Input module" ON)
option(AMPERSAND_MODULE_NET "Build the Network module" ON)
option(AMPERSAND_MODULE_PHYSICS "Build the Physics module" ON)
option(AMPERSAND_MODULE_RENDER "Build the Render module" ON)
option(AMPERSAND_MODULE_SCENE "Build the Scene module" ON)
option(AMPERSAND_MODULE_SCRIPTING "Build the Scripting module" ON)

option(AMPERSAND_BUILD_TESTS "Build the engine unit tests" ${PROJECT_IS_TOP_LEVEL})

include(cmake/CPM.cmake)

if(AMPERSAND_BUILD_TESTS)
    include(CTest)
endif()

if(AMPERSAND_BUILD_TESTS AND BUILD_TESTING)
    CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.15.2
        OPTIONS
            "INSTALL_GTEST OFF"
            "gtest_force_shared_crt ON"
    )
endif()

if (AMPERSAND_MODULE_RENDER OR AMPERSAND_MODULE_AUDIO)
    CPMAddPackage(
        NAME SFML
        GITHUB_REPOSITORY SFML/SFML
        GIT_TAG 3.0.2
        OPTIONS
            "SFML_BUILD_EXAMPLES OFF"
            "SFML_BUILD_TEST_SUITE OFF"
            "SFML_USE_SYSTEM_DEPS OFF"
    )
endif()

if (AMPERSAND_MODULE_NET AND NOT TARGET asio)
    CPMAddPackage(
        NAME asio
        GITHUB_REPOSITORY chriskohlhoff/asio
        GIT_TAG asio-1-30-2
        DOWNLOAD_ONLY YES
    )

    find_package(Threads REQUIRED)

    add_library(asio STATIC "${asio_SOURCE_DIR}/asio/src/asio.cpp")
    target_include_directories(asio SYSTEM PUBLIC "${asio_SOURCE_DIR}/asio/include")
    target_compile_definitions(asio PUBLIC ASIO_STANDALONE ASIO_SEPARATE_COMPILATION ASIO_NO_DEPRECATED)
    target_link_libraries(asio PUBLIC Threads::Threads)

    if(WIN32)
        target_compile_definitions(asio PUBLIC _WIN32_WINNT=0x0A00)
        target_link_libraries(asio PUBLIC ws2_32 mswsock)
    endif()
endif()

if (AMPERSAND_MODULE_SCENE)
    CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        GIT_TAG v3.12.0
    )
endif()

if (AMPERSAND_MODULE_SCRIPTING)
    enable_language(C)

    CPMAddPackage(
        NAME lua
        GITHUB_REPOSITORY lua/lua
        GIT_TAG v5.4.7
        DOWNLOAD_ONLY YES
    )

    file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/*.c")
    list(REMOVE_ITEM LUA_SOURCES
        "${lua_SOURCE_DIR}/lua.c"
        "${lua_SOURCE_DIR}/luac.c"
        "${lua_SOURCE_DIR}/onelua.c"
        "${lua_SOURCE_DIR}/ltests.c"
    )

    add_library(lua_static STATIC ${LUA_SOURCES})
    target_include_directories(lua_static SYSTEM PUBLIC "${lua_SOURCE_DIR}")

    if(UNIX)
        target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX)
        target_link_libraries(lua_static PUBLIC m)
    endif()

    CPMAddPackage(
        NAME sol2
        GITHUB_REPOSITORY ThePhD/sol2
        GIT_TAG v3.5.0
    )
endif()

add_subdirectory(src)

if(AMPERSAND_BUILD_TESTS AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
