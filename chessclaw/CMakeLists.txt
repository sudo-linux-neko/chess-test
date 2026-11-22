cmake_minimum_required(VERSION 3.20)
project(chessclaw VERSION 1.0.0 LANGUAGES CXX)
set(PROJECT_AUTHOR "vaubhav")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHESSCLAW_ENABLE_NATIVE "Use -march=native (default ON, OFF for portable builds)" ON)
option(CHESSCLAW_ENABLE_AVX2 "Enable AVX2 code paths" ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Max-speed: aggressive but honest flags for Alder Lake i5-1335U.
  # -O3 + native AVX2/VNNI + LTO + unroll + fast-math-lite (no associative
  # breakage for search; NNUE float trainer benefits). PGO documented in README.
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG -funroll-loops -fomit-frame-pointer)
    # add_link_options(-flto)  # disabled: LTO extern-const issues
    # LTO re-enabled (bitboard magic tables are plain non-const arrays now).
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _cc_ipo OUTPUT _cc_ipo_msg)
    if(_cc_ipo)
      set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
    if(CHESSCLAW_ENABLE_NATIVE)
      add_compile_options(-march=native -mtune=native)
    else()
      add_compile_options(-mtune=generic)
    endif()
    if(CHESSCLAW_ENABLE_AVX2)
      add_compile_definitions(CHESSCLAW_HAS_AVX2=1)
    endif()
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -fno-omit-frame-pointer)
  endif()
elseif(MSVC)
  add_compile_options(/W4 /O2 /std:c++20)
  add_compile_definitions(CHESSCLAW_HAS_AVX2=1 NOMINMAX)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CHESSCLAW_CORE_SRCS
  src/bitboard.cpp
  src/position.cpp
  src/movegen.cpp
  src/evaluate.cpp
  src/nnue.cpp
  src/transposition.cpp
  src/search.cpp
  src/time_manager.cpp
  src/uci.cpp
  src/benchmark.cpp
  src/storage_check.cpp
  src/ui/terminal.cpp
  src/ui/double_buffer.cpp
  src/ui/board_renderer.cpp
  src/ui/animator.cpp
  src/ui/charts.cpp
  src/ui/progress_bar.cpp
  src/ui/dashboard.cpp
  src/ui/transitions.cpp
  src/ui/training_dashboard.cpp
  src/ui/anime_bg.cpp
  src/train/data_gen.cpp
  src/train/trainer.cpp
  src/train/elo_estimator.cpp
)

add_library(chessclaw_core STATIC ${CHESSCLAW_CORE_SRCS})
target_include_directories(chessclaw_core PUBLIC include)
if(UNIX)
  target_link_libraries(chessclaw_core PUBLIC pthread)
endif()

add_executable(chessclaw src/app/main.cpp)
target_link_libraries(chessclaw PRIVATE chessclaw_core)

add_executable(chessclaw-perft tests/test_perft.cpp)
target_link_libraries(chessclaw-perft PRIVATE chessclaw_core)

enable_testing()
add_test(NAME perft_startpos COMMAND chessclaw-perft)
