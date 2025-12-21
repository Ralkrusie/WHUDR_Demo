cmake_minimum_required(VERSION 3.20)
project(WHUDR LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# SFML 路径
set(SFML_DIR "${CMAKE_SOURCE_DIR}/external/SFML/SFML-3.0.2/lib/cmake/SFML")

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

# 递归收集 src 下所有 cpp
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

add_executable(WHUDR ${SRC_FILES})

# 确保新房间源文件被显式加入（即使旧生成目录未重新配置）
target_sources(WHUDR PRIVATE
    src/Overworld/AlphysClass.cpp
    src/Overworld/SecretRoom.cpp
    src/Battle/BattleActor.cpp
)

# 包含 src 头文件路径
target_include_directories(WHUDR PRIVATE src)

# 链接 SFML
target_link_libraries(WHUDR PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)
