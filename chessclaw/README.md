# chessclaw

CPU-only chess engine and NNUE self-training pipeline by **vaubhav**, licensed **GPL-3.0-only**.
Pure C++20. No Python. No GPU. CMake build.

> Strength honesty: 3300 Elo is NOT achievable under the given CPU/time/storage/self-play constraints.
> Realistic estimate for this tiny hybrid engine on the target machine: ~1800-2200 Elo (ESTIMATE) with classical eval;
> NNUE hybrid adds a small measurable gain when trained. All Elo charts are labeled ESTIMATE and derived from real self-play W/D/L.

## Quick start (Linux/Debian/WSL2)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j12
./build/chessclaw-perft            # perft suite (must print ALL PERFT PASS)
./build/chessclaw perft 6          # 119060324 nodes at depth 6
./build/chessclaw bench 12         # ~1.3M nps on i5-1335U (will vary)
printf 'position startpos\ngo depth 8\nquit\n' | ./build/chessclaw uci
./build/chessclaw play             # animated dashboard game (you are White)
./build/chessclaw train 200 4      # quick self-play + NNUE train demo
./build/chessclaw full8h           # FULL 8-hour run: 1500 games + 12 epochs (budget-enforced)
./build/chessclaw storage          # check <10GB budget
```

## Windows (MSYS2 MinGW-w64)
```sh
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCHESSCLAW_ENABLE_NATIVE=OFF
cmake --build build -j
.\build\chessclaw.exe uci
```
Or MSVC: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCHESSCLAW_ENABLE_NATIVE=OFF` then build.

## macOS (Apple Clang)
```sh
xcode-select --install
brew install cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCHESSCLAW_ENABLE_NATIVE=OFF
cmake --build build -j
./build/chessclaw uci
```
Apple Silicon cross: `-DCMAKE_OSX_ARCHITECTURES=arm64` (native Clang handles it).

## Android Termux (clang)
```sh
pkg install clang cmake make
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCHESSCLAW_ENABLE_NATIVE=OFF
cmake --build build -j
./build/chessclaw uci
```

## UCI
- `uci`, `isready`, `ucinewgame`, `position startpos|fen ... [moves ...]`, `go depth|nodes|movetime|wtime|btime|winc|binc|movestogo|infinite`, `setoption name Hash|Threads|MoveOverhead|NNUEFile value ...`, `stop`, `quit`
- Identity: `id name chessclaw 1.0.0`, `id author vaubhav`

## Self-training (CPU-only, no Stockfish required)
- `train G E`: self-play G games (depth 4, 6 threads) -> `data/selfplay.bin`, then Adam train E epochs -> `models/chessclaw.nnue` with live dashboard.
- `full8h`: 1500 games depth 5 on 10 threads + 12 epochs batch 512, ~4.5h train budget. Total <8h on i5-1335U with margin. Storage <10GB (checker included).
- Optional Stockfish labels: if `/usr/games/stockfish` exists you can label positions externally, but the pipeline runs fully without it. Stockfish is GPLv3, compatible with GPL-3.0-only for private use; redistribution must include source/offers per GPL.
- Data format: `u32 count, per-sample {u8 fenLen, fen, i16 scoreCp, u8 result*100}`. Legal status: 100% self-generated, no external dataset required.

## Benchmarking & Elo honesty
- `bench D`: reports nodes/nps at depth D.
- Elo: `train::eloFromScore(W,D,L)` logistic 400-scale with error bars. Charts labeled ESTIMATE. Never fabricate.
- To validate: play matches vs a reference (e.g., same binary with/without NNUE, or Stockfish at limited nodes) and compute Elo from real W/D/L.

## Layout
- Left: animated Unicode board (mahogany/cream, 24-bit). Falls back to ASCII if locale lacks UTF-8.
- Right: move history (fade trail), Elo chart (ESTIMATE, blue->red), engine stats (nps/depth/eval bar/hash bar/time/threads/PV).
- Training: epoch/batch gradient bars, dual loss chart, lr log chart, pos/s, ETA (green->red), 12 CPU bars. Double-buffered, no flicker.
- Animations: slide, capture flash, promotion glow, castling (king+rook), en-passant dissolve, checkmate pulse, stalemate fade, mode transitions.
- Rendering is isolated in `src/ui/` + `include/chessclaw/ui/`; search/eval/train never call render.

## License
GPL-3.0-only. See LICENSE. Author: vaubhav. Every source file carries SPDX header. No third-party deps (only stdlib + pthread). CMake is build tooling only.

## Anime-night screensaver dashboard (pure C++20, Tokyo-night dark)

`train`, `train-show`, `full8h` now render a live procedural anime-night scene
behind the charts: night gradient, twinkling stars, glowing moon, mountain
silhouettes, neon shore lights, shimmering lake reflection, mist, drifting
sakura petals, vignette, neon title - all pure C++ ANSI 24-bit truecolor,
double-buffered, no flicker, no libraries, no downloads.

Optional photo backdrop (darkened, half-block truecolor):

```sh
# PPM only (P6/P3), pure C++ loader, no libs. Darkened to 35% for night theme.
CHESSCLAW_BG=/path/to/dark-anime.ppm ./build/chessclaw train-show
# convert any image to PPM first (outside project):
# convert input.jpg -resize 160x50 -brightness-contrast 0x-30 bg.ppm
```

On plain `xterm-256color` (your WSL2 terminal) truecolor degrades gracefully;
for full art use a truecolor terminal (Windows Terminal, WezTerm, Kitty).
Sixel/Kitty-graphics photo embedding is intentionally NOT used: it breaks on
xterm-256color/WSL2, so the portable half-block + procedural scene is honest.

Max-speed build (this machine, Alder Lake i5-1335U, 12 threads):
`-O3 -march=native -mtune=native -funroll-loops -flto`, AVX2 hand-vectorized
NNUE CreLU+dot (8x16 int16 per iter, `_mm256_madd_epi16`), TT sized to RAM,
lazy SMP on all 12 threads. PGO (extra ~5-10%):
```sh
cmake -S . -B build-pgo -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fprofile-generate"
cmake --build build-pgo -j12 && ./build-pgo/chessclaw bench 12
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fprofile-use -fprofile-correction"
cmake --build build -j12
```
