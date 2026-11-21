#include "export/JsonExporter.h"

// 【骨架】組員 B 實作

bool JsonExporter::exportToFile(const Project& project, const std::string& path) {
    (void)project;
    (void)path;
    // TODO(B): 序列化 BPM、每個 Pattern 的音軌名稱 / .wav 路徑 / 音量 / 16 步狀態
    return false;
}

std::optional<Project> JsonExporter::importFromFile(const std::string& path) {
    (void)path;
    // TODO(B): 讀取 JSON 並重建 Project
    return std::nullopt;
}
