/*
 * ctfhunter.c - CTF Flag & String Finder  (case-insensitive)
 * Compiles on Windows and Linux (plain C, C99)
 *
 * Usage:
 *   ctfhunter <start_dir> <search_string>
 *
 *   <start_dir>     : root directory to search (e.g. "/" or "C:/")
 *   <search_string> : string to search for inside all files (e.g. "HTB{")
 *
 * Both the filename match and the in-file string search are case-insensitive.
 */

#ifndef _WIN32
#  define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
  #include <windows.h>
  #define PATH_SEP     '\\'
  #define MAX_PATH_LEN MAX_PATH
#else
  #include <dirent.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #define PATH_SEP     '/'
  #define MAX_PATH_LEN 4096
#endif

/* --------------------------------------------------------------------------
 * Portable case-insensitive string compare
 * ------------------------------------------------------------------------*/
static int str_icmp(const char *a, const char *b)
{
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* --------------------------------------------------------------------------
 * Return a new heap-allocated lower-cased copy of s. Caller must free().
 * ------------------------------------------------------------------------*/
static char *str_dup_lower(const char *s)
{
    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    for (size_t i = 0; i <= len; i++)
        out[i] = (char)tolower((unsigned char)s[i]);
    return out;
}

/* --------------------------------------------------------------------------
 * Utility: build  parent/child  into out (size = MAX_PATH_LEN)
 * ------------------------------------------------------------------------*/
static void build_path(const char *parent, const char *child, char *out)
{
    snprintf(out, MAX_PATH_LEN, "%s%c%s", parent, PATH_SEP, child);
}

/* --------------------------------------------------------------------------
 * Print file contents to stdout
 * ------------------------------------------------------------------------*/
static void print_file_contents(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("  [!] Could not open file for reading.\n");
        return;
    }
    printf("  --- Contents of %s ---\n", path);
    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf) - 1, f)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    printf("\n  --- End of file ---\n");
    fclose(f);
}

/* --------------------------------------------------------------------------
 * Case-insensitive file content scan using a sliding window.
 * needle_lc must already be lower-cased by the caller.
 * Returns 1 if found, 0 otherwise.
 * ------------------------------------------------------------------------*/
static int file_contains_string(const char *path, const char *needle_lc)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    size_t nlen = strlen(needle_lc);
    if (nlen == 0) { fclose(f); return 1; }

    size_t bufsize = 65536;
    size_t overlap = nlen - 1;

    /* raw read buffer + lower-cased version, both prefixed by overlap area */
    char *buf  = (char *)malloc(bufsize + nlen);
    char *lbuf = (char *)malloc(bufsize + nlen);
    if (!buf || !lbuf) { free(buf); free(lbuf); fclose(f); return 0; }

    memset(buf,  0, overlap);
    memset(lbuf, 0, overlap);

    int found = 0;

    while (!found) {
        size_t n = fread(buf + overlap, 1, bufsize, f);
        if (n == 0) break;
        size_t total = overlap + n;

        /* Lower-case the entire window */
        for (size_t i = 0; i < total; i++)
            lbuf[i] = (char)tolower((unsigned char)buf[i]);

        /* Search for needle_lc in lbuf */
        for (size_t i = 0; i + nlen <= total; i++) {
            if (memcmp(lbuf + i, needle_lc, nlen) == 0) {
                found = 1;
                break;
            }
        }

        /* Slide overlap to the front for next iteration */
        if (!found && n == bufsize) {
            memmove(buf,  buf  + total - overlap, overlap);
            memmove(lbuf, lbuf + total - overlap, overlap);
        } else {
            break;
        }
    }

    free(buf);
    free(lbuf);
    fclose(f);
    return found;
}

/* --------------------------------------------------------------------------
 * Target filenames for function 1  (comparison is case-insensitive)
 * ------------------------------------------------------------------------*/
static const char *TARGET_NAMES[] = {
    "flag.txt", "root.txt", "user.txt", "proof.txt", NULL
};

static int is_target_file(const char *name)
{
    for (int i = 0; TARGET_NAMES[i]; i++) {
        if (str_icmp(name, TARGET_NAMES[i]) == 0) return 1;
    }
    return 0;
}

/* ==========================================================================
 * RECURSIVE DIRECTORY WALKER
 * ========================================================================*/

#ifdef _WIN32
/* ---- Windows: FindFirstFile / FindNextFile --------------------------------*/

static void walk(const char *dir, const char *needle_lc)
{
    char pattern[MAX_PATH_LEN];
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        const char *name = fd.cFileName;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char full[MAX_PATH_LEN];
        build_path(dir, name, full);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            walk(full, needle_lc);
        } else {
            /* Function 1: flag filenames (case-insensitive) */
            if (is_target_file(name)) {
                printf("[FLAG FILE FOUND] %s\n", full);
                print_file_contents(full);
            }
            /* Function 2: string search inside file (case-insensitive) */
            if (needle_lc && needle_lc[0]) {
                if (file_contains_string(full, needle_lc)) {
                    printf("[STRING MATCH]   %s\n", full);
                }
            }
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
}

#else
/* ---- POSIX: opendir / readdir --------------------------------------------*/

static void walk(const char *dir, const char *needle_lc)
{
    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        const char *name = entry->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char full[MAX_PATH_LEN];
        build_path(dir, name, full);

        struct stat st;
        if (lstat(full, &st) != 0) continue;  /* skip on error  */
        if (S_ISLNK(st.st_mode))   continue;  /* skip symlinks  */

        if (S_ISDIR(st.st_mode)) {
            walk(full, needle_lc);
        } else if (S_ISREG(st.st_mode)) {
            /* Function 1: flag filenames (case-insensitive) */
            if (is_target_file(name)) {
                printf("[FLAG FILE FOUND] %s\n", full);
                print_file_contents(full);
            }
            /* Function 2: string search inside file (case-insensitive) */
            if (needle_lc && needle_lc[0]) {
                if (file_contains_string(full, needle_lc)) {
                    printf("[STRING MATCH]   %s\n", full);
                }
            }
        }
    }

    closedir(d);
}

#endif

/* ==========================================================================
 * MAIN
 * ========================================================================*/

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr,
            "Usage: %s <start_dir> <search_string>\n"
            "  <start_dir>     : directory to search recursively\n"
            "  <search_string> : string to look for inside files (e.g. \"HTB{\")\n"
            "  Both filename matching and content search are case-insensitive.\n",
            argv[0]);
        return 1;
    }

    const char *start_dir = argv[1];

    /* Lower-case the needle once; reuse for every file scan */
    char *needle_lc = str_dup_lower(argv[2]);
    if (!needle_lc) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    printf("=== CTF Hunter ===\n");
    printf("Start dir    : %s\n", start_dir);
    printf("Search string: %s  (case-insensitive)\n", argv[2]);
    printf("Target files : flag.txt / root.txt / user.txt / proof.txt  (case-insensitive)\n");
    printf("==================\n\n");

    walk(start_dir, needle_lc);

    free(needle_lc);

    printf("\n=== Done ===\n");
    return 0;
}
                 
