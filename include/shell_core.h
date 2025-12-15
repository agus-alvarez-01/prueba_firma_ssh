#ifndef SHELL_CORE_H
#define SHELL_CORE_H

#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

// Globals shared between main and test code
extern int fd[2];            // comunicación con proceso monitoring
extern int logpipe[2];       // comunicación con hilo logger
extern pid_t pid_monitoring; // PID del proceso de monitoreo
extern bool monitoring_active;
extern time_t t_start;            // tiempo de inicio de la shell
extern time_t t_monitoring_start; // tiempo de inicio del monitoreo

// Funciones exportadas
void handler(int sig); // ctl-c handler
void ontime(time_t t, char* buffer, size_t size);
void startMonitoring();
void stopMonitoring();
void showLastMetric();
void showStatus();
void exitProgram();
void* loggerDaemon(void* arg);

#endif // SHELL_CORE_H
