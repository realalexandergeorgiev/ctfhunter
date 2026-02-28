This is a simple C program that searches for specific files and directories on both Windows and Linux platforms. 
Made using Claude Code Sonnet 4.6 and 1 prompt. Amazing times we live in!


----

# 🎯 CTFHunter

A fast, recursive file scanner written in **plain C** for CTF (Capture the Flag) post-exploitation and flag hunting. Runs natively on both **Linux** and **Windows** with zero dependencies.

---

## Features

- **Flag File Detection** — automatically finds `flag.txt`, `root.txt`, `user.txt`, and `proof.txt` (case-insensitive), prints their location *and* contents, then keeps going
- **String Search** — scans every file recursively for a user-supplied string (e.g. `HTB{`, `DUCTF{`, `picoCTF{`) and reports all matching paths
- **Case-Insensitive** — both filename matching and file content search ignore case, so `FLAG.TXT` or `Flag.Txt` are caught just the same
- **Binary-Safe Scanning** — uses a sliding-window chunk reader so the search string is never missed at a buffer boundary
- **Symlink Safe** — skips symbolic links on Linux to avoid infinite loops
- **Cross-Platform** — single `.c` source file, `#ifdef`-guarded for Win32 API vs POSIX; includes a Makefile for Linux-native and Windows cross-compilation

---

## Usage

```
ctfhunter <start_dir> <search_string>
```

| Argument | Description | Example |
|---|---|---|
| `start_dir` | Root directory to search recursively | `/`, `C:/`, `/home/user` |
| `search_string` | String to find inside files (case-insensitive) | `HTB{`, `flag`, `password` |

### Examples

```bash
# Hunt from filesystem root on Linux
./ctfhunter_linux / "HTB{"

# Hunt a specific directory
./ctfhunter_linux /home/bob "secret"

# Windows — search from C drive
ctfhunter.exe C:/ "HTB{"

# Windows — narrow the scope
ctfhunter.exe C:\Users "DUCTF{"
```

### Sample Output

```
=== CTF Hunter ===
Start dir    : /home/ctf
Search string: HTB{  (case-insensitive)
Target files : flag.txt / root.txt / user.txt / proof.txt  (case-insensitive)
==================

[FLAG FILE FOUND] /home/ctf/challenge/flag.txt
  --- Contents of /home/ctf/challenge/flag.txt ---
HTB{s0m3_r34lly_c00l_fl4g}
  --- End of file ---

[STRING MATCH]   /home/ctf/notes/hints.txt
[STRING MATCH]   /home/ctf/.config/app/settings.json

=== Done ===
```

---

## Building

### Requirements

| Target | Tool |
|---|---|
| Linux binary | `gcc` |
| Windows binary (cross-compile) | `x86_64-w64-mingw32-gcc` (`mingw-w64`) |

```bash
# Install cross-compiler (Debian/Ubuntu)
sudo apt install mingw-w64

# Build both binaries
make

# Build Linux only
make linux

# Build Windows only
make windows

# Clean
make clean
```

Output binaries: `ctfhunter_linux` and `ctfhunter.exe`

### Manual Compilation

```bash
# Linux
gcc -O2 -Wall -std=c99 -o ctfhunter_linux ctfhunter.c

# Windows (cross-compile from Linux)
x86_64-w64-mingw32-gcc -O2 -Wall -std=c99 -o ctfhunter.exe ctfhunter.c

# Windows (native, with MSVC or MinGW)
cl ctfhunter.c /Fe:ctfhunter.exe
```

---

## How It Works

On each file encountered during the recursive walk, CTFHunter performs two independent checks:

1. **Filename check** — the filename is compared case-insensitively against the list of known flag filenames. On a match, the full path is printed and the file contents are dumped to stdout.

2. **Content scan** — the file is read in 64 KB chunks with a `(needle_length - 1)` byte overlap between chunks, ensuring matches that straddle a chunk boundary are never missed. Each chunk is lower-cased into a parallel buffer before comparison, keeping the search case-insensitive without modifying the file on disk.

The needle string is lower-cased once at startup and reused for every file, avoiding redundant work.

---

## Compatibility

| Platform | Architecture | Status |
|---|---|---|
| Linux | x86-64 | ✅ Tested |
| Windows 10/11 | x86-64 | ✅ Builds via mingw-w64 |
| Windows (native compile) | x86-64 | ✅ MSVC / MinGW compatible |

---

## License

MIT
