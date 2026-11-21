// 核心模組單元測試（不需要 GUI）。執行：ctest 或直接跑 core_tests
#include "core/InstrumentTrack.h"
#include "core/Pattern.h"
#include "core/Project.h"
#include "core/Step.h"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;
int checks   = 0;

#define CHECK(cond)                                                                       \
    do {                                                                                  \
        ++checks;                                                                         \
        if (!(cond)) {                                                                    \
            ++failures;                                                                   \
            std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n";      \
        }                                                                                 \
    } while (0)

template <typename E, typename F>
bool throws(F&& f) {
    try {
        f();
    } catch (const E&) {
        return true;
    } catch (...) {
    }
    return false;
}

const std::string kAssets = DRUM_ASSETS_DIR;
const std::string kKick   = kAssets + "/samples/kick.wav";
const std::string kSnare  = kAssets + "/samples/snare.wav";

// ---------------------------------------------------------------------------
void testStep() {
    Step s;
    CHECK(!s.getIsActive());
    CHECK(s.getVelocity() == Step::kDefaultVelocity);
    s.toggle();
    CHECK(s.getIsActive());
    s.toggle();
    CHECK(!s.getIsActive());
    s.setActive(true);
    CHECK(s.getIsActive());
    s.setVelocity(500);
    CHECK(s.getVelocity() == 127);
    s.setVelocity(-3);
    CHECK(s.getVelocity() == 0);
    Step t(true, 64);
    CHECK(t.getIsActive() && t.getVelocity() == 64);
}

void testTrackSteps() {
    InstrumentTrack t("Kick");
    CHECK(t.getTrackName() == "Kick");
    CHECK(t.getActiveStepCount() == 0);
    t.toggleStep(0);
    t.toggleStep(15);
    CHECK(t.isStepActive(0) && t.isStepActive(15) && !t.isStepActive(7));
    CHECK(t.getActiveStepCount() == 2);
    t.toggleStep(0);
    CHECK(!t.isStepActive(0));
    t.setStepActive(3, true);
    CHECK(t.getStep(3).getIsActive());
    t.clearSteps();
    CHECK(t.getActiveStepCount() == 0);

    CHECK(throws<std::out_of_range>([&] { t.toggleStep(16); }));
    CHECK(throws<std::out_of_range>([&] { t.toggleStep(-1); }));
    CHECK(throws<std::out_of_range>([&] { (void)t.isStepActive(99); }));
    CHECK(throws<std::out_of_range>([&] { t.playStep(16); }));
}

void testTrackVolume() {
    InstrumentTrack t;
    CHECK(t.getVolume() == InstrumentTrack::kDefaultVolume);
    t.setVolume(150.f);
    CHECK(t.getVolume() == 100.f);
    t.setVolume(-5.f);
    CHECK(t.getVolume() == 0.f);
    t.setVolume(42.f);
    CHECK(t.getVolume() == 42.f);
}

void testTrackSample() {
    InstrumentTrack t("Kick");
    CHECK(!t.hasSample());
    CHECK(t.getWavFilePath().empty());

    // 沒有音色時 playStep 不會發聲也不會當掉
    t.setStepActive(0, true);
    CHECK(t.playStep(0) == false);
    t.preview();

    // 載入失敗：保持沒有音色
    CHECK(t.loadWavFile("does/not/exist.wav") == false);
    CHECK(t.loadWavFile("") == false);
    CHECK(!t.hasSample());

    // 載入成功
    CHECK(t.loadWavFile(kKick));
    CHECK(t.hasSample());
    CHECK(t.getWavFilePath() == kKick);
    CHECK(t.getSampleDuration() > 0.1f);

    CHECK(t.playStep(0) == true);    // ON + 有音色 → 發聲
    CHECK(t.playStep(1) == false);   // OFF → 不發聲

    // 再次載入失敗時，舊音色與路徑保留
    CHECK(t.loadWavFile("nope.wav") == false);
    CHECK(t.hasSample());
    CHECK(t.getWavFilePath() == kKick);

    // 換音色
    CHECK(t.loadWavFile(kSnare));
    CHECK(t.getWavFilePath() == kSnare);
    t.stop();
}

void testTrackMove() {
    InstrumentTrack a("Snare");
    a.loadWavFile(kSnare);
    a.setStepActive(4, true);
    a.setVolume(30.f);

    InstrumentTrack b(std::move(a));
    CHECK(b.getTrackName() == "Snare");
    CHECK(b.hasSample());
    CHECK(b.isStepActive(4));
    CHECK(b.getVolume() == 30.f);
    CHECK(b.playStep(4));

    InstrumentTrack c("Other");
    c.loadWavFile(kKick);
    c = std::move(b);
    CHECK(c.getTrackName() == "Snare");
    CHECK(c.getWavFilePath() == kSnare);
    CHECK(c.playStep(4));
    c.stop();
}

void testPatternTracks() {
    Pattern p("Pattern A");
    CHECK(p.getName() == "Pattern A");
    CHECK(p.getTrackCount() == 0);
    CHECK(p.isEnabled());

    p.addTrack("Kick", kKick).setStepActive(0, true);
    p.addTrack("Snare", kSnare).setStepActive(0, true);
    p.addTrack("Empty");                       // 沒有音色
    p.getTrack(2).setStepActive(0, true);
    p.addTrack("Broken", "missing.wav");       // 載入失敗仍建立音軌
    CHECK(p.getTrackCount() == 4);
    CHECK(!p.getTrack(3).hasSample());

    // 只有「ON 且有音色」的音軌會發聲
    CHECK(p.triggerStep(0) == 2);
    CHECK(p.triggerStep(1) == 0);

    // 刪除中間一條，順序保持
    CHECK(p.removeTrack(1));
    CHECK(p.getTrackCount() == 3);
    CHECK(p.getTrack(0).getTrackName() == "Kick");
    CHECK(p.getTrack(1).getTrackName() == "Empty");
    CHECK(p.getTrack(0).hasSample());

    // 無效 index
    CHECK(!p.removeTrack(-1));
    CHECK(!p.removeTrack(3));
    CHECK(throws<std::out_of_range>([&] { (void)p.getTrack(10); }));

    p.clearAllSteps();
    CHECK(p.triggerStep(0) == 0);
    p.stopAll();
}

// vector 擴充時會搬移 InstrumentTrack，音色必須仍然可用（sf::Sound 不能指向失效的 buffer）
void testPatternReallocationKeepsSamples() {
    Pattern p;
    for (int i = 0; i < Pattern::kMaxTracks; ++i) {
        p.addTrack("T" + std::to_string(i), (i % 2) ? kKick : kSnare).setStepActive(i % 16, true);
    }
    CHECK(!p.canAddTrack());
    CHECK(throws<std::length_error>([&] { p.addTrack("overflow"); }));

    int played = 0;
    for (int s = 0; s < InstrumentTrack::kStepCount; ++s) played += p.triggerStep(s);
    CHECK(played == Pattern::kMaxTracks);

    while (p.getTrackCount() > 0) p.removeTrack(0);
    CHECK(p.getTrackCount() == 0);
    CHECK(p.canAddTrack());
}

void testPatternMove() {
    std::vector<Pattern> patterns;
    patterns.emplace_back("A");
    patterns[0].addTrack("Kick", kKick).setStepActive(0, true);
    for (int i = 0; i < 10; ++i) patterns.emplace_back("P" + std::to_string(i));   // 強迫重新配置
    CHECK(patterns[0].getTrack(0).hasSample());
    CHECK(patterns[0].triggerStep(0) == 1);
}

void testProjectSkeleton() {
    Project proj;
    CHECK(proj.getPatternCount() == 1);
    CHECK(proj.getBPM() == Project::kDefaultBpm);
    proj.setBPM(10);
    CHECK(proj.getBPM() == 50);
    proj.setBPM(999);
    CHECK(proj.getBPM() == 200);
    CHECK(!proj.getIsPlaying());
    CHECK(proj.getCurrentStep() == -1);
    proj.togglePlay();
    CHECK(proj.getIsPlaying());
    proj.togglePlay();
    CHECK(!proj.getIsPlaying());
}

}  // namespace

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"Step", testStep},
        {"InstrumentTrack steps", testTrackSteps},
        {"InstrumentTrack volume", testTrackVolume},
        {"InstrumentTrack sample", testTrackSample},
        {"InstrumentTrack move", testTrackMove},
        {"Pattern tracks", testPatternTracks},
        {"Pattern reallocation", testPatternReallocationKeepsSamples},
        {"Pattern move", testPatternMove},
        {"Project skeleton", testProjectSkeleton},
    };

    for (const auto& [name, fn] : tests) {
        const int before = failures;
        try {
            fn();
        } catch (const std::exception& e) {
            ++failures;
            std::cerr << "  EXCEPTION " << e.what() << "\n";
        }
        std::cout << (failures == before ? "[ OK ] " : "[FAIL] ") << name << "\n";
    }
    std::cout << "\n" << checks << " checks, " << failures << " failure(s)\n";
    return failures == 0 ? 0 : 1;
}
