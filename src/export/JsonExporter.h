#pragma once

#include "export/Exporter.h"

#include <optional>

#include "core/Project.h"

// =============================================================================
// JsonExporter —— 以 JSON 存檔 / 讀檔（負責人：組員 B 邱禹峰）
// 【骨架】由組員 A 先建立，內容待組員 B 實作。
//
// 可用到的 InstrumentTrack 公開介面（組員 A 提供）：
//   存檔：getTrackName()、getWavFilePath()、getVolume()、getStep(i).getIsActive()/getVelocity()
//   讀檔：Pattern::addTrack(name, wavPath)、setVolume()、getStep(i).setActive()/setVelocity()
// =============================================================================
class JsonExporter : public Exporter {
public:
    bool exportToFile(const Project& project, const std::string& path) override;

    // 讀檔失敗回傳 std::nullopt
    std::optional<Project> importFromFile(const std::string& path);
};
