#include "platform/FileDialog.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>

#include <vector>

namespace {

std::string wideToUtf8(const wchar_t* wide) {
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) {
        return {};
    }
    std::string utf8(static_cast<std::size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8.data(), size, nullptr, nullptr);
    return utf8;
}

}  // namespace

bool FileDialog::isSupported() { return true; }

std::optional<std::string> FileDialog::openWavFile(void* ownerWindow) {
    std::vector<wchar_t> buffer(32768, L'\0');   // 支援超長路徑

    OPENFILENAMEW ofn{};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = static_cast<HWND>(ownerWindow);
    ofn.lpstrFilter  = L"WAV audio (*.wav)\0*.wav\0All files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile    = buffer.data();
    ofn.nMaxFile     = static_cast<DWORD>(buffer.size());
    ofn.lpstrTitle   = L"Import .wav sample";
    // OFN_NOCHANGEDIR：不要改變程式的工作目錄，否則 assets/ 的相對路徑會失效
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&ofn)) {
        return std::nullopt;   // 取消或發生錯誤
    }
    return wideToUtf8(buffer.data());
}

#else  // ---------------------------------------------------------------- 其他平台

bool FileDialog::isSupported() { return false; }

std::optional<std::string> FileDialog::openWavFile(void* /*ownerWindow*/) { return std::nullopt; }

#endif
