# Benchmarking & Elo (honest)

- Nodes/s: `./build/chessclaw bench 12` (expect ~1.0-1.5M nps single-thread on i5-1335U; varies with position/TT).
- Perft: `./build/chessclaw-perft` (6 required positions) and `./build/chessclaw perft 6` (119060324).
- Matches: play `chessclaw` vs reference (e.g., without NNUE or Stockfish with `Skill Level`/`nodes`) at fixed time (e.g., 60+0.6, 200 games), record W/D/L, compute `eloFromScore` (400*log10(s/(1-s))) with error `400*sqrt(s(1-s)/n)/(s(1-s)ln10)`. Report as ESTIMATE with n and TC.
- Never report Elo without games. Label every estimate.
