# 16-Step Drum Machine & Pattern Manager

AOOP 期末專題 — 113511166 徐偉瀞（組員 A）、112511121 邱禹峰（組員 B）

C++17 · SFML 3.0.2 · Dear ImGui 1.91.1 · ImGui-SFML 3.0

## 建置與執行（Windows + VS Code）

第三方函式庫由 CMake 在第一次設定時自動下載，**不需要手動安裝 SFML / ImGui**。

需要：Visual Studio 2022 或 Build Tools 2022（勾選「使用 C++ 的桌面開發」）、VS Code 的 **CMake Tools** 擴充。

1. VS Code 開啟 repo 資料夾。
2. `Ctrl+Shift+P` → **CMake: Select a Kit** → 選 **Visual Studio … 2022 Release - amd64**。
3. 等 CMake 跑完（第一次要下載，約 1~3 分鐘）。
4. 狀態列按 **Build**。

### 新增檔案不用改 CMakeLists.txt

CMake 會自動收集以下位置的 `.cpp`：

| 位置 | 編進 |
|---|---|
| `src/core/`、`src/export/` | `drum_core` 函式庫（不含 GUI） |
| `src/main.cpp`、`src/gui/`、`src/app/`、`src/platform/` | `DrumMachine` 主程式 |
| `tests/*.cpp` | 每個檔案各自是一個測試程式 |

`DrumMachine` 只有在 `src/main.cpp` 存在時才會建立。**第一次新增 `main.cpp` 後，請執行一次 `CMake: Configure`**（`Ctrl+Shift+P`）。

### 單元測試

狀態列選目標 `track_tests` 執行，或：

```bat
ctest --test-dir build -C Debug --output-on-failure
```

## 目前的程式結構

```
src/core/
├─ Step.h/.cpp             [A] 單一步進：ON/OFF + 力度 (0~127)
└─ InstrumentTrack.h/.cpp  [A] 單一樂器音軌：名稱、.wav、音量、16 個 Step、發聲
tests/track_tests.cpp      [A] Step / InstrumentTrack 單元測試
assets/samples/            預設音色 kick / snare / hihat / clap
```

### 分工

| 組員 A 徐偉瀞 | 組員 B 邱禹峰 |
|---|---|
| ✅ `Step`、`InstrumentTrack` | `Pattern`（管理多條 InstrumentTrack） |
| 新增 / 刪除音軌（加進 B 的 Pattern） | `Project`、播放引擎（BPM 計時） |
| 單一 Pattern 編輯畫面 | 主視窗 `main.cpp`、頂部控制列、所有 Pattern 模式 |
| 匯入 .wav（Windows 檔案總管） | `Exporter` / `JsonExporter` 存檔讀檔 |

## InstrumentTrack 使用說明（給組員 B 實作 Pattern 用）

```cpp
#include "core/InstrumentTrack.h"

std::vector<InstrumentTrack> tracks;          // Pattern 裡可以這樣存

tracks.emplace_back("Kick");                  // 建立音軌（用 emplace_back，不能複製）
InstrumentTrack& kick = tracks.back();
kick.loadWavFile("assets/samples/kick.wav");  // 成功回傳 true
kick.setVolume(80.f);                         // 0~100
kick.setStepActive(0, true);                  // 第 1 步打開
kick.toggleStep(8);                           // 切換第 9 步

// 播放引擎每走到一步，就讓每條音軌播放這一步：
for (InstrumentTrack& t : tracks) {
    t.playStep(currentStep);                  // 該步 ON 且有音色才會發聲
}
```

### 完整介面

| 函式 | 說明 |
|---|---|
| `InstrumentTrack(std::string name)` | 建立音軌 |
| `getTrackName()` / `setTrackName(name)` | 名稱 |
| `bool loadWavFile(path)` | 載入音色；失敗回傳 false，舊音色保留。路徑為 UTF-8，中文可 |
| `hasSample()` / `getWavFilePath()` | 是否已載入、檔案路徑（存檔用） |
| `setVolume(v)` / `getVolume()` | 音量 0~100，超出會被夾住 |
| `toggleStep(i)` / `setStepActive(i, on)` / `isStepActive(i)` | 步進開關，`i` 為 0~15 |
| `getStep(i)` | 取得 `Step&`，可用 `getVelocity()` / `setVelocity(0~127)` |
| `clearSteps()` / `getActiveStepCount()` | 清除全部步進 / 計算 ON 的步數 |
| `bool playStep(i)` | 播放第 `i` 步；回傳是否真的發聲（音量 = 音量 × 力度） |
| `preview()` / `stop()` | 不看步進直接試聽 / 停止 |
| `InstrumentTrack::kStepCount` | 常數 16 |

### 注意事項

- **InstrumentTrack 只能移動、不能複製**（move-only），因為 `sf::Sound` 綁定在 `sf::SoundBuffer` 的位址上。
  - 放進 `std::vector` 沒問題，請用 `emplace_back` 或 `std::move`。
  - 傳給函式請用參考：`void f(InstrumentTrack& t)`。
  - 包含它的類別（例如 Pattern）也會自動變成 move-only，這是正常的。
- `std::vector` 新增元素後，之前拿到的 `InstrumentTrack&` 可能失效，請用 index 重新取得。
- 步進 index 超出 0~15 會丟 `std::out_of_range`，方便抓出播放引擎算錯步數的 bug。
- 音色檔路徑是相對於執行檔的位置；`assets/` 會在建置後自動複製到執行檔旁邊。
