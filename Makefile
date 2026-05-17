CXX        := g++
CXXFLAGS   := -std=c++17 -O2 -Isrc
LDFLAGS    :=

WIN_CXX    := x86_64-w64-mingw32-g++
WIN_FLAGS  := -std=c++17 -O2 -Isrc -static-libgcc -static-libstdc++ -static
WIN_LDFLAGS:= -lws2_32

TARGET     := MissedClick
WIN_TARGET := MissedClick.exe
BUILD      := build
WIN_BUILD  := build_win

SRCS := \
        src/board/types.cpp         \
        src/board/board.cpp         \
        src/board/hash.cpp          \
        src/attacks/attacks.cpp     \
        src/moves/movegen.cpp       \
        src/moves/makemove.cpp      \
        src/eval/evaluate.cpp       \
        src/eval/evaluation_enhanced.cpp \
        src/search/tt.cpp           \
        src/search/moveorder.cpp    \
        src/search/search.cpp       \
        src/search/search_enhanced.cpp \
        src/utils/utils.cpp         \
        src/engine/uci.cpp          \
        src/engine/main.cpp

OBJS     := $(patsubst src/%.cpp, $(BUILD)/%.o,     $(SRCS))
WIN_OBJS := $(patsubst src/%.cpp, $(WIN_BUILD)/%.o, $(SRCS))

# ── Default (Linux) ──────────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "[MissedClick] Linux build complete → $(TARGET)"

$(BUILD)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# ── Windows cross-compile (needs mingw-w64 on Linux) ─────────────────────────
windows: $(WIN_TARGET)

$(WIN_TARGET): $(WIN_OBJS)
	$(WIN_CXX) $(WIN_FLAGS) -o $@ $^ $(WIN_LDFLAGS)
	@echo "[MissedClick] Windows build complete → $(WIN_TARGET)"

$(WIN_BUILD)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(WIN_CXX) $(WIN_FLAGS) -c -o $@ $<

# ── Debug build ───────────────────────────────────────────────────────────────
debug: CXXFLAGS += -g -O0 -fsanitize=address,undefined -DDEBUG
debug: TARGET    = $(TARGET)_debug
debug: $(TARGET)

# ── Release (strip + LTO) ─────────────────────────────────────────────────────
release: CXXFLAGS += -DNDEBUG -flto
release: LDFLAGS  += -flto -s
release: $(TARGET)

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD) $(WIN_BUILD) $(TARGET) $(TARGET)_debug $(WIN_TARGET)

.PHONY: all debug release windows clean
