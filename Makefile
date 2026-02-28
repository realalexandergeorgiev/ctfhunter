# =============================================================================
# Makefile for ctfhunter
#
# Targets:
#   make            -> build both linux and windows binaries
#   make linux      -> build linux binary only (ctfhunter_linux)
#   make windows    -> build windows binary only (ctfhunter.exe)
#   make clean      -> remove all binaries
#
# Requirements for cross-compilation (Windows target on Linux):
#   sudo apt install mingw-w64
#   (Fedora/RHEL: sudo dnf install mingw64-gcc)
# =============================================================================

CC_LINUX   := gcc
CC_WIN     := x86_64-w64-mingw32-gcc

CFLAGS     := -O2 -Wall -Wextra -std=c99

SRC        := ctfhunter.c
BIN_LINUX  := ctfhunter_linux
BIN_WIN    := ctfhunter.exe

.PHONY: all linux windows clean

all: linux windows

linux: $(BIN_LINUX)

windows: $(BIN_WIN)

$(BIN_LINUX): $(SRC)
	$(CC_LINUX) $(CFLAGS) -o $@ $<
	@echo "[+] Linux binary ready: $@"

$(BIN_WIN): $(SRC)
	$(CC_WIN) $(CFLAGS) -o $@ $< -lws2_32
	@echo "[+] Windows binary ready: $@"

clean:
	rm -f $(BIN_LINUX) $(BIN_WIN)
	@echo "[+] Cleaned."
                               
