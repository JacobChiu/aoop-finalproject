#include "gui/PatternEditorView.h"

#include "core/InstrumentTrack.h"
#include "core/Pattern.h"
#include "core/Step.h"
#include "platform/FileDialog.h"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstring>
#include <string>
#include <utility>

namespace {

// ---- 外觀常數 ---------------------------------------------------------------
constexpr float kStepSize       = 26.f;
constexpr float kStepGap        = 3.f;
constexpr float kBeatGap        = 9.f;   // 每 4 步（一拍）之間多留一點空隙
constexpr float kNameColWidth   = 120.f;
constexpr float kSampleColWidth = 210.f;
constexpr float kVolumeColWidth = 130.f;

const ImVec4 kStepOffA   {0.22f, 0.23f, 0.26f, 1.f};   // 第 1、3 拍
const ImVec4 kStepOffB   {0.30f, 0.31f, 0.35f, 1.f};   // 第 2、4 拍
const ImVec4 kStepOnLow  {0.55f, 0.30f, 0.08f, 1.f};   // ON、力度低
const ImVec4 kStepOnHigh {1.00f, 0.55f, 0.10f, 1.f};   // ON、力度高
const ImVec4 kPlayhead   {0.95f, 0.95f, 0.95f, 1.f};
const ImVec4 kWarnColor  {1.00f, 0.80f, 0.25f, 1.f};
const ImVec4 kErrorColor {1.00f, 0.40f, 0.40f, 1.f};
const ImVec4 kOkColor    {0.50f, 0.90f, 0.55f, 1.f};

ImVec4 lerp(const ImVec4& a, const ImVec4& b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t};
}

ImVec4 brighten(const ImVec4& c, float amount) {
    return {std::min(c.x + amount, 1.f), std::min(c.y + amount, 1.f), std::min(c.z + amount, 1.f), c.w};
}

// 只取檔名（同時處理 / 與 \）
std::string fileNameOf(const std::string& path) {
    const auto pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

template <std::size_t N>
void copyToBuffer(std::array<char, N>& buffer, const std::string& text) {
    const std::size_t n = std::min(text.size(), N - 1);
    std::memcpy(buffer.data(), text.data(), n);
    buffer[n] = '\0';
}

float stepsColumnWidth() {
    return kStepSize * InstrumentTrack::kStepCount + kStepGap * 12 + kBeatGap * 3;
}

constexpr const char* kDeletePopupId = "Delete track?";
constexpr const char* kPathPopupId   = "Load sample from path";

}  // namespace

PatternEditorView::PatternEditorView(void* nativeWindowHandle) : ownerWindow(nativeWindowHandle) {}

