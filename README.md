# 16-Step Drum Machine & Pattern Manager

AOOP 期末專題 — 113511166 徐偉瀞（組員 A）、112511121 邱禹峰（組員 B）

C++17 · SFML 3.0.2 · Dear ImGui 1.91.1 · ImGui-SFML 3.0

## 建置與執行（Windows + Visual Studio 2022）

第三方函式庫由 CMake 在第一次設定時自動下載，**不需要手動安裝 SFML / ImGui**。

1. Visual Studio Installer 確認已勾選「使用 C++ 的桌面開發」（內含 CMake）。
2. VS 2022 →「開啟本機資料夾」→ 選這個 repo 的根目錄。
3. 等輸出視窗出現 `CMake generation finished`（第一次要下載，約 1~3 分鐘）。
4. 上方啟動項目選 **DrumMachine.exe** → 按 F5。

命令列也可以：

```bat
cmake -S . -B build
cmake --build build --config Release
build\bin\Release\DrumMachine.exe
```

`assets/` 會在建置後自動複製到 exe 旁邊，所以預設音色路徑 `assets/samples/*.wav` 可以直接使用。

### 單元測試

```bat
cmake --build build --config Debug --target core_tests
ctest --test-dir build -C Debug --output-on-failure
```

## 程式結構

```
src/
├─ core/
│  ├─ Step.h/.cpp             [A] 單一步進：ON/OFF + 力度 (0~127)
│  ├─ InstrumentTrack.h/.cpp  [A] 單軌：16 步、音量 (0~100)、SFML 音訊封裝
│  ├─ Pattern.h/.cpp          [A] addTrack / removeTrack / triggerStep
│  │                          [B] 名稱、總覽模式開關與其他功能
│  └─ Project.h/.cpp          [B] 骨架：BPM、播放狀態、Pattern 管理、播放引擎(TODO)
├─ export/
│  ├─ Exporter.h              [B] 抽象匯出介面（Strategy Pattern）
│  └─ JsonExporter.h/.cpp     [B] 骨架：JSON 存檔/讀檔(TODO)
├─ gui/
│  └─ PatternEditorView.h/.cpp [A] 單一 Pattern 模式畫面
├─ platform/
│  └─ FileDialog.h/.cpp       [A] Windows 檔案總管「開啟檔案」視窗
└─ main.cpp                   [B] 骨架：主視窗、頂部控制列(TODO)
tests/core_tests.cpp          核心模組單元測試
assets/samples/               預設音色 kick / snare / hihat / clap
```

## 組員 A ↔ 組員 B 介面合約

### 播放引擎（`Project::updateSequencer`）會用到

| 呼叫 | 說明 |
|---|---|
| `int Pattern::triggerStep(int step)` | 所有音軌播放第 `step` 步（0~15），回傳實際發聲的音軌數。只有「該步 ON 且已載入音色」的音軌會發聲，音量 = 軌道音量 × 力度。 |
| `void Pattern::stopAll()` | 停止這個 Pattern 所有聲音（按停止時呼叫）。 |
| `InstrumentTrack::kStepCount` | 步數常數 16。 |

### GUI（主視窗）會用到

```cpp
PatternEditorView editor(window.getNativeHandle());   // 建立一次
// 每一幀，在 ImGui::Begin/End 之間：
editor.draw(project.getActivePattern(),   // 要編輯的 Pattern
            project.getIsPlaying(),       // true → 鎖定畫面禁止修改
            project.getCurrentStep());    // 播放頭 0~15，-1 = 不顯示
```

### 存檔 / 讀檔（`JsonExporter`）會用到

| 存檔 | 讀檔 |
|---|---|
| `Pattern::getTracks()` | `Pattern::addTrack(name, wavPath)` |
| `InstrumentTrack::getTrackName()` | （名稱在 addTrack 時給） |
| `InstrumentTrack::getWavFilePath()`（UTF-8） | （路徑在 addTrack 時給，載入失敗仍會建立音軌） |
| `InstrumentTrack::getVolume()` | `InstrumentTrack::setVolume(float)` |
| `getStep(i).getIsActive()` / `getVelocity()` | `getStep(i).setActive(bool)` / `setVelocity(int)` |

### 注意事項

- `InstrumentTrack` 與 `Pattern` 是 **move-only**（不能複製），因為 `sf::Sound` 綁定在 `sf::SoundBuffer` 的位址上。放進 `std::vector` 沒問題；傳遞時請用參考或 `std::move`。
- `Pattern::addTrack` 回傳的參考在下一次 `addTrack` / `removeTrack` 之後可能失效，請用 `getTrack(index)` 重新取得。
- 步進 index 超出 0~15 會丟 `std::out_of_range`，方便抓出播放引擎算錯步數的 bug。
- 所有字串（音軌名稱、檔案路徑）都是 UTF-8，中文路徑可以正常載入。
