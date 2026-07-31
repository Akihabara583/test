cmake_minimum_required(VERSION 3.20)
project(CS2_AI_Headless LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CS2_AI_ROOT "${CMAKE_CURRENT_LIST_DIR}/../CS2-AI")

add_executable(cs2_ai
    main.cpp
    "${CS2_AI_ROOT}/src/CS2/Aimbot.cpp"
    "${CS2_AI_ROOT}/src/CS2/Button.cpp"
    "${CS2_AI_ROOT}/src/CS2/CS2AI.cpp"
    "${CS2_AI_ROOT}/src/CS2/GameInformationHandler.cpp"
    "${CS2_AI_ROOT}/src/CS2/MemoryManager.cpp"
    "${CS2_AI_ROOT}/src/CS2/MovementStrategy.cpp"
    "${CS2_AI_ROOT}/src/CS2/Offsets.cpp"
    "${CS2_AI_ROOT}/src/CS2/Triggerbot.cpp"
    "${CS2_AI_ROOT}/src/Utility/Dijkstra.cpp"
    "${CS2_AI_ROOT}/src/Utility/Logging.cpp"
    "${CS2_AI_ROOT}/src/Utility/Utility.cpp"
)

target_include_directories(cs2_ai PRIVATE "${CS2_AI_ROOT}/include")
target_compile_definitions(cs2_ai PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(cs2_ai PRIVATE Psapi)

if(MSVC)
    target_compile_options(cs2_ai PRIVATE /W4 /permissive- /MP)
    set_property(TARGET cs2_ai PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_custom_command(TARGET cs2_ai POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_LIST_DIR}/../Configuration"
        "$<TARGET_FILE_DIR:cs2_ai>/Configuration"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_LIST_DIR}/../Navmesh"
        "$<TARGET_FILE_DIR:cs2_ai>/Navmesh"
)
