/*
* Programming Assignment 02: lsv1.0.0
* This is the source file of version 1.0.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.0.0 
*       % lsv1.0.0  /home
*       $ lsv1.0.0  /home/kali/   /etc/
*/
#include <pwd.h>
#include <sys/ioctl.h>
#include <grp.h>
#include <time.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

extern int errno;

void do_ls(const char *dir);
void do_ls_long(const char *dir);
char **read_names(const char *dir, int *count, int *maxlen);
void free_names(char **names, int n);
int term_width(void);
void print_cols(char **names, int n, int maxlen);
int long_flag = 0;

int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l': long_flag = 1; break;
        default:
            fprintf(stderr, "Usage: %s [-l] [dir...]\n", argv[0]);
            exit(1);
        }
    }
    if (optind == argc)
        do_ls(".");
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i]);
            puts("");
        }
    }
    return 0;
}

void do_ls(const char *dir)
{
    if (long_flag) { do_ls_long(dir); return; }
    int n, maxlen;
    char **names = read_names(dir, &n, &maxlen);
    if (n < 0) return;                 /* opendir failed */
    print_cols(names, n, maxlen);
    free_names(names, n);
}



static void mode_to_str(mode_t m, char *p)
{
    const mode_t bits[9] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP,
                            S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
    const char *rwx = "rwxrwxrwx";

    if (S_ISDIR(m))       p[0] = 'd';
    else if (S_ISLNK(m))  p[0] = 'l';
    else if (S_ISCHR(m))  p[0] = 'c';
    else if (S_ISBLK(m))  p[0] = 'b';
    else if (S_ISFIFO(m)) p[0] = 'p';
    else if (S_ISSOCK(m)) p[0] = 's';
    else                  p[0] = '-';

    for (int i = 0; i < 9; i++)
        p[i + 1] = (m & bits[i]) ? rwx[i] : '-';

    if (m & S_ISUID) p[3] = (m & S_IXUSR) ? 's' : 'S';
    if (m & S_ISGID) p[6] = (m & S_IXGRP) ? 's' : 'S';
    if (m & S_ISVTX) p[9] = (m & S_IXOTH) ? 't' : 'T';
    p[10] = '\0';
}

static const char *user_of(uid_t u, char *buf, size_t n)
{
    struct passwd *pw = getpwuid(u);
    if (pw) return pw->pw_name;
    snprintf(buf, n, "%u", (unsigned)u);
    return buf;
}

static const char *group_of(gid_t g, char *buf, size_t n)
{
    struct group *gr = getgrgid(g);
    if (gr) return gr->gr_name;
    snprintf(buf, n, "%u", (unsigned)g);
    return buf;
}

void do_ls_long(const char *dir)
{
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    struct dirent *entry;
    struct stat st;
    char path[PATH_MAX], tmp[32], tmp2[32];
    int wl = 0, wu = 0, wg = 0, ws = 0;
    long total = 0;

    /* pass 1: column widths + total blocks */
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;
        snprintf(path, sizeof path, "%s/%s", dir, entry->d_name);
        if (lstat(path, &st) == -1) { perror(path); continue; }

        total += st.st_blocks;
        int l;
        l = snprintf(tmp, sizeof tmp, "%lu", (unsigned long)st.st_nlink);
        if (l > wl) wl = l;
        l = strlen(user_of(st.st_uid, tmp, sizeof tmp));
        if (l > wu) wu = l;
        l = strlen(group_of(st.st_gid, tmp2, sizeof tmp2));
        if (l > wg) wg = l;
        l = snprintf(tmp, sizeof tmp, "%lld", (long long)st.st_size);
        if (l > ws) ws = l;
    }

    printf("total %ld\n", total / 2);   /* st_blocks = 512B units, ls = 1K */
    rewinddir(dp);

    /* pass 2: print */
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;
        snprintf(path, sizeof path, "%s/%s", dir, entry->d_name);
        if (lstat(path, &st) == -1) continue;

        char perm[11];
        mode_to_str(st.st_mode, perm);

        printf("%s %*lu %-*s %-*s %*lld %.12s %s",
               perm,
               wl, (unsigned long)st.st_nlink,
               wu, user_of(st.st_uid, tmp, sizeof tmp),
               wg, group_of(st.st_gid, tmp2, sizeof tmp2),
               ws, (long long)st.st_size,
               ctime(&st.st_mtime) + 4,
               entry->d_name);

        if (S_ISLNK(st.st_mode))
        {
            char target[PATH_MAX];
            ssize_t n = readlink(path, target, sizeof target - 1);
            if (n != -1) { target[n] = '\0'; printf(" -> %s", target); }
        }
        putchar('\n');
    }
    closedir(dp);
}




char **read_names(const char *dir, int *count, int *maxlen)
{
    *count = 0;
    *maxlen = 0;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        *count = -1;
        return NULL;
    }

    char **names = NULL;
    int n = 0, cap = 0;
    struct dirent *entry;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;
        if (n == cap)
        {
            cap = cap ? cap * 2 : 64;
            char **tmp = realloc(names, cap * sizeof(char *));
            if (tmp == NULL) { perror("realloc"); exit(1); }
            names = tmp;
        }
        names[n] = strdup(entry->d_name);
        if (names[n] == NULL) { perror("strdup"); exit(1); }
        int l = strlen(names[n]);
        if (l > *maxlen) *maxlen = l;
        n++;
    }
    if (errno != 0) perror("readdir failed");

    closedir(dp);
    *count = n;
    return names;
}

void free_names(char **names, int n)
{
    for (int i = 0; i < n; i++) free(names[i]);
    free(names);
}

int term_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        return w.ws_col;
    return 80;
}

void print_cols(char **names, int n, int maxlen)
{
    if (n == 0) return;
    int colw = maxlen + 2;
    int cols = term_width() / colw;
    if (cols < 1) cols = 1;
    int rows = (n + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int i = r + c * rows;
            if (i >= n) break;
            printf("%-*s", colw, names[i]);
        }
        putchar('\n');
    }
}