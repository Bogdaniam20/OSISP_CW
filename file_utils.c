#define _DEFAULT_SOURCE 

#include "deduplicator.h"

void process_directory(const char *dirpath, FileList *file_list) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, COLOR_RED "Не удается открыть директорию %s: %s\n" COLOR_RESET, dirpath, strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (entry->d_name[0] == '.') {
            continue;
        }

        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);

        struct stat statbuf;
        if (lstat(path, &statbuf) == -1) {
            fprintf(stderr, COLOR_RED "Не удается получить информацию о %s: %s\n" COLOR_RESET, path, strerror(errno));
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            process_directory(path, file_list);
        } else if (S_ISREG(statbuf.st_mode)) {
            if (statbuf.st_nlink == 1) {
                unsigned char md5[MD5_DIGEST_LENGTH];
                if (calculate_md5(path, md5)) {
                    fprintf(stderr, COLOR_RED "Не удалось вычислить MD5 для %s\n" COLOR_RESET, path);
                    continue;
                }
                add_to_file_list(file_list, path, statbuf.st_ino, statbuf.st_size, md5);
            }
        }
    }
    closedir(dir);
}