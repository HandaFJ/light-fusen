cmake_minimum_required(VERSION 3.16)
project(StickyNoteFLTK VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Homebrewやカスタムprefixに入れたFLTKを見つけやすくする
if(APPLE AND DEFINED ENV{HOMEBREW_PREFIX})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{HOMEBREW_PREFIX}")
elseif(APPLE AND EXISTS "/opt/homebrew")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

find_package(FLTK REQUIRED)

# MinGWでは配布用にFLTKを静的リンクしたい。
# fltk::fltk は共有ライブラリ(libfltk-1.4.dll)を指すため、静的版を優先して探す。
set(STICKY_FLTK_LIBS "")
if(MINGW)
    if(TARGET fltk::fltk-static)
        set(STICKY_FLTK_LIBS fltk::fltk-static)
    else()
        find_library(STICKY_FLTK_STATIC_LIB NAMES libfltk.a)
        if(STICKY_FLTK_STATIC_LIB)
            set(STICKY_FLTK_LIBS ${STICKY_FLTK_STATIC_LIB})
        endif()
    endif()
endif()

# 上記で見つからなければ通常の方法。
# FLTK 1.4 は fltk::fltk ターゲットを提供する。1.3系は FLTK_LIBRARIES を使う
if(NOT STICKY_FLTK_LIBS)
    if(TARGET fltk::fltk)
        set(STICKY_FLTK_LIBS fltk::fltk)
    elseif(FLTK_LIBRARIES)
        set(STICKY_FLTK_LIBS ${FLTK_LIBRARIES})
    else()
        set(STICKY_FLTK_LIBS fltk)
    endif()
endif()
message(STATUS "FLTK link: ${STICKY_FLTK_LIBS}")

# ソースファイルは main.mm または main.cpp のどちらか存在するものを利用
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.mm")
    set(APP_SOURCES main.mm)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    set(APP_SOURCES main.cpp)
else()
    message(FATAL_ERROR "main.mm または main.cpp が見つかりません。")
endif()

add_executable(sticky_note ${APP_SOURCES})

if(FLTK_INCLUDE_DIR)
    target_include_directories(sticky_note PRIVATE ${FLTK_INCLUDE_DIR})
endif()
if(FLTK_INCLUDE_DIRS)
    target_include_directories(sticky_note PRIVATE ${FLTK_INCLUDE_DIRS})
endif()

# macOSではCocoaフレームワークをリンクし、.cppの場合はObjective-C++として扱う
if(APPLE)
    enable_language(OBJCXX)
    set_source_files_properties(${APP_SOURCES} PROPERTIES LANGUAGE OBJCXX)
    target_link_libraries(sticky_note PRIVATE ${STICKY_FLTK_LIBS} "-framework Cocoa")
elseif(WIN32)
    target_link_libraries(sticky_note PRIVATE ${STICKY_FLTK_LIBS}
        comctl32 comdlg32 gdi32 ole32 uuid user32 shell32 advapi32 ws2_32 winspool shlwapi)
    if(MINGW)
        # DLLなしで配布できるよう、ランタイムも静的リンク
        target_link_options(sticky_note PRIVATE -mwindows -static -static-libgcc -static-libstdc++)
    elseif(MSVC)
        set_target_properties(sticky_note PROPERTIES WIN32_EXECUTABLE ON)
        # main() をそのままエントリポイントにする
        target_link_options(sticky_note PRIVATE /ENTRY:mainCRTStartup)
    endif()
else()
    target_link_libraries(sticky_note PRIVATE ${STICKY_FLTK_LIBS})
endif()

# Releaseビルド時はサイズ最適化（-Os）を適用
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(sticky_note PRIVATE /O1)
    else()
        target_compile_options(sticky_note PRIVATE -Os)
    endif()
endif()
