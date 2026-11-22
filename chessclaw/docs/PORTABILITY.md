# Portability

- Linux/Debian/WSL2: g++14 + CMake 3.20, `-march=native` ON by default.
- Windows MSYS2/MinGW-w64 or MSVC: set `-DCHESSCLAW_ENABLE_NATIVE=OFF` (no -march=native). ANSI works in modern Terminal/ConPTY.
- macOS Apple Clang: `-DCHESSCLAW_ENABLE_NATIVE=OFF`. ARM64 via `-DCMAKE_OSX_ARCHITECTURES=arm64`.
- Android Termux clang: `-DCHESSCLAW_ENABLE_NATIVE=OFF`.
- No internet at build time (pure stdlib). Offline fallback: N/A (no downloads).
- SIMD: portable scalar; AVX2 via autovectorize when NATIVE=ON on x86_64. ARM NEON via compiler.
