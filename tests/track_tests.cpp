// Step / InstrumentTrack 單元測試（組員 A）。執行：ctest 或直接跑 track_tests
#include "core/InstrumentTrack.h"
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

// Pattern 之後很可能用 std::vector<InstrumentTrack> 存音軌：
// vector 擴充時會搬移 (move) 音軌，音色必須仍然可用（sf::Sound 不能指向失效的 buffer）
void testTracksInVector() {
    std::vector<InstrumentTrack> tracks;
    for (int i = 0; i < 20; ++i) {   // 20 次 emplace_back 一定會觸發重新配置
        tracks.emplace_back("T" + std::to_string(i));
        tracks.back().loadWavFile((i % 2) ? kKick : kSnare);
        tracks.back().setStepActive(i % 16, true);
    }
    int played = 0;
    for (int s = 0; s < InstrumentTrack::kStepCount; ++s) {
        for (InstrumentTrack& t : tracks) {
            if (t.playStep(s)) ++played;
        }
    }
    CHECK(played == 20);

    tracks.erase(tracks.begin() + 3);   // 刪除中間一條，後面的會往前搬
    CHECK(tracks.size() == 19);
    CHECK(tracks[3].getTrackName() == "T4");
    CHECK(tracks[3].hasSample());
    CHECK(tracks[3].playStep(4));
    for (InstrumentTrack& t : tracks) t.stop();
}

}  // namespace

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"Step", testStep},
        {"InstrumentTrack steps", testTrackSteps},
        {"InstrumentTrack volume", testTrackVolume},
        {"InstrumentTrack sample", testTrackSample},
        {"InstrumentTrack move", testTrackMove},
        {"InstrumentTrack in std::vector", testTracksInVector},
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
