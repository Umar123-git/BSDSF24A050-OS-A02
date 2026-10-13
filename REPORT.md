# REPORT: OS Programming Assignment 02 (ls utility)

Roll No: BSDSF24A050

---

## Feature 2: v1.1.0 Long Listing (-l)

**Q1. Difference between stat() and lstat(). When is lstat() more appropriate in ls?**

- `stat()` follows symbolic links. It returns information about the file the link points to (the target).
- `lstat()` does not follow symbolic links. It returns information about the link itself.
- For regular files and directories both return the same result.
- `ls` should use `lstat()`:
  - `ls -l` must show the type `l`, the link's own size/permissions, and `name -> target`. `stat()` would hide that the entry is a link.
  - In `-R`, `lstat()` stops the program from following a symlink that points to a parent directory, which would recurse forever.
  - It still works for broken links. `stat()` fails on a link whose target is missing.

**Q2. Extracting file type and permissions from st_mode with bitwise operators and macros.**

- `st_mode` packs the file type (upper bits), the setuid/setgid/sticky bits, and 9 permission bits (user, group, other) into one integer.
- File type: mask with `S_IFMT` and compare, e.g. `(st_mode & S_IFMT) == S_IFDIR`. The macros `S_ISDIR(m)`, `S_ISREG(m)`, `S_ISLNK(m)`, etc. do this.
- Permissions: AND with a single-bit mask. `st_mode & S_IRUSR` is non-zero if the owner can read. Same for `S_IWUSR`, `S_IXUSR`, `S_IRGRP`, `S_IWGRP`, `S_IXGRP`, `S_IROTH`, `S_IWOTH`, `S_IXOTH`.
- Special bits: `S_ISUID`, `S_ISGID`, `S_ISVTX` change the x position to `s`/`S` or `t`/`T`.
- In my code, `mode_to_str()` loops over 9 masks, writing the character from `"rwxrwxrwx"` if the bit is set and `-` if not, then applies the special bits.

---

## Feature 3: v1.2.0 Column Display (down then across)

**Q1. Logic for "down then across" printing. Why is a single loop insufficient?**

- All names are read into an array first. Then:
  - `colw = maxlen + 2`
  - `cols = terminal_width / colw` (at least 1)
  - `rows = ceil(n / cols) = (n + cols - 1) / cols`
- Items fill column 1 from top to bottom, then column 2, and so on. The item at row `r`, column `c` is `names[r + c * rows]`. Printing goes row by row; for each row, the program prints one item from each column and ends the row with a newline.
- A single loop `for i in 0..n-1` prints items in array order, which is left-to-right (row-major). The next item in a row is `rows` positions away in the array, not the next one. So the loop must be two nested loops (rows, then columns) with index arithmetic. It also needs `rows` and `cols` before printing anything.

**Q2. Purpose of ioctl. Limitations of a fixed 80-column fallback.**

- `ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)` asks the terminal driver for the current window size. `w.ws_col` is the width in characters.
- The column count is computed from this width, so the layout fits the real terminal.
- With a fixed 80:
  - Wide terminals leave unused space (fewer columns than possible).
  - Narrow terminals (under 80) wrap lines and break the alignment.
  - Resizing the window has no effect.
- The 80-column fallback is still used when stdout is not a terminal (pipe or file), where `ioctl` fails.

---

## Feature 4: v1.3.0 Horizontal Display (-x)

**Q1. Complexity of vertical vs horizontal printing. Which needs more pre-calculation?**

- Horizontal (`-x`) is simpler. It keeps a running position `pos`. For each name, if `pos + colw > width`, print a newline and reset `pos`. Then print the name and add `colw`. It needs only the terminal width and `colw`.
- Vertical (down then across) needs more pre-calculation: number of columns, then number of rows (ceiling division), then the index formula `r + c * rows`, and handling of the last column having fewer items. The layout must be known completely before the first character is printed.

**Q2. Strategy for managing display modes (-l, -x, default).**

