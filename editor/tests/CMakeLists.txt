include(GoogleTest)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
add_executable(test_editor
    ${SOURCES}
)
target_link_libraries(test_editor
    PRIVATE
        GTest::gtest_main
        ampersand_editor
)
gtest_discover_tests(test_editor)