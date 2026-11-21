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
# ---------------------------------------------------------------------------
add_library(drum_core STATIC
    src/core/Step.cpp
    src/core/InstrumentTrack.cpp
    src/core/Pattern.cpp
    src/core/Project.cpp
    src/export/JsonExporter.cpp
)
target_include_directories(drum_core PUBLIC src)
target_link_libraries(drum_core PUBLIC SFML::Audio)
drum_set_warnings(drum_core)

# ---------------------------------------------------------------------------
# DrumMachine：GUI 主程式
# ---------------------------------------------------------------------------
if(DRUM_BUILD_APP)
    add_executable(DrumMachine
        src/main.cpp
        src/gui/PatternEditorView.cpp
        src/platform/FileDialog.cpp
    )
    target_link_libraries(DrumMachine PRIVATE drum_core ImGui-SFML::ImGui-SFML SFML::Graphics)
    if(WIN32)
        target_link_libraries(DrumMachine PRIVATE comdlg32)   # GetOpenFileNameW
    endif()
    drum_set_warnings(DrumMachine)

    # 把 assets/ 複製到 exe 旁邊，並讓 VS 按 F5 時的工作目錄就是 exe 所在資料夾
    add_custom_command(TARGET DrumMachine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:DrumMachine>/assets)
    set_target_properties(DrumMachine PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:DrumMachine>)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DrumMachine)
endif()

# ---------------------------------------------------------------------------
# 單元測試
# ---------------------------------------------------------------------------
if(DRUM_BUILD_TESTS)
    enable_testing()
    add_executable(core_tests tests/core_tests.cpp)
    target_link_libraries(core_tests PRIVATE drum_core)
    target_compile_definitions(core_tests PRIVATE
        DRUM_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
    drum_set_warnings(core_tests)
    add_test(NAME core_tests COMMAND core_tests)
endif()
