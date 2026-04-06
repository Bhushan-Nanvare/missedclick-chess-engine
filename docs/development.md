# Development notes

- **Source layout**: implementation lives under `src/` in modules (`board`, `attacks`, `moves`, `eval`, `search`, `engine`, `utils`). See the main [README](../README.md) for the full tree.
- **Build**: use the project `Makefile` on Unix-like systems or `compile_windows.bat` on Windows.
- **Protocol**: the engine speaks UCI; test with Arena, Cute Chess, or any UCI GUI.
