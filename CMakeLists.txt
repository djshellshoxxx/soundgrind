cmake_minimum_required(VERSION 3.24)
project(SoundGrind VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_TESTING ON CACHE BOOL "Build tests" FORCE)

# Exact upstream commits: CLAP 1.2.9 and nlohmann/json 3.12.0.
FetchContent_Declare(clap
  GIT_REPOSITORY https://github.com/free-audio/clap.git
  GIT_TAG cd94482ba5941ae410809b6fbaed3bc851044270
  GIT_SHALLOW FALSE)
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG 65ee68451d8eb2b5f3a30b410476ab83deb3289b
  GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(clap nlohmann_json)

add_library(soundgrind_engine
  src/campaign.cpp
  src/config.cpp
  src/evidence.cpp
  src/hash.cpp
  src/module_registry.cpp
  src/process.cpp
  src/report.cpp)
target_include_directories(soundgrind_engine PUBLIC include)
target_link_libraries(soundgrind_engine PUBLIC nlohmann_json::nlohmann_json)
if(WIN32)
  # Windows process supervision uses the platform process API.
else()
  target_link_libraries(soundgrind_engine PRIVATE ${CMAKE_DL_LIBS})
endif()

add_library(soundgrind_clap_adapter src/clap_adapter.cpp)
target_include_directories(soundgrind_clap_adapter PUBLIC include)
target_link_libraries(soundgrind_clap_adapter PUBLIC soundgrind_engine clap)

add_executable(soundgrind_worker src/worker_main.cpp)
target_link_libraries(soundgrind_worker PRIVATE soundgrind_clap_adapter)

add_executable(soundgrind src/main.cpp)
target_link_libraries(soundgrind PRIVATE soundgrind_engine)

add_library(soundgrind_fixture MODULE fixtures/synthetic_clap.cpp)
target_link_libraries(soundgrind_fixture PRIVATE clap)
set_target_properties(soundgrind_fixture PROPERTIES PREFIX "" SUFFIX ".clap")
if(APPLE)
  set_target_properties(soundgrind_fixture PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION "clap")
endif()

include(CTest)
if(BUILD_TESTING)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_executable(soundgrind_tests tests/engine_tests.cpp)
  target_link_libraries(soundgrind_tests PRIVATE soundgrind_engine)
  add_test(NAME soundgrind_engine_tests COMMAND soundgrind_tests)
  add_test(NAME soundgrind_fixture_smoke
    COMMAND soundgrind test --target $<TARGET_FILE:soundgrind_fixture>
      --worker $<TARGET_FILE:soundgrind_worker> --out ${CMAKE_CURRENT_BINARY_DIR}/fixture-smoke
      --profile level1 --seed 123)
  set_tests_properties(soundgrind_fixture_smoke PROPERTIES TIMEOUT 60)
  add_test(NAME soundgrind_cli_integration
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_checks.py
      $<TARGET_FILE:soundgrind> $<TARGET_FILE:soundgrind_worker>
      $<TARGET_FILE:soundgrind_fixture> ${CMAKE_CURRENT_BINARY_DIR}/integration-checks)
  set_tests_properties(soundgrind_cli_integration PROPERTIES TIMEOUT 90)
  foreach(pair IN ITEMS "nan;non_finite_output" "crash;worker_crash" "hang;hang_or_timeout" "oracle;fixture_oracle_mismatch")
    list(GET pair 0 fault)
    list(GET pair 1 expected)
    add_test(NAME soundgrind_fixture_${fault}
      COMMAND ${CMAKE_COMMAND}
        -DCLI=$<TARGET_FILE:soundgrind>
        -DWORKER=$<TARGET_FILE:soundgrind_worker>
        -DTARGET=$<TARGET_FILE:soundgrind_fixture>
        -DOUT=${CMAKE_CURRENT_BINARY_DIR}/fixture-${fault}
        -DFAULT=${fault}
        -DEXPECTED=${expected}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_fixture_failure.cmake)
    set_tests_properties(soundgrind_fixture_${fault} PROPERTIES TIMEOUT 60)
  endforeach()
endif()
