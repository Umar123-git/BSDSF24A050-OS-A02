
# BSDSF24A050-OS-A02

Operating Systems, Programming Assignment 02: a re-implementation of the Unix `ls` utility in C.

**Roll No:** BSDSF24A050

## Features (by release)

| Version | Feature |
|---|---|
| v1.0.0 | Starter: simple listing, hidden files skipped |
| v1.1.0 | `-l` long listing (permissions, links, owner, group, size, mtime, symlink targets) |
| v1.2.0 | Multi-column output, down then across, adapts to terminal width |
| v1.3.0 | `-x` horizontal (row-major) column output |
| v1.4.0 | Alphabetical sorting using `qsort()` |
| v1.5.0 | Colorized output by file type |
| v1.6.0 | `-R` recursive listing |

## Project structure

```
BSDSF24A050-OS-A02/
├── src/        ls-v1.0.0.c   (source, extended in every feature)
├── bin/        compiled executable (bin/ls)
├── obj/        object files (reserved)
├── man/        man page (bonus, reserved)
├── Makefile    build script
├── README.md   this file
└── REPORT.md   answers to the report questions
```

## Build

```bash
make          # builds bin/ls
make clean    # removes the binary
```

Requires `gcc` and `make` on Linux.

## Usage

```bash
./bin/ls [-l] [-x] [-R] [directory ...]
```

| Option | Meaning |
|---|---|
| (none) | Columns, down then across, sorted |
| `-l` | Long listing format |
| `-x` | Horizontal columns (left to right) |
| `-R` | List subdirectories recursively |

Examples:

```bash
./bin/ls
./bin/ls -l /etc
./bin/ls -x src
./bin/ls -R src
./bin/ls -lR ~/project
```

## Colors

| Color | File type |
|---|---|
| Blue | Directory |
| Green | Executable |
| Red | Archives (`.tar`, `.gz`, `.zip`) |
| Pink | Symbolic link |
| Reverse video | Device files, sockets, pipes |
| Default | Regular file |

## System calls and functions used

`lstat()`, `readdir()`, `opendir()`, `closedir()`, `readlink()`, `ioctl(TIOCGWINSZ)`, `getpwuid()`, `getgrgid()`, `ctime()`, `getopt()`, `qsort()`.

## Git workflow

- One feature branch per version (`feature-long-listing-v1.1.0`, ..., `feature-recursive-listing-v1.6.0`).
- Each branch merged into `main`, tagged with an annotated tag (`v1.1.0` to `v1.6.0`).
- Each tag has a GitHub Release with the compiled `ls` binary attached.

## Known differences from GNU ls

- Sorting uses `strcmp()` (ASCII order, uppercase before lowercase), not locale rules.
- Hidden files (names starting with `.`) are never shown (`-a` not implemented).
- `-l` always shows the time, never the year.
- Output is in columns even when piped.

## Report

See [REPORT.md](REPORT.md) for the answers to the assignment questions.
