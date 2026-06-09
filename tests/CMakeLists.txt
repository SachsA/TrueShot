cmake_minimum_required(VERSION 3.16)

# Unit tests for the netcode invariants — Phase 1.10.
# Built only when TRUESHOT_BUILD_TESTS is ON (default in the strict preset
# and in CI). Linked against trueshot_network so it can exercise the
# Server-side library directly without needing a live socket.

find_package(GTest CONFIG REQUIRED)

add_executable(trueshot_tests
    test_netsim.cpp
    test_bitstream.cpp
    test_player_history.cpp
    test_lag_compensation.cpp
    test_client_prediction.cpp
)

target_link_libraries(trueshot_tests
    PRIVATE
        trueshot_network
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(trueshot_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/network_module/include
)

# We also need the client-side `net/client_prediction.cpp` and
# `net/remote_player.cpp` to link the prediction tests. Compile them
# directly into the test binary — they're small and we don't want to
# extract a separate library yet.
target_sources(trueshot_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/client_prediction.cpp
        ${CMAKE_SOURCE_DIR}/src/remote_player.cpp
)

# GoogleTest helper that registers each TEST as a ctest entry.
include(GoogleTest)
gtest_discover_tests(trueshot_tests)