// =============================================================================
// 進入點
// =============================================================================
void PatternEditorView::draw(Pattern& pattern, bool locked, int playheadStep) {
    ImGui::PushID(&pattern);

    ImGui::Text("Pattern: %s", pattern.getName().c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("(%d / %d tracks)", pattern.getTrackCount(), Pattern::kMaxTracks);

    if (locked) {
        drawLockBanner();
    }

    drawTrackTable(pattern, locked, playheadStep);
    ImGui::Spacing();
    drawAddTrackControls(pattern, locked);
    drawStatus();

    // 彈出視窗必須在表格「外面」開啟/繪製，刪除音軌也在這裡才執行，
    // 避免在走訪 tracks 的途中修改 vector。
    if (requestDeletePopup) {
        ImGui::OpenPopup(kDeletePopupId);
        requestDeletePopup = false;
    }
    if (requestPathPopup) {
        ImGui::OpenPopup(kPathPopupId);
        requestPathPopup = false;
    }
    drawDeleteConfirmPopup(pattern, locked);
    drawManualPathPopup(pattern, locked);

    ImGui::PopID();
}

void PatternEditorView::drawLockBanner() const {
    ImGui::TextColored(kWarnColor, "Playing - editing is locked. Stop playback to make changes.");
}

// =============================================================================
// 音軌表格
// =============================================================================
void PatternEditorView::drawTrackTable(Pattern& pattern, bool locked, int playheadStep) {
    if (pattern.getTrackCount() == 0) {
        ImGui::TextDisabled("No tracks yet. Add one below.");
        return;
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollX |
                                  ImGuiTableFlags_ScrollY;
    // 表格高度：最多佔掉剩餘空間，保留底部放「新增音軌」等控制項
    const float reserveBottom = ImGui::GetFrameHeightWithSpacing() * 3.f;
    const ImVec2 size{0.f, std::max(ImGui::GetContentRegionAvail().y - reserveBottom, 120.f)};

    if (!ImGui::BeginTable("tracks", 5, flags, size)) {
        return;
    }
    ImGui::TableSetupScrollFreeze(1, 1);
    ImGui::TableSetupColumn("Track", ImGuiTableColumnFlags_WidthFixed, kNameColWidth);
    ImGui::TableSetupColumn("Sample", ImGuiTableColumnFlags_WidthFixed, kSampleColWidth);
    ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, kVolumeColWidth);
    ImGui::TableSetupColumn("Steps (left: on/off, right: velocity)", ImGuiTableColumnFlags_WidthFixed,
                            stepsColumnWidth());
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableHeadersRow();

    for (int i = 0; i < pattern.getTrackCount(); ++i) {
        drawTrackRow(pattern.getTrack(i), i, locked, playheadStep);
    }
    ImGui::EndTable();
}

void PatternEditorView::drawTrackRow(InstrumentTrack& track, int index, bool locked, int playheadStep) {
    ImGui::PushID(index);
    ImGui::TableNextRow(ImGuiTableRowFlags_None, kStepSize + 6.f);

    // ---- 名稱 ---------------------------------------------------------------
    ImGui::TableSetColumnIndex(0);
    ImGui::BeginDisabled(locked);
    {
        std::array<char, 64> nameBuffer{};
        copyToBuffer(nameBuffer, track.getTrackName());
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##name", nameBuffer.data(), nameBuffer.size())) {
            track.setTrackName(nameBuffer.data());
        }
    }
    ImGui::EndDisabled();

    // ---- 音色 ---------------------------------------------------------------
    ImGui::TableSetColumnIndex(1);
    drawSampleCell(track, index, locked);

    // ---- 音量 ---------------------------------------------------------------
    ImGui::TableSetColumnIndex(2);
    ImGui::BeginDisabled(locked);
    {
        float volume = track.getVolume();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderFloat("##volume", &volume, InstrumentTrack::kMinVolume,
                               InstrumentTrack::kMaxVolume, "Vol %.0f")) {
            track.setVolume(volume);
        }
    }
    ImGui::EndDisabled();

    // ---- 16 格步進 ----------------------------------------------------------
    ImGui::TableSetColumnIndex(3);
    drawStepButtons(track, locked, playheadStep);

    // ---- 試聽 / 刪除 --------------------------------------------------------
    ImGui::TableSetColumnIndex(4);
    ImGui::BeginDisabled(!track.hasSample());   // 試聽不改資料，播放中也允許
    if (ImGui::Button("Preview")) {
        track.preview();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(locked);
    if (ImGui::Button("Delete")) {
        pendingDeleteIndex = index;
        requestDeletePopup = true;
    }
    ImGui::EndDisabled();

    ImGui::PopID();
}

