#include "core/Project.h"

#include <algorithm>

// 【骨架】組員 B 可自由改寫本檔

Project::Project() { patterns.emplace_back("Pattern A"); }

void Project::setBPM(int newBpm) { bpm = std::clamp(newBpm, kMinBpm, kMaxBpm); }

int Project::getBPM() const { return bpm; }

void Project::togglePlay() {
    isPlaying = !isPlaying;
    if (!isPlaying) {
        currentStep = -1;
        for (Pattern& p : patterns) {
            p.stopAll();
        }
    }
}

bool Project::getIsPlaying() const { return isPlaying; }

void Project::setSingleMode(bool single) { isSingleMode = single; }

bool Project::getIsSingleMode() const { return isSingleMode; }

Pattern& Project::addPattern(const std::string& name) {
    patterns.emplace_back(name);
    return patterns.back();
}

bool Project::removePattern(int index) {
    if (index < 0 || index >= getPatternCount() || getPatternCount() <= 1) {
        return false;
    }
    patterns.erase(patterns.begin() + index);
    activePatternIndex = std::min(activePatternIndex, getPatternCount() - 1);
    return true;
}

int Project::getPatternCount() const { return static_cast<int>(patterns.size()); }

std::vector<Pattern>& Project::getPatterns() { return patterns; }

const std::vector<Pattern>& Project::getPatterns() const { return patterns; }

void Project::setActivePatternIndex(int index) {
    if (index >= 0 && index < getPatternCount()) {
        activePatternIndex = index;
    }
}

int Project::getActivePatternIndex() const { return activePatternIndex; }

Pattern& Project::getActivePattern() { return patterns.at(static_cast<std::size_t>(activePatternIndex)); }

const Pattern& Project::getActivePattern() const {
    return patterns.at(static_cast<std::size_t>(activePatternIndex));
}

void Project::updateSequencer(float deltaTime) {
    (void)deltaTime;
    // TODO(B): 計時播放引擎，見 Project.h 的說明
}

int Project::getCurrentStep() const { return currentStep; }
