#include "core/InstrumentTrack.h"

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>

InstrumentTrack::InstrumentTrack(std::string name) : trackName(std::move(name)) {}

// sound 宣告在 soundBuffer 之後，解構時會先釋放 sound，再釋放它引用的 buffer。
InstrumentTrack::~InstrumentTrack() = default;

InstrumentTrack::InstrumentTrack(InstrumentTrack&& other) noexcept = default;

InstrumentTrack& InstrumentTrack::operator=(InstrumentTrack&& other) noexcept {
    if (this != &other) {
        // 先放掉自己的 sound，再換 buffer，避免 sound 短暫指向已釋放的 buffer
        sound.reset();
        trackName   = std::move(other.trackName);
        wavFilePath = std::move(other.wavFilePath);
        volume      = other.volume;
        steps       = other.steps;
        soundBuffer = std::move(other.soundBuffer);
        sound       = std::move(other.sound);
    }
    return *this;
}

// ---- 名稱 -------------------------------------------------------------------

const std::string& InstrumentTrack::getTrackName() const { return trackName; }

void InstrumentTrack::setTrackName(std::string name) { trackName = std::move(name); }

// ---- 音色 -------------------------------------------------------------------

bool InstrumentTrack::loadWavFile(const std::string& utf8Path) {
    if (utf8Path.empty()) {
        return false;
    }

    // 先載入到新的 buffer，成功才替換，失敗時舊音色不受影響
    auto newBuffer = std::make_unique<sf::SoundBuffer>();
    // u8path：讓 Windows 上的中文路徑也能正確開啟
    if (!newBuffer->loadFromFile(std::filesystem::u8path(utf8Path))) {
        return false;
    }

    if (sound) {
        sound->stop();
    }
    sound.reset();                       // 舊 sound 必須在舊 buffer 之前釋放
    soundBuffer = std::move(newBuffer);
    sound       = std::make_unique<sf::Sound>(*soundBuffer);
    wavFilePath = utf8Path;
    return true;
}

bool InstrumentTrack::hasSample() const { return sound != nullptr; }

const std::string& InstrumentTrack::getWavFilePath() const { return wavFilePath; }

float InstrumentTrack::getSampleDuration() const {
    return soundBuffer ? soundBuffer->getDuration().asSeconds() : 0.f;
}

// ---- 音量 -------------------------------------------------------------------

void InstrumentTrack::setVolume(float vol) { volume = std::clamp(vol, kMinVolume, kMaxVolume); }

float InstrumentTrack::getVolume() const { return volume; }

// ---- 步進 -------------------------------------------------------------------

void InstrumentTrack::checkIndex(int index) {
    if (index < 0 || index >= kStepCount) {
        throw std::out_of_range("InstrumentTrack: step index " + std::to_string(index) +
                                " is out of range [0, " + std::to_string(kStepCount - 1) + "]");
    }
}

void InstrumentTrack::toggleStep(int index) {
    checkIndex(index);
    steps[index].toggle();
}

void InstrumentTrack::setStepActive(int index, bool state) {
    checkIndex(index);
    steps[index].setActive(state);
}

bool InstrumentTrack::isStepActive(int index) const {
    checkIndex(index);
    return steps[index].getIsActive();
}

Step& InstrumentTrack::getStep(int index) {
    checkIndex(index);
    return steps[index];
}

const Step& InstrumentTrack::getStep(int index) const {
    checkIndex(index);
    return steps[index];
}

int InstrumentTrack::getActiveStepCount() const {
    return static_cast<int>(std::count_if(steps.begin(), steps.end(),
                                          [](const Step& s) { return s.getIsActive(); }));
}

void InstrumentTrack::clearSteps() {
    for (Step& s : steps) {
        s.setActive(false);
    }
}

// ---- 發聲 -------------------------------------------------------------------

void InstrumentTrack::trigger(int velocity) {
    if (!sound) {
        return;
    }
    // 最終音量 = 軌道音量 (0~100) × 力度比例 (0~1)
    const float gain = volume * static_cast<float>(velocity) / static_cast<float>(Step::kMaxVelocity);
    sound->setVolume(gain);
    sound->play();   // 若正在播放，SFML 會從頭重新播放（鼓機的 choke 行為）
}

bool InstrumentTrack::playStep(int index) {
    checkIndex(index);
    const Step& step = steps[index];
    if (!step.getIsActive() || !sound) {
        return false;
    }
    trigger(step.getVelocity());
    return true;
}

void InstrumentTrack::preview() { trigger(Step::kDefaultVelocity); }

void InstrumentTrack::stop() {
    if (sound) {
        sound->stop();
    }
}
