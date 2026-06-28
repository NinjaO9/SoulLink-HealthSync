cmake_minimum_required(VERSION 3.20)
project(SoulLink-HealthSync)

set(CMAKE_CXX_STANDARD 17)

add_executable(SoulLink-HealthSync
    src/main.cpp
    src/gen3_utils.cpp
    src/game_registry.cpp
)

target_include_directories(SoulLink-HealthSync PRIVATE src)

target_link_libraries(SoulLink-HealthSync PRIVATE psapi)