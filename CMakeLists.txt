cmake_minimum_required(VERSION 3.20)
project(agent_avenue_ai_lab VERSION 3.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/utf-8)
endif()

find_package(Threads REQUIRED)

add_executable(core_checks tests/core_checks.cpp)
add_executable(fast_checks tests/fast_checks.cpp)
add_executable(information_checks tests/information_checks.cpp)
add_executable(v3_model_checks tests/v3_model_checks.cpp)
target_link_libraries(v3_model_checks PRIVATE Threads::Threads)
add_executable(v3_cuda_checks tests/v3_cuda_checks.cpp)
target_link_libraries(v3_cuda_checks PRIVATE Threads::Threads)
add_executable(v3_benchmark tests/v3_benchmark.cpp)
add_executable(v3_train src/v3_train_main.cpp)
target_link_libraries(v3_train PRIVATE Threads::Threads)
target_link_libraries(fast_checks PRIVATE Threads::Threads)

enable_testing()
add_test(NAME core_checks COMMAND core_checks --test)
add_test(NAME fast_checks COMMAND fast_checks)
add_test(NAME information_checks COMMAND information_checks)
add_test(NAME v3_model_checks COMMAND v3_model_checks)
add_test(NAME v3_cuda_checks COMMAND v3_cuda_checks)

if(WIN32)
  add_executable(agent_avenue_ai_lab WIN32 src/windows.cpp)
  target_compile_definitions(agent_avenue_ai_lab PRIVATE UNICODE _UNICODE)
  target_link_libraries(agent_avenue_ai_lab PRIVATE gdi32 user32 shell32 gdiplus Threads::Threads)
  add_custom_command(TARGET agent_avenue_ai_lab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets/cards"
            "$<TARGET_FILE_DIR:agent_avenue_ai_lab>/assets/cards"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/models/pretrained-17m.bin"
            "$<TARGET_FILE_DIR:agent_avenue_ai_lab>/training.bin"
    COMMENT "Copying original card artwork")
  if(MSVC)
    target_compile_options(agent_avenue_ai_lab PRIVATE /utf-8 /O2)
    target_link_options(agent_avenue_ai_lab PRIVATE /STACK:8388608)
  else()
    target_compile_options(agent_avenue_ai_lab PRIVATE -O2 -municode)
    target_link_options(agent_avenue_ai_lab PRIVATE -static -municode -Wl,--stack,8388608)
  endif()
endif()

add_executable(ui_checks tests/ui_checks.cpp)
target_link_libraries(ui_checks PRIVATE Threads::Threads)
add_test(NAME ui_checks COMMAND ui_checks)
