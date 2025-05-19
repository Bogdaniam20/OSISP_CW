#include "deduplicator.h"

void deduplicate_files(FileList *file_list) {
    bool found_duplicates = false;
    for (size_t i = 0; i < file_list->count; i++) {
        if (file_list->files[i].inode == 0) {
            continue;
        }

        for (size_t j = i + 1; j < file_list->count; j++) {
            if (file_list->files[j].inode == 0) {
                continue;
            }

            if (is_same_file(&file_list->files[i], &file_list->files[j])) {
                if (unlink(file_list->files[j].path)) {
                    fprintf(stderr, COLOR_RED "Не удалось удалить %s: %s\n" COLOR_RESET,
                            file_list->files[j].path, strerror(errno));
                    continue;
                }

                if (link(file_list->files[i].path, file_list->files[j].path)) {
                    fprintf(stderr, COLOR_RED "Не удалось создать жесткую ссылку с %s на %s: %s\n" COLOR_RESET,
                            file_list->files[i].path, file_list->files[j].path, strerror(errno));
                    continue;
                }

                log_action("ЗАМЕЩЕНО", file_list->files[j].path, file_list->files[i].path);
                printf(COLOR_GREEN "Заменено %s на жесткую ссылку к %s\n" COLOR_RESET,
                       file_list->files[j].path, file_list->files[i].path);

                file_list->files[j].inode = 0;
                found_duplicates = true;
            }
        }
    }
    if (!found_duplicates) {
        log_action("Дубликаты не найдены", "", "");
        printf(COLOR_RED "Дубликаты не найдены в этом сканировании\n" COLOR_RESET);
    }
}

bool is_same_file(const FileInfo *a, const FileInfo *b) {
    if (a->inode == b->inode) {
        return true;
    }

    if (a->size != b->size) {
        return false;
    }

    return memcmp(a->md5, b->md5, MD5_DIGEST_LENGTH) == 0;
}