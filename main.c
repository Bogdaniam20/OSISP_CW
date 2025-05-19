#include "deduplicator.h"
#include <poll.h>
#include <time.h>

#define SCAN_INTERVAL_SEC 10

void print_current_time() {
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    printf(COLOR_BLUE"Время сканирования: %02d:%02d:%02d\n"COLOR_RESET, local->tm_hour, local->tm_min, local->tm_sec);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, COLOR_RED "Использование: %s <директория>\n" COLOR_RESET, argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    ensure_log_file_exists();

    char *scan_dir = argv[1];
    set_nonblocking_mode(true);
    system("clear");
    printf(COLOR_GREEN"Программа запущена!\n"COLOR_RESET);
    printf("Нажмите 'm' для открытия меню\n");
    printf("или ожидайте сканирования...\n");

    struct timespec last_scan_time;
    clock_gettime(CLOCK_MONOTONIC, &last_scan_time);

    while (!stop_flag) {
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);

        if ((current_time.tv_sec - last_scan_time.tv_sec) >= SCAN_INTERVAL_SEC) {
            FileList file_list;
            init_file_list(&file_list);
            printf(COLOR_YELLOW "---------------------------------------------\n" COLOR_RESET);
            printf(COLOR_BLUE "Сканирование директории %s...\n" COLOR_RESET, scan_dir);
            process_directory(scan_dir, &file_list);

            printf(COLOR_BLUE "Найдено %zu файлов. Проверка на дубликаты...\n" COLOR_RESET, file_list.count+2);
            deduplicate_files(&file_list);

            free_file_list(&file_list);
            printf(COLOR_GREEN "Готово. Проверьте %s для деталей.\n" COLOR_RESET, LOG_FILE);
            print_current_time();
            printf(COLOR_YELLOW "---------------------------------------------\n" COLOR_RESET);

            last_scan_time = current_time;
        }

        struct pollfd fds = { .fd = STDIN_FILENO, .events = POLLIN };
        if (poll(&fds, 1, 100) > 0) {
            check_for_menu(&stop_flag, &scan_dir);
        }
    }

    set_nonblocking_mode(false);
    printf("\n" COLOR_YELLOW "Программа остановлена пользователем\n" COLOR_RESET);
    return EXIT_SUCCESS;
}