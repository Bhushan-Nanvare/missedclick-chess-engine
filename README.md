# MissedClick Chess Engine (author: Bhushan Nanvare)

A UCI-compliant chess engine written in C++17, compatible with **Arena**, Cute Chess,
and any UCI-aware chess GUI.

## Features

- **Bitboard board representation** with magic bitboards for fast slider attacks
- **Negamax** with alpha-beta pruning and iterative deepening
- **Quiescence search** to avoid the horizon effect
- **Transposition table** (Zobrist hashing, ~1M entries)
- **Move ordering**: MVV-LVA captures, killer moves, history heuristics, PV-move
- **Pruning**: null-move pruning, late-move reductions, check extensions, mate-distance pruning
- **Evaluation**: material + piece-square tables, bishop-pair bonus
- Full **UCI protocol**: `uci`, `isready`, `ucinewgame`, `position`, `go`, `stop`, `quit`

---

## Project Structure

```
MissedClick/
├── src/
│   ├── board/     Board representation, types, Zobrist hashing
│   ├── attacks/   Attack generation (magic bitboards)
│   ├── moves/     Move generation, make/undo move
│   ├── eval/      Evaluation (material + PST)
│   ├── search/    Negamax, transposition table, move ordering
│   ├── engine/    UCI handler, main entry point
│   └── utils/     Platform utilities (timing, stdin polling)
├── Makefile
├── compile_windows.bat   ← double-click to build on Windows
└── README.md
```

---

## Building on Linux / Mac

```bash
# Release build
make

# Clean
make clean
```

Requires **g++ with C++17 support** (install via `sudo apt install g++` or `brew install gcc`).

---

## Building on Windows (produces MissedClick.exe)

### Option A — One-click (recommended)

1. Install **MSYS2**: https://www.msys2.org/  
   Open MSYS2 MinGW64 shell and run once:
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   ```
2. Double-click **`compile_windows.bat`** inside the `MissedClick` folder.  
   It auto-detects g++ in common install locations.  
   `MissedClick.exe` will appear in the same folder.

### Option B — MSYS2 shell

```bash
# Inside MSYS2 MinGW64 shell, in the MissedClick folder:
g++ -std=c++17 -O2 -Isrc -static-libgcc -static-libstdc++ -static \
    src/board/types.cpp src/board/board.cpp src/board/hash.cpp \
    src/attacks/attacks.cpp src/moves/movegen.cpp src/moves/makemove.cpp \
    src/eval/evaluate.cpp src/eval/evaluation_enhanced.cpp \
    src/search/tt.cpp src/search/moveorder.cpp src/search/search.cpp \
    src/search/search_enhanced.cpp src/utils/utils.cpp \
    src/engine/uci.cpp src/engine/main.cpp \
    -o MissedClick.exe
```

### Option C — Cross-compile from Linux (needs mingw-w64)

```bash
sudo apt install mingw-w64
make windows     # produces MissedClick.exe
```

---

## Adding to Arena

1. Open Arena → **Engines → Install New Engine**
2. Browse to `MissedClick.exe` (Windows) or `MissedClick` (Linux)
3. Select **UCI** as the protocol
4. Click OK — Arena will detect it automatically

---

## Quick UCI smoke-test

```bash
printf "uci\nisready\nposition startpos\ngo movetime 1000\nquit\n" | ./MissedClick
```

Expected output includes:
```
uciok
readyok
info depth 1 score cp ...
...
bestmove <move>
```

---

## Known-correct positions

| Position                                          | Expected bestmove |
|---------------------------------------------------|-------------------|
| `6k1/5ppp/8/8/8/8/8/R5K1 w - -` (mate in 1)     | `a1a8`            |
| Ruy Lopez after 1.e4 e5 2.Nf3 Nc6 3.Bb5         | `a7a6` / `g8f6`   |
| Scholar's Mate setup                              | `h5f7` (Qxf7#)    |
