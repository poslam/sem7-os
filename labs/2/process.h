#ifndef PROCESS_H
#define PROCESS_H

#ifdef _WIN32
#include <windows.h>
typedef HANDLE process_handle_t;
#else
#include <sys/types.h>
typedef pid_t process_handle_t;
#endif

/**
 * @brief Запускает программу в фоновом режиме
 * @param program_path Путь к исполняемому файлу
 * @param args Аргументы командной строки (NULL-terminated array)
 * @return Дескриптор процесса или недопустимое значение при ошибке
 */
process_handle_t start_background_process(const char *program_path, char *const args[]);

/**
 * @brief Ожидает завершения процесса
 * @param handle Дескриптор процесса
 * @param exit_code Указатель для сохранения кода возврата (может быть NULL)
 * @return 0 при успехе, -1 при ошибке
 */
int wait_for_process(process_handle_t handle, int *exit_code);

/**
 * @brief Проверяет, запущен ли процесс
 * @param handle Дескриптор процесса
 * @return 1 если процесс запущен, 0 если завершен, -1 при ошибке
 */
int is_process_running(process_handle_t handle);

/**
 * @brief Закрывает дескриптор процесса (освобождает ресурсы)
 * @param handle Дескриптор процесса
 */
void close_process_handle(process_handle_t handle);

#endif /* PROCESS_H */
