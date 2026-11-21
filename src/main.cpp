// =============================================================================
// 主程式（主視窗框架負責人：組員 B 邱禹峰）
//
// 【骨架】由組員 A 先建立，讓單一 Pattern 編輯畫面可以實際執行測試。
// 標示 TODO(B) 的地方（頂部控制列、所有 Pattern 模式、匯出）請組員 B 接手。
// =============================================================================
#include "core/Project.h"
#include "gui/PatternEditorView.h"

#include <imgui-SFML.h>
#include <imgui.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <filesystem>
#include <optional>

namespace {

// 使用支援中文的字型（微軟正黑體），這樣中文檔名 / 音軌名稱才不會變成 ???
void setupFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
#ifdef _WIN32
    const char* cjkFont = "C:\\Windows\\Fonts\\msjh.ttc";
    if (std::filesystem::exists(cjkFont)) {
        io.Fonts->AddFontFromFileTTF(cjkFont, 18.f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        return;
    }
#endif
    io.Fonts->AddFontDefault();
}

// 開啟程式時的示範內容：Kick / Snare / Hi-Hat 與一個基本節奏
void loadDemoPattern(Pattern& pattern) {
    InstrumentTrack& kick = pattern.addTrack("Kick", "assets/samples/kick.wav");
    for (int s : {0, 8, 10}) kick.setStepActive(s, true);

    InstrumentTrack& snare = pattern.addTrack("Snare", "assets/samples/snare.wav");
    for (int s : {4, 12}) snare.setStepActive(s, true);

    InstrumentTrack& hat = pattern.addTrack("Hi-Hat", "assets/samples/hihat.wav");
    hat.setVolume(55.f);
    for (int s = 0; s < InstrumentTrack::kStepCount; s += 2) hat.setStepActive(s, true);

    pattern.addTrack("Clap", "assets/samples/clap.wav");
}

// TODO(B): 頂部全域控制列（播放/停止、BPM、模式切換、Pattern 切換、匯出）
void drawTopBarPlaceholder(Project& project) {
    if (ImGui::Button(project.getIsPlaying() ? "Stop" : "Play", ImVec2{80.f, 0.f})) {
        project.togglePlay();
    }
    ImGui::SameLine();
    int bpm = project.getBPM();
    ImGui::SetNextItemWidth(200.f);
    ImGui::BeginDisabled(project.getIsPlaying());
    if (ImGui::SliderInt("BPM", &bpm, Project::kMinBpm, Project::kMaxBpm)) {
        project.setBPM(bpm);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("(top bar placeholder - owned by member B)");
}

}  // namespace

int main() {
    sf::RenderWindow window(sf::VideoMode({1280u, 720u}), "16-Step Drum Machine");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window, /*loadDefaultFont=*/false)) {
        return 1;
    }
    ImGui::GetIO().IniFilename = nullptr;   // 不要產生 imgui.ini
    setupFonts();
    if (!ImGui::SFML::UpdateFontTexture()) {
        return 1;
    }

    Project project;
    loadDemoPattern(project.getActivePattern());

#ifdef _WIN32
    void* nativeHandle = window.getNativeHandle();   // 讓檔案視窗以主視窗為父視窗
#else
    void* nativeHandle = nullptr;
#endif
    PatternEditorView patternEditor(nativeHandle);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        const sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);
        project.updateSequencer(dt.asSeconds());   // TODO(B)

        // 一個填滿整個 SFML 視窗的 ImGui 視窗
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::Begin("Main", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

        drawTopBarPlaceholder(project);
        ImGui::Separator();

        // TODO(B): 依 project.getIsSingleMode() 切換「單一 Pattern」與「所有 Pattern」畫面
        patternEditor.draw(project.getActivePattern(), project.getIsPlaying(), project.getCurrentStep());

        ImGui::End();

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
