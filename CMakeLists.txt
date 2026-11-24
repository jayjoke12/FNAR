cmake_minimum_required(VERSION 3.20)
project(FNAF_CPP LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(ANDROID)
    if(NOT ANDROID_NDK AND CMAKE_ANDROID_NDK)
        set(ANDROID_NDK "${CMAKE_ANDROID_NDK}")
    endif()
    set(PLATFORM "Android" CACHE STRING "raylib platform" FORCE)
    set(GRAPHICS "GRAPHICS_API_OPENGL_ES2" CACHE STRING "Graphics target" FORCE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    set(PLATFORM "Desktop" CACHE STRING "raylib platform" FORCE)
    set(GRAPHICS "GRAPHICS_API_OPENGL_33" CACHE STRING "Graphics target" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /GL)
        add_link_options(/LTCG)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic -ffunction-sections -fdata-sections)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
        add_link_options(-Wl,--gc-sections)
        if(NOT ANDROID)
            add_compile_options(-flto)
            add_link_options(-flto -s)
        endif()
    endif()
endif()

if((WIN32 OR MINGW) AND NOT ANDROID)
    if(MINGW)
        add_link_options(-static)
    endif()
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build raylib static" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RAUDIO ON CACHE BOOL "Enable raylib audio" FORCE)
add_subdirectory(thirdparty/raylib EXCLUDE_FROM_ALL)

target_compile_definitions(raylib PRIVATE SUPPORT_FILEFORMAT_PNG SUPPORT_FILEFORMAT_JPG)

set(ASSETS_BUNDLE "${CMAKE_CURRENT_BINARY_DIR}/assets.bundle")
file(GLOB_RECURSE BUNDLED_ASSET_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/config/*"
)
add_custom_command(
    OUTPUT "${ASSETS_BUNDLE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/pack_assets.py"
            "${ASSETS_BUNDLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${CMAKE_CURRENT_SOURCE_DIR}/config"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/pack_assets.py"
        ${BUNDLED_ASSET_FILES}
    COMMENT "Packing game assets into embedded bundle"
    VERBATIM
)

set_source_files_properties(src/EmbeddedAssets.S PROPERTIES
    OBJECT_DEPENDS "${ASSETS_BUNDLE}"
)

set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/Office.cpp
    src/Cameras.cpp
    src/Animatronic.cpp
    src/Power.cpp
    src/UI.cpp
    src/Audio.cpp
    src/AssetManager.cpp
    src/NightManager.cpp
    src/Settings.cpp
    src/BundleManager.cpp
    src/EmbeddedAssets.S
    "${ASSETS_BUNDLE}"
)

set(HEADERS
    src/Common.h
    src/Game.h
    src/Office.h
    src/Cameras.h
    src/Animatronic.h
    src/Power.h
    src/UI.h
    src/Audio.h
    src/AssetManager.h
    src/NightManager.h
    src/Settings.h
    src/BundleManager.h
)

if(ANDROID)
    # Android NativeActivity requires a shared library named by android.app.lib_name.
    add_library(fnaf SHARED ${SOURCES} ${HEADERS})
else()
    add_executable(fnaf ${SOURCES} ${HEADERS})
endif()

target_include_directories(fnaf PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(fnaf PRIVATE raylib)

if(ANDROID)
    target_include_directories(fnaf PRIVATE "${ANDROID_NDK}/sources/android/native_app_glue")
    target_link_libraries(fnaf PRIVATE android log EGL GLESv2 OpenSLES atomic c)
    target_link_options(fnaf PRIVATE "-Wl,-u,ANativeActivity_onCreate")
    set_target_properties(fnaf PROPERTIES
        OUTPUT_NAME "fnaf"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )
elseif(WIN32 OR MINGW)
    target_link_libraries(fnaf PRIVATE opengl32 gdi32 winmm shell32)
    if(MSVC)
        target_link_options(fnaf PRIVATE /SUBSYSTEM:WINDOWS)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_options(fnaf PRIVATE -mwindows)
    endif()
endif()

if(NOT ANDROID)
    add_custom_command(TARGET fnaf POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/config"
            "$<TARGET_FILE_DIR:fnaf>/config"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:fnaf>/assets"
        COMMENT "Copying assets and config to binary folder"
    )

    set(TEST_SOURCES
        tests/test_gameplay.cpp
        src/Settings.cpp
        src/NightManager.cpp
        src/Power.cpp
        src/Animatronic.cpp
        src/Office.cpp
        src/Cameras.cpp
        src/Audio.cpp
        src/AssetManager.cpp
        src/BundleManager.cpp
        src/EmbeddedAssets.S
        "${ASSETS_BUNDLE}"
    )

    add_executable(test_gameplay ${TEST_SOURCES})
    target_include_directories(test_gameplay PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(test_gameplay PRIVATE raylib)

    if(WIN32 OR MINGW)
        target_link_libraries(test_gameplay PRIVATE opengl32 gdi32 winmm shell32)
    endif()
endif()
