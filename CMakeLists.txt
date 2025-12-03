cmake_minimum_required(VERSION 3.27)
project(snake-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Выбираем, какие модули snake-shared включать
set(VSHARED_ENABLE_LOGGING ON  CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_NET     OFF CACHE BOOL "" FORCE)
# set(VSHARED_ENABLE_DB   ON  CACHE BOOL "" FORCE)

add_subdirectory(snake-shared)

file(GLOB_RECURSE SNAKE_SERVER_SOURCES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

add_executable(snake-server
        ${SNAKE_SERVER_SOURCES}
)

# ОДНА строка – и все включённые модули автоматически прилетают
target_link_libraries(snake-server
        PRIVATE
        snake-shared::all
)

target_compile_features(snake-server PUBLIC cxx_std_23)