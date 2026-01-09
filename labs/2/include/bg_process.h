#ifndef BG_PROCESS_H
#define BG_PROCESS_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
#ifdef _WIN32
        void *process; /* HANDLE */
#else
    int pid;
#endif
    } ProcessHandle;

    /* Инициализирует ProcessHandle */
    void bgproc_init_handle(ProcessHandle *handle);

    /* Запускает процесс в фоне. args[0] — исполняемый файл, остальные — аргументы.
     * args_count - количество элементов в массиве args.
     * Возвращает 0 при успехе, -1 при ошибке.
     * err_buf - буфер для сообщения об ошибке (может быть NULL).
     * err_size - размер буфера err_buf.
     */
    int bgproc_start(const char *const *args, int args_count, ProcessHandle *handle,
                     char *err_buf, int err_size);

    /* Ожидает завершения процесса. timeout_ms < 0 — ждать бесконечно.
     * Возвращает 0 при успехе, -1 при ошибке.
     * exit_code заполняется кодом возврата процесса.
     */
    int bgproc_wait(ProcessHandle *handle, int *exit_code, int timeout_ms,
                    char *err_buf, int err_size);

    /* Освобождает ресурсы дескриптора (если осталось что-то открыто). */
    void bgproc_close(ProcessHandle *handle);

#ifdef __cplusplus
}
#endif

#endif /* BG_PROCESS_H */
