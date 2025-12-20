cmake_minimum_required(VERSION 3.27)
project(snake-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(BOOST_ASIO_NO_DEPRECATED)
set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-deprecated-declarations"
)

# ===============================
# Platform-specific setup
# ===============================
if (APPLE)
    set(CMAKE_FIND_FRAMEWORK FIRST)

    list(PREPEND CMAKE_PREFIX_PATH
            "/opt/homebrew"
            "/usr/local"
    )
else()
    # Windows / MinGW
    set(CMAKE_FIND_FRAMEWORK NEVER)

    # Fix "too many sections / file too big" on MinGW (Boost/Beast etc.)
    add_compile_options(-Wa,-mbig-obj)

    # MSYS2 MinGW64 deps (OpenSSL + Boost)
    list(PREPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
    set(OPENSSL_ROOT_DIR "C:/msys64/mingw64" CACHE PATH "" FORCE)
    set(BOOST_ROOT       "C:/msys64/mingw64" CACHE PATH "" FORCE)
endif()

set(BUILD_SHARED_LIBS OFF)

# ===============================
# snake-shared options
# ===============================
set(VSHARED_ENABLE_UTILS            ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_LOGGING          ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_WEBSOCKET        ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_SERVICE_LOADER   ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_COROUTINE        ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_MYSQL            ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_CRYPT            ON CACHE BOOL "" FORCE)

# ===============================
# Includes / subprojects
# ===============================
include_directories(
        src
        /usr/local/include
)

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

# ===============================
# Windows runtime DLL copy helper (optional but handy)
# ===============================
if (WIN32)
    # Если у тебя Boost/OpenSSL в DLL-режиме, Windows будет требовать *.dll при запуске.
    # Самый простой вариант: скопировать нужные DLL рядом с exe.
    #
    # Подстрой список по факту (смотри C:/msys64/mingw64/bin).
    add_custom_command(TARGET snake-server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/msys64/mingw64/bin/libboost_json-mt.dll"
            "$<TARGET_FILE_DIR:snake-server>/libboost_json-mt.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/msys64/mingw64/bin/libboost_system-mt.dll"
            "$<TARGET_FILE_DIR:snake-server>/libboost_system-mt.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/msys64/mingw64/bin/libssl-3-x64.dll"
            "$<TARGET_FILE_DIR:snake-server>/libssl-3-x64.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/msys64/mingw64/bin/libcrypto-3-x64.dll"
            "$<TARGET_FILE_DIR:snake-server>/libcrypto-3-x64.dll"
    )
endif()
