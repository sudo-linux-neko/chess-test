# chessclaw self-training (CPU-only)

Single command (8h budget):

  ./build/chessclaw full8h

Steps it runs:
1. Self-play: 1500 games, depth 5, 10 threads, epsilon 15%, random 4-8 ply openings -> data/selfplay_full.bin (~50-90k samples, <50MB).
2. Train: 12 epochs, batch 512, Adam lr 0.02 decay 0.92, 4.5h cap -> models/chessclaw.nnue (193KB).
3. Dashboard: epoch/batch bars, loss/val curves, lr log, pos/s, ETA, 12 CPU bars.
4. Validate: best-val checkpoint saved; final quant + reload.
5. Storage check vs 10GB.

Quick demo: `./build/chessclaw train 200 4` (~1-2 min).

Load net: `setoption name NNUEFile value models/chessclaw.nnue` then `go depth 10`.

Honesty: Elo chart is ESTIMATE from real W/D/L only. No interpolation without games.
