# chessclaw architecture (by vaubhav, GPL-3.0-only)

## Bitboards: byColor[2] + byType[7] + board[64]. Sliding attacks on-the-fly (no magics, honest + portable). Knight/king/pawn tables precomputed.
## Movegen: pseudo-legal then isLegal filter (make + king-attack test). Covers pins/checks/double-check/castling rights+attack tests/EP legality via make-check/promotions.
## Search: iterative deepening + aspiration (25cp, <d4 full), PVS, quiescence (stand-pat + SEE>-20 + delta-ish), null-move (R=3+d/4, verification by fail-high return), LMR log-scaled, futility d<=3, extended futility, SEE capture prune, history aging, 2 killers/ply, countermove (stored, lightly used), check extension, TT (lockless xor-key, depth-prefer), time (soft/hard + overhead + emergency clamp), lazy SMP (shared TT, main thread PV).
## Eval: material + PST + doubled/isolated + passed + king shield + mobility*2 + tempo. NNUE: 768->128x2->1, CreLU(0..255... trainer uses 0..1 float approx), INT16 quant (x64), stm-first ordering, white-perspective convert. Blend 60/40 NNUE/classical.
## TT concurrency: atomic key/data, xor validation, single-slot depth-prefer replace. No locks. TSan-clean for tested paths.
## Time: allocateTime() 1/movestogo + 0.6*inc soft, 0.25*time hard, minus overhead.
## UI: DoubleBuffer (visible-cell map, ANSI zero-width, full redraw Home, alt-screen). Board 4x2 cells, gradient mahogany/cream, Unicode pieces + shadow row. Animator: staged slide echo + flashes. Charts: block levels 0..8 + gradient. Dashboard: board + moves + Elo(ESTIMATE) + engine stats. Training dashboard: epoch/batch bars + dual loss + lr-log + ETA + 12 CPU bars.
## Training: self-play epsilon-greedy depth4/5, adjudicate |eval|>1200, result by final classical eval. Adam (0.9/0.999), BCE on blended target 0.7*result+0.3*sigmoid(score/400), clip 0.01..0.99, clip weights +-2, lr decay per epoch. Quantize each best-val epoch. Known limitation: trainer uses [W,B] order with mixed stm/white target (documented, still learns; future: store stm color per sample).
## SIMD: scalar + -march=native autovectorize; AVX2 paths via compiler. No intrinsics hand-roll (portable). x86_64/avx2/arm64 all build with NATIVE=OFF.
## Storage: recursive size check vs 10GB.
