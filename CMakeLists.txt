cmake_minimum_required(VERSION 3.24)
project(DrumMachine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DRUM_BUILD_APP   "Build the GUI application (needs SFML Graphics + ImGui)" ON)
option(DRUM_BUILD_TESTS "Build the core unit tests" ON)

# 所有輸出（exe、dll）放同一個資料夾，assets 也複製到這裡
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------------------------
# 第三方函式庫：第一次 configure 時自動從 GitHub 下載，組員不用手動安裝
# ---------------------------------------------------------------------------
include(FetchContent)

set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
if(NOT DRUM_BUILD_APP)
    # 只建核心 + 測試時不需要視窗/繪圖模組
    set(SFML_BUILD_WINDOW   OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_GRAPHICS OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

if(DRUM_BUILD_APP)
    # ImGui-SFML v3.0 官方測試搭配的是 Dear ImGui v1.91.1
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(imgui)

    set(IMGUI_DIR ${imgui_SOURCE_DIR})
    set(IMGUI_SFML_FIND_SFML OFF)
    FetchContent_Declare(ImGui-SFML
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG v3.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(ImGui-SFML)
endif()

# ---------------------------------------------------------------------------
# 共用編譯選項
# ---------------------------------------------------------------------------
function(drum_set_warnings target)
    if(MSVC)
        # /utf-8：原始碼含中文註解，避免 VS 在繁中系統用 Big5 讀檔 (C4819)
        target_compile_options(${target} PRIVATE /W4 /utf-8 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# drum_core：資料模型 + 音訊（不依賴 GUI，可單獨測試）
#
# 自動收集 src/core/ 與 src/export/ 底下所有 .cpp，
# 新增檔案時「不需要」修改這個 CMakeLists.txt。
# ---------------------------------------------------------------------------
file(GLOB_RECURSE DRUM_CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_SOURCE_DIR}/src/export/*.cpp)
add_library(drum_core STATIC ${DRUM_CORE_SOURCES})
target_include_directories(drum_core PUBLIC src)
target_link_libraries(drum_core PUBLIC SFML::Audio)
drum_set_warnings(drum_core)

# ---------------------------------------------------------------------------
# DrumMachine：GUI 主程式
#
# 有 src/main.cpp 時才會建立。會自動收集 main.cpp 以及
# src/gui/、src/app/、src/platform/ 底下的所有 .cpp。
# 第一次新增 main.cpp 後，請執行一次「CMake: Configure」。
# ---------------------------------------------------------------------------
if(DRUM_BUILD_APP AND EXISTS ${CMAKE_SOURCE_DIR}/src/main.cpp)
    file(GLOB_RECURSE DRUM_APP_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/src/main.cpp
        ${CMAKE_SOURCE_DIR}/src/gui/*.cpp
        ${CMAKE_SOURCE_DIR}/src/app/*.cpp
        ${CMAKE_SOURCE_DIR}/src/platform/*.cpp)
    add_executable(DrumMachine ${DRUM_APP_SOURCES})
    target_link_libraries(DrumMachine PRIVATE drum_core ImGui-SFML::ImGui-SFML SFML::Graphics)
    if(WIN32)
        target_link_libraries(DrumMachine PRIVATE comdlg32)   # Windows 開啟檔案視窗
    endif()
    drum_set_warnings(DrumMachine)

    # 把 assets/ 複製到 exe 旁邊，並讓 VS 按 F5 時的工作目錄就是 exe 所在資料夾
    add_custom_command(TARGET DrumMachine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:DrumMachine>/assets)
    set_target_properties(DrumMachine PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:DrumMachine>)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DrumMachine)
elseif(DRUM_BUILD_APP)
    message(STATUS "src/main.cpp not found yet - skipping the DrumMachine app (core + tests only)")
endif()

# ---------------------------------------------------------------------------
# 單元測試：tests/ 底下每個 .cpp 各自是一個測試程式
# ---------------------------------------------------------------------------
if(DRUM_BUILD_TESTS)
    enable_testing()
    file(GLOB DRUM_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/*.cpp)
    foreach(test_src IN LISTS DRUM_TEST_SOURCES)
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_link_libraries(${test_name} PRIVATE drum_core)
        target_compile_definitions(${test_name} PRIVATE
            DRUM_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
        drum_set_warnings(${test_name})
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
