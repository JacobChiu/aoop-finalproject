#pragma once

#include <array>
#include <string>

class Pattern;
class InstrumentTrack;

// =============================================================================
// PatternEditorView —— 單一 Pattern 模式的編輯畫面（負責人：組員 A 徐偉瀞）
//
// 在「目前的 ImGui 視窗」裡畫出：
//   每條音軌：名稱、匯入 .wav、音量滑桿、16 格步進、試聽、刪除
//   下方：新增音軌、清除所有步進
//
// 使用方式（組員 B 的主視窗在 ImGui::Begin/End 之間呼叫）：
//     editor.draw(project.getActivePattern(),
//                 project.getIsPlaying(),      // 播放中 → 鎖定畫面
//                 project.getCurrentStep());   // 播放頭，-1 表示不顯示
//
// 操作：左鍵點步進格切換 ON/OFF；右鍵點步進格可調整該步的力度 (velocity)。
// =============================================================================
class PatternEditorView {
public:
    // nativeWindowHandle：主視窗的原生 handle，讓檔案視窗能置中在主視窗上
    explicit PatternEditorView(void* nativeWindowHandle = nullptr);

    void draw(Pattern& pattern, bool locked, int playheadStep = -1);

private:
    void drawLockBanner() const;
    void drawTrackTable(Pattern& pattern, bool locked, int playheadStep);
    void drawTrackRow(InstrumentTrack& track, int index, bool locked, int playheadStep);
    void drawSampleCell(InstrumentTrack& track, int index, bool locked);
    void drawStepButtons(InstrumentTrack& track, bool locked, int playheadStep);
    void drawAddTrackControls(Pattern& pattern, bool locked);
    void drawDeleteConfirmPopup(Pattern& pattern, bool locked);
    void drawManualPathPopup(Pattern& pattern, bool locked);
    void drawStatus() const;

    void importSample(InstrumentTrack& track, const std::string& path);
    void setStatus(std::string message, bool isError);

    void* ownerWindow;

    // 刪除確認（防呆）：先記下要刪哪一條，表格畫完後再跳出確認視窗
    int  pendingDeleteIndex = -1;
    bool requestDeletePopup = false;

    // 非 Windows 平台沒有檔案視窗時，改用手動輸入路徑
    int                    manualPathTrackIndex = -1;
    bool                   requestPathPopup     = false;
    std::array<char, 1024> manualPathBuffer{};

    std::array<char, 64> newTrackName{};

    std::string statusMessage;
    bool        statusIsError = false;
};
