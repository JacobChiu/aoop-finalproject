#pragma once

#include <string>

class Project;

// =============================================================================
// Exporter —— 匯出策略的抽象介面（Strategy Pattern，負責人：組員 B 邱禹峰）
// 【骨架】由組員 A 先建立，組員 B 可自由修改。
// =============================================================================
class Exporter {
public:
    virtual ~Exporter() = default;

    // 成功回傳 true
    virtual bool exportToFile(const Project& project, const std::string& path) = 0;
};
