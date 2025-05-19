#include "deduplicator.h"
#include <termios.h>

volatile sig_atomic_t stop_flag = 0;


void set_nonblocking_mode(bool enable) {
    static struct termios oldt, newt;

    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

void check_for_menu(volatile sig_atomic_t *stop_flag, char **scan_dir) {
    (void)stop_flag;
    char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        if (ch == KEY_MENU) {
            show_menu(scan_dir);
        }
    }
}

void show_menu(char **scan_dir) {
    set_nonblocking_mode(false);

    printf("\n" COLOR_YELLOW "============ MENU ============\n" COLOR_RESET);
    printf(COLOR_GREEN       "|1.Продолжить сканирование   |\n"COLOR_RESET);
    printf(COLOR_BLUE        "|2.Сменить папку сканирования|\n"COLOR_RESET);
    printf(COLOR_RED         "|3.Выход                     |\n"COLOR_RESET);
    printf("Выбор -> ");

    char option;
    scanf(" %c", &option);

    switch (option) {
        case '1':
            printf(COLOR_GREEN"Возобновление...\n"COLOR_RESET);
            break;
        case '2': {
            char new_dir[MAX_PATH];
            printf(COLOR_BLUE"Введите путь папки: "COLOR_RESET);
            scanf("%s", new_dir);
            *scan_dir = strdup(new_dir);
            printf(COLOR_BLUE"Папка для сканирования изменана на: %s\n"COLOR_RESET, *scan_dir);
            break;
        }
        case '3':
            printf(COLOR_RED"Выход...\n"COLOR_RESET);
            exit(EXIT_SUCCESS);
            break;
        default:
            printf(COLOR_RED"Неверные данные, возобновление...\n"COLOR_RESET);
    }

    set_nonblocking_mode(true);
}

void handle_signal(int signum) {
    (void)signum;
    stop_flag = 1;
}

void init_file_list(FileList *list) {
    list->capacity = 16;
    list->count = 0;
    list->files = malloc(list->capacity * sizeof(FileInfo));
    if (!list->files) {
        perror(COLOR_RED"Ошибка выделения памяти"COLOR_RESET);
        exit(EXIT_FAILURE);
    }
}

void free_file_list(FileList *list) {
    free(list->files);
    list->files = NULL;
    list->count = list->capacity = 0;
}

void add_to_file_list(FileList *list, const char *path, ino_t inode, off_t size, const unsigned char *md5) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        FileInfo *new_files = realloc(list->files, list->capacity * sizeof(FileInfo));
        if (!new_files) {
            perror(COLOR_RED"Ошибка выделения памяти"COLOR_RESET);
            exit(EXIT_FAILURE);
        }
        list->files = new_files;
    }

    strncpy(list->files[list->count].path, path, MAX_PATH - 1);
    list->files[list->count].path[MAX_PATH - 1] = '\0';
    list->files[list->count].inode = inode;
    list->files[list->count].size = size;
    memcpy(list->files[list->count].md5, md5, MD5_DIGEST_LENGTH);
    list->count++;
}

void ensure_log_file_exists() {
    struct stat st;
    if (stat(LOG_FILE, &st) == -1) {
        FILE *log = fopen(LOG_FILE, "w");
        if (!log) {
            perror(COLOR_RED"Ошибка создания файла"COLOR_RESET);
            exit(EXIT_FAILURE);
        }
        fclose(log);
        printf(COLOR_BLUE "Создан новый лог файл:%s\n" COLOR_RESET, LOG_FILE);
    }
}

void log_action(const char *action, const char *file1, const char *file2) {
    ensure_log_file_exists();

    time_t now = time(NULL);
    char timestr[20];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));

    FILE *log = fopen(LOG_FILE, "a");
    if (!log) {
        perror(COLOR_RED"Ошибка открытия файла"COLOR_RESET);
        return;
    }

    fprintf(log, "[%s] %s: %s -> %s\n", timestr, action, file1, file2);
    fclose(log);
}

int calculate_md5(const char *path, unsigned char *md5) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return -1;
    }

    EVP_MD_CTX *md5_ctx = EVP_MD_CTX_new();
    if (!md5_ctx) {
        fclose(file);
        return -1;
    }

    if (EVP_DigestInit_ex(md5_ctx, EVP_md5(), NULL) != 1) {
        EVP_MD_CTX_free(md5_ctx);
        fclose(file);
        return -1;
    }

    unsigned char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        if (EVP_DigestUpdate(md5_ctx, buffer, bytes_read) != 1) {
            EVP_MD_CTX_free(md5_ctx);
            fclose(file);
            return -1;
        }
    }

    if (EVP_DigestFinal_ex(md5_ctx, md5, NULL) != 1) {
        EVP_MD_CTX_free(md5_ctx);
        fclose(file);
        return -1;
    }

    EVP_MD_CTX_free(md5_ctx);
    fclose(file);
    return 0;
}