- A global enum `{ MODE_DEFAULT, MODE_LONG, MODE_HORIZ }` and an integer variable `mode`.
- `getopt()` in `main` sets `mode`: `-l` gives `MODE_LONG`, `-x` gives `MODE_HORIZ`. Without options it stays `MODE_DEFAULT`.
- `do_ls()` reads and sorts the names, then checks `mode`: `MODE_LONG` calls `do_ls_long()`, `MODE_HORIZ` calls `print_horiz()`, otherwise `print_cols()`. Parsing is separated from display, so a new mode needs only a new enum value and a new function.

---

## Feature 5: v1.4.0 Alphabetical Sort

**Q1. Why read all entries into memory before sorting? Drawbacks for millions of files.**

- `readdir()` returns entries in an arbitrary order (the order of the directory structure on disk). Sorting needs every element to compare, so no entry can be placed in its final position until all are read. The column layout also needs all names (for `maxlen` and `n`).
- Drawbacks with millions of files:
  - High memory use: one string plus one pointer per entry, and `realloc` may copy the array.
  - Latency: nothing is printed until the whole directory has been read and sorted.
  - Sorting costs O(n log n) time.
  - Memory allocation failure becomes possible.
- Unsorted `ls -f` avoids this by printing entries as they are read.

**Q2. Purpose and signature of the qsort() comparison function.**

- `qsort(base, n, size, cmp)` is a generic sort and does not know the element type. The comparison function tells it how to order two elements.
- Signature: `int cmp(const void *a, const void *b)`. It returns a negative value if `a < b`, zero if equal, and a positive value if `a > b`.
- `qsort` passes pointers to the array elements. My array holds `char *`, so each argument is a `char **`. The function casts: `*(const char * const *)a` gives the `char *` string, and then `strcmp` compares the strings.
- The parameters are `const void *` so one function type works for every data type, and `const` promises the function will not modify the elements.

---

## Feature 6: v1.5.0 Colorized Output

**Q1. How ANSI escape codes produce color. Sequence for green.**

- The terminal treats the byte `ESC` (`\033`) followed by `[`, attribute numbers, and `m` as a command to change text style, not as text to display.
- Format: `\033[<attributes>m`. `0` resets, `7` is reverse video, `31` is red, `32` is green, `34` is blue, `35` is magenta/pink.
- Green text: `printf("\033[0;32m%s\033[0m", name);` The first sequence turns green on, then the text is printed, and `\033[0m` resets to the default so later output is not colored.
- These bytes have no visible width, but `printf("%-*s")` counts them. So in my code the name is printed first, then padded by hand using `strlen(name)`.

**Q2. Permission bits that show a file is executable.**

- `S_IXUSR` (0100) for the owner, `S_IXGRP` (0010) for the group, `S_IXOTH` (0001) for others.
- A file is shown as executable if any of them is set: `st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)`.
- To check one class only, test that one bit, for example `st_mode & S_IXUSR` for the owner.
- The type must be checked first, because directories also have x bits (meaning "searchable"). That is why my code checks the directory case before the executable case.

---

## Feature 7: v1.6.0 Recursive Listing (-R)

**Q1. What is a base case? What is the base case of the recursive ls?**

- A base case is the condition in a recursive function where it returns without making another recursive call. Without it, recursion never ends.
- In `do_ls`, the base case is a directory that has no subdirectories. The loop finds no entry for which `S_ISDIR` is true, so no recursive call is made and the function returns. An unreadable directory also stops recursion (`opendir` fails, error printed, function returns).
- Recursion also terminates because every call works on a path one level deeper in a finite tree, and because `.` and `..` are never entered (they would loop forever) and symlinks are not followed (`lstat` is used).

**Q2. Why build the full path before the recursive call? What if do_ls("subdir") were called?**

- `readdir()` returns only the entry name, such as `subdir`. The program's current working directory does not change while listing.
- The path must be built as `parent_dir/subdir` (with `snprintf(path, sizeof path, "%s/%s", dir, name)`). Then both `lstat()` and `opendir()` refer to the correct location, however deep the recursion is.
- Calling `do_ls("subdir")` makes the system look for `./subdir` relative to the current working directory, not relative to `parent_dir`. It would fail with "Cannot open directory" if no such folder exists there, or worse, silently list a different directory that has the same name.
- The same full path is also needed for `lstat()` when checking whether the entry is a directory.
