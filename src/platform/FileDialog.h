#pragma once

#include <optional>
#include <string>

// =============================================================================
// FileDialog —— 呼叫作業系統的「開啟檔案」視窗（負責人：組員 A 徐偉瀞）
//
// Windows：使用內建的 GetOpenFileNameW（就是一般的檔案總管開啟視窗），
//          不需要任何第三方函式庫。
// 其他平台：目前不支援，isSupported() 回傳 false，GUI 會改顯示路徑輸入框。
// =============================================================================
namespace FileDialog {

bool isSupported();

// 開啟「選擇 .wav 檔」視窗。回傳 UTF-8 路徑；使用者按取消則回傳 std::nullopt。
// ownerWindow：父視窗的原生 handle（sf::Window::getNativeHandle()），
//              傳入後對話框會置中於主視窗並維持在其上方；可傳 nullptr。
std::optional<std::string> openWavFile(void* ownerWindow = nullptr);

}  // namespace FileDialog
