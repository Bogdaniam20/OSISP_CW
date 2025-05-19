#ifndef DEDUPLICATOR_H
#define DEDUPLICATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <libgen.h>
#include <signal.h>
#include <termios.h>
#include <poll.h>
#include <time.h>

#define MD5_DIGEST_LENGTH 16
#define MAX_PATH 4096
#define LOG_FILE "info.log"
#define SCAN_INTERVAL 10

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

#define SCAN_INTERVAL_SEC 10
#define KEY_MENU 'm'

typedef struct {
    char path[MAX_PATH];
    ino_t inode;
    off_t size;
    unsigned char md5[MD5_DIGEST_LENGTH];
} FileInfo;

typedef struct {
    FileInfo *files;
    size_t count;
    size_t capacity;
} FileList;

extern volatile sig_atomic_t stop_flag;

void set_nonblocking_mode(bool enable);
void check_for_menu(volatile sig_atomic_t *stop_flag, char **scan_dir);
void show_menu(char **scan_dir);
void handle_signal(int signum);
void init_file_list(FileList *list);
void free_file_list(FileList *list);
void add_to_file_list(FileList *list, const char *path, ino_t inode, off_t size, const unsigned char *md5);
void ensure_log_file_exists();
void log_action(const char *action, const char *file1, const char *file2);
int calculate_md5(const char *path, unsigned char *md5);
bool is_same_file(const FileInfo *a, const FileInfo *b);
void process_directory(const char *dirpath, FileList *file_list);
void deduplicate_files(FileList *file_list);

#endif