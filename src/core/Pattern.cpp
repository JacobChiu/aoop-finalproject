#include "core/Pattern.h"

#include <stdexcept>
#include <utility>

Pattern::Pattern(std::string patternName) : name(std::move(patternName)) {}

// ---- 名稱 / 總覽模式開關 ----------------------------------------------------

const std::string& Pattern::getName() const { return name; }

void Pattern::setName(std::string newName) { name = std::move(newName); }

void Pattern::setEnabled(bool enabled) { enabledInAllMode = enabled; }

bool Pattern::isEnabled() const { return enabledInAllMode; }

// ---- 音軌管理 ---------------------------------------------------------------

bool Pattern::canAddTrack() const { return static_cast<int>(tracks.size()) < kMaxTracks; }

InstrumentTrack& Pattern::addTrack(const std::string& trackName) {
    if (!canAddTrack()) {
        throw std::length_error("Pattern: cannot add more than " + std::to_string(kMaxTracks) +
                                " tracks");
    }
    tracks.emplace_back(trackName);
    return tracks.back();
}

InstrumentTrack& Pattern::addTrack(const std::string& trackName, const std::string& wavPath) {
    InstrumentTrack& track = addTrack(trackName);
    track.loadWavFile(wavPath);   // 失敗也保留音軌，讓使用者之後再匯入
    return track;
}

bool Pattern::removeTrack(int index) {
    if (index < 0 || index >= getTrackCount()) {
        return false;
    }
    tracks[index].stop();
    tracks.erase(tracks.begin() + index);
    return true;
}

int Pattern::getTrackCount() const { return static_cast<int>(tracks.size()); }

InstrumentTrack& Pattern::getTrack(int index) { return tracks.at(static_cast<std::size_t>(index)); }

const InstrumentTrack& Pattern::getTrack(int index) const {
    return tracks.at(static_cast<std::size_t>(index));
}

std::vector<InstrumentTrack>& Pattern::getTracks() { return tracks; }

const std::vector<InstrumentTrack>& Pattern::getTracks() const { return tracks; }

// ---- 發聲 -------------------------------------------------------------------

int Pattern::triggerStep(int stepIndex) {
    int played = 0;
    for (InstrumentTrack& track : tracks) {
        if (track.playStep(stepIndex)) {
            ++played;
        }
    }
    return played;
}

void Pattern::stopAll() {
    for (InstrumentTrack& track : tracks) {
        track.stop();
    }
}

void Pattern::clearAllSteps() {
    for (InstrumentTrack& track : tracks) {
        track.clearSteps();
    }
}
