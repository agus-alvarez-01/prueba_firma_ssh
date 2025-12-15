#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "create_directory.h"
#include "shell_core.h"

#define PATH_DIR "/var/log/monitoreo" // directorio para el archivo de actions
#define LOG_PATH "/var/log/monitoreo/actions.log"
#define BUFFER_LAST_METRIC 256
#define BUFFER_TIME 64
#define MAX_ARGS 32
#define ONE_SEC 1 // segundo de sleep

int fd[2];                 // comunicación con proceso monitoring
int logpipe[2];            // comunicación con hilo logger
pid_t pid_monitoring = -1; // PID del proceso de monitoreo
bool monitoring_active = false;
time_t t_start;            // tiempo de inicio de la shell
time_t t_monitoring_start; // tiempo de inicio del monitoreo

void handler(int sig) // ctl-c handler
{
    printf("\n\nApretaste ctrl+c, se va detener la interaccion con la shell...\n");
    if (pid_monitoring != -1)
    {
        kill(pid_monitoring, SIGTERM);    // terminar proceso monitoring si esta activo
        waitpid(pid_monitoring, NULL, 0); // Espera que muera el hijo para evitar que quede zombie
        sleep(ONE_SEC);
        printf("End Monitoring.\n");
    }
    sleep(ONE_SEC);
    printf("End Shell.\n");
    sleep(ONE_SEC);
    exit(EXIT_SUCCESS);
}

void ontime(time_t t, char* buffer, size_t size)
{
    time_t now = time(NULL);
    time_t diff = now - t;
    int hours = diff / 3600;
    int minutes = (diff % 3600) / 60;
    int seconds = diff % 60;
    snprintf(buffer, size, "%02dh %02dm %02ds", hours, minutes, seconds);
}

// start
void startMonitoring() // a veces anda a veces no
{
    if (!monitoring_active)
    {
        if (pipe(fd) == -1) // crea pipe
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        pid_monitoring = fork();
        if (pid_monitoring == 0)
        {
            // Proceso Monitoring.
            prctl(PR_SET_PDEATHSIG, SIGTERM); // para que el hijo muera si el padre muere
            close(fd[0]);                     // no lee
            char fd_str[BUFFER_LAST_METRIC];
            sprintf(fd_str, "%d", fd[1]);
            execl("./build/monitoring", "./monitoring", fd_str, NULL);
            perror("execl");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("\nHaz iniciado la obtencion de metricas.\n");
            close(fd[1]); // padre solo LEE
            monitoring_active = true;
            t_monitoring_start = time(NULL);
        }
    }
    else
    {
        printf("\nEl monitoreo ya esta activo!.\n");
    }
}

// stop
void stopMonitoring()
{
    if (monitoring_active)
    {
        kill(pid_monitoring, SIGTERM); // terminar proceso monitoring
        int status;
        waitpid(pid_monitoring, &status, 0); // Espera que muera el hijo para evitar que quede zombie
        close(fd[0]);
        close(fd[1]); // cerrar pipe, para cuando se inicie otra vez
        monitoring_active = false;
        pid_monitoring = -1; // reestablesco pid_monitoring
        printf("\nHas parado la obtencion de metricas.\n");
    }
    else
    {
        printf("\nEl monitoreo no esta activo.\n");
    }
}

// status
void showLastMetric()
{
    if (monitoring_active)
    {
        kill(pid_monitoring, SIGUSR1); // pedir dato
        char lastMetric[BUFFER_LAST_METRIC];
        int n = read(fd[0], &lastMetric, sizeof(lastMetric)); // se bloquea hasta leer
        if (n == -1)
        {
            perror("read");
            return;
        }
        printf("\nBuscando ultima metrica ...\n");
        sleep(ONE_SEC);
        printf("\nLa ultima metrica es: %s", lastMetric);
        lastMetric[0] = '\0'; // limpio buffer
    }
    else
    {
        printf("\nEl monitoreo no esta activo. No se puede obtener la ultima metrica.\n");
    }
}

// psnode
void showStatus()
{
    char tiempo[BUFFER_TIME];
    ontime(t_start, tiempo, sizeof(tiempo));
    printf("\n\nLa shell esta activa hace: %s \n", tiempo);
    printf("PID de shell: %d\n", getpid());
    if (monitoring_active)
    {
        char tiempo_monitoring[BUFFER_TIME];
        ontime(t_monitoring_start, tiempo_monitoring, sizeof(tiempo_monitoring));
        printf("El monitoreo esta activo hace: %s\n", tiempo_monitoring);
        printf("PID de Monitoring: %d\n", pid_monitoring);
    }
    else
    {
        printf("El monitoreo no esta activo.\n");
    }
}

// exit
void exitProgram()
{
    printf("EXIT: Saliendo del programa...\n");
    if (pid_monitoring != -1)
    {
        kill(pid_monitoring, SIGTERM);    // terminar proceso monitoring si esta activo
        waitpid(pid_monitoring, NULL, 0); // Espera que muera el hijo para evitar que quede zombie
    }
    sleep(ONE_SEC); // para que se vea mejor la interaccion
}

void* loggerDaemon(void* arg)
{
    // Creo el directorio si no existe
    createDirectoryIfNotExists(PATH_DIR);
    // Abro y cierro el archivo en modo escritura para que se limpie cada vez
    FILE* outACTIONS = fopen(LOG_PATH, "w");
    if (outACTIONS == NULL)
    {
        perror("Error abriendo archivo de salida");
        exit(EXIT_FAILURE);
    }
    fclose(outACTIONS);
    // Abro en modo append para guardar la opcion
    outACTIONS = fopen(LOG_PATH, "a");
    if (outACTIONS == NULL)
    {
        perror("Error abriendo archivo de salida");
        pthread_exit(NULL);
    }
    time_t t;
    char opcion[MAX_ARGS]; // se espera solo un caracter, pero para que tambien guarde opciones incorrectas
    while (1)
    {
        int n = read(logpipe[0], opcion, sizeof(opcion));
        if (n == -1)
        {
            perror("read from logger pipe");
            return NULL;
        }
        if (n > 0)
        {
            t = time(NULL);
            opcion[n] = '\0';
            char* ts = ctime(&t);
            ts[strcspn(ts, "\n")] = '\0'; // reemplaza el salto por \0, para que quede todo en una linea
            fprintf(outACTIONS, "%s >> Se ingreso por consola: %s\n", ts, opcion);
            fflush(outACTIONS);
        }
    }
    fclose(outACTIONS);
    return NULL;
}
