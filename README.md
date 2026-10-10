# WAFlash-ReFlexed

Clean-room Flash Player engine built with C++17 targeting WebAssembly and native C++.

## 🌐 Live Demo

**[Play on GitHub Pages →](https://clausvalcatd.github.io/WAFlash-Reflexed/)**

## ✅ Compatibility

| Game | Ruffle | WAFlash-ReFlexed |
|:-----|:-------|:-----------------|
| Hoshi Saga 1 (36 stages) | ❌ Broken ([#3615](https://github.com/ruffle-rs/ruffle/issues/3615)) | ✅ 36/36 stages |
| Hoshi Saga 2 | ⚠️ Partial | 🔄 In progress |
| Hoshi Saga 3 | ⚠️ Partial | 🔄 In progress |

## 🔨 Build

### Native (tests)
```bash
mkdir build && g++ -std=c++17 -Isrc/ src/**/*.cpp tests/test_engine.cpp -o build/test_engine -lz
./build/test_engine
```

### WebAssembly
```bash
# Install Emscripten first: https://emscripten.org/
make -f Makefile.emscripten
# Output: dist/waflash-reflexed.wasm + dist/waflash-reflexed.js
```

### GitHub Actions
Push to main → auto builds WASM → auto deploys to Pages 🚀

## Enable GitHub Pages

1. Go to repo Settings → Pages
2. Source: GitHub Actions
3. Push to main → workflow runs automatically
4. Live at: https://clausvalcatd.github.io/WAFlash-Reflexed/