void PatternEditorView::drawSampleCell(InstrumentTrack& track, int index, bool locked) {
    ImGui::BeginDisabled(locked);
    if (ImGui::Button("Import .wav")) {
        if (FileDialog::isSupported()) {
            // 會跳出 Windows 檔案總管的開啟視窗；這段期間畫面會暫停更新
            if (auto path = FileDialog::openWavFile(ownerWindow)) {
                importSample(track, *path);
            }
        } else {
            manualPathTrackIndex = index;
            copyToBuffer(manualPathBuffer, track.getWavFilePath());
            requestPathPopup = true;
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (track.hasSample()) {
        ImGui::TextUnformatted(fileNameOf(track.getWavFilePath()).c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s\n%.2f s", track.getWavFilePath().c_str(), track.getSampleDuration());
        }
    } else {
        ImGui::TextDisabled("(no sample)");
    }
}

void PatternEditorView::drawStepButtons(InstrumentTrack& track, bool locked, int playheadStep) {
    // 鎖定時仍要看得清楚播放頭，所以把 disabled 的淡化程度調輕
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.8f);
    ImGui::BeginDisabled(locked);

    for (int s = 0; s < InstrumentTrack::kStepCount; ++s) {
        if (s > 0) {
            ImGui::SameLine(0.f, (s % 4 == 0) ? kBeatGap : kStepGap);
        }
        ImGui::PushID(s);

        Step&      step     = track.getStep(s);
        const bool active   = step.getIsActive();
        const bool isPlayhd = (s == playheadStep);

        ImVec4 color;
        if (active) {
            const float t = static_cast<float>(step.getVelocity()) / static_cast<float>(Step::kMaxVelocity);
            color = lerp(kStepOnLow, kStepOnHigh, t);
        } else {
            color = ((s / 4) % 2 == 0) ? kStepOffA : kStepOffB;
        }
        if (isPlayhd) {
            color = brighten(color, 0.25f);
        }

        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, brighten(color, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, brighten(color, 0.2f));
        ImGui::PushStyleColor(ImGuiCol_Border, isPlayhd ? kPlayhead : ImVec4{0, 0, 0, 0});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, isPlayhd ? 2.f : 0.f);

        if (ImGui::Button("##step", ImVec2{kStepSize, kStepSize})) {
            track.toggleStep(s);
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Step %d  %s  velocity %d", s + 1, active ? "ON" : "off", step.getVelocity());
        }

        // 右鍵：調整力度
        if (ImGui::BeginPopupContextItem("velocity")) {
            ImGui::Text("Step %d", s + 1);
            int velocity = step.getVelocity();
            ImGui::SetNextItemWidth(160.f);
            if (ImGui::SliderInt("Velocity", &velocity, Step::kMinVelocity, Step::kMaxVelocity)) {
                step.setVelocity(velocity);
            }
            if (ImGui::Button(active ? "Turn off" : "Turn on")) {
                step.toggle();
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    ImGui::EndDisabled();
    ImGui::PopStyleVar();
}

// =============================================================================
// 下方控制列
// =============================================================================
void PatternEditorView::drawAddTrackControls(Pattern& pattern, bool locked) {
    ImGui::BeginDisabled(locked || !pattern.canAddTrack());

    ImGui::SetNextItemWidth(180.f);
    const bool enterPressed = ImGui::InputTextWithHint("##newTrack", "New track name", newTrackName.data(),
                                                       newTrackName.size(),
                                                       ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::Button("+ Add Track") || enterPressed) {
        std::string name = newTrackName.data();
        if (name.empty()) {
            name = "Track " + std::to_string(pattern.getTrackCount() + 1);
        }
        pattern.addTrack(name);
        newTrackName[0] = '\0';
        setStatus("Added track \"" + name + "\". Click \"Import .wav\" to give it a sound.", false);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(locked || pattern.getTrackCount() == 0);
    if (ImGui::Button("Clear All Steps")) {
        pattern.clearAllSteps();
        setStatus("Cleared all steps in \"" + pattern.getName() + "\".", false);
    }
    ImGui::EndDisabled();

    if (!pattern.canAddTrack()) {
        ImGui::SameLine();
        ImGui::TextDisabled("Track limit reached (%d)", Pattern::kMaxTracks);
    }
}

void PatternEditorView::drawStatus() const {
    if (!statusMessage.empty()) {
        ImGui::TextColored(statusIsError ? kErrorColor : kOkColor, "%s", statusMessage.c_str());
    }
}

// =============================================================================
// 彈出視窗
// =============================================================================
void PatternEditorView::drawDeleteConfirmPopup(Pattern& pattern, bool locked) {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
    if (!ImGui::BeginPopupModal(kDeletePopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    const bool valid = pendingDeleteIndex >= 0 && pendingDeleteIndex < pattern.getTrackCount();
    if (locked || !valid) {
        // 開始播放或音軌已不存在 → 直接取消
        pendingDeleteIndex = -1;
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    const std::string name = pattern.getTrack(pendingDeleteIndex).getTrackName();
    ImGui::Text("Delete track \"%s\"?", name.c_str());
    ImGui::TextDisabled("Its 16 steps and sample will be removed.");
    ImGui::Separator();

    if (ImGui::Button("Delete", ImVec2{100.f, 0.f})) {
        pattern.removeTrack(pendingDeleteIndex);
        setStatus("Deleted track \"" + name + "\".", false);
        pendingDeleteIndex = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2{100.f, 0.f}) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        pendingDeleteIndex = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void PatternEditorView::drawManualPathPopup(Pattern& pattern, bool locked) {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
    if (!ImGui::BeginPopupModal(kPathPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    const bool valid = manualPathTrackIndex >= 0 && manualPathTrackIndex < pattern.getTrackCount();
    if (locked || !valid) {
        manualPathTrackIndex = -1;
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    ImGui::TextUnformatted("Path to a .wav file:");
    ImGui::SetNextItemWidth(420.f);
    const bool enter = ImGui::InputText("##path", manualPathBuffer.data(), manualPathBuffer.size(),
                                        ImGuiInputTextFlags_EnterReturnsTrue);
    if (ImGui::Button("Load", ImVec2{100.f, 0.f}) || enter) {
        importSample(pattern.getTrack(manualPathTrackIndex), manualPathBuffer.data());
        manualPathTrackIndex = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2{100.f, 0.f}) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        manualPathTrackIndex = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// =============================================================================
// 輔助
// =============================================================================
void PatternEditorView::importSample(InstrumentTrack& track, const std::string& path) {
    if (track.loadWavFile(path)) {
        setStatus("Loaded \"" + fileNameOf(path) + "\" into \"" + track.getTrackName() + "\".", false);
        track.preview();   // 匯入後立即試聽一次
    } else {
        setStatus("Could not load \"" + fileNameOf(path) + "\". Is it a valid .wav file?", true);
    }
}

void PatternEditorView::setStatus(std::string message, bool isError) {
    statusMessage = std::move(message);
    statusIsError = isError;
}
