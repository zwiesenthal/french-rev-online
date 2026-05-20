cmake_minimum_required(VERSION 3.16)
project(french_rev_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(french_rev_engine
  src/engine.cpp
  src/generated_data.cpp
)
target_include_directories(french_rev_engine PUBLIC src)

add_executable(french_rev_server src/main.cpp)
target_link_libraries(french_rev_server PRIVATE french_rev_engine)

add_executable(french_rev_sim src/simulate.cpp)
target_link_libraries(french_rev_sim PRIVATE french_rev_engine)
