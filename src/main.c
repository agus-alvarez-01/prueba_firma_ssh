#include <errno.h>   //para errno
#include <pthread.h> //para hilos
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    //para strcpy
#include <sys/prctl.h> //para prctl
#include <sys/stat.h>  //para mkdir
#include <sys/wait.h>  //para waitpid
#include <time.h>
#include <unistd.h> //para sleep

#include <flags_bash.h> //flags para la bash

#define PATH_DIR "/var/log/monitoreo" // directorio para el archivo de actions
#define LOG_PATH "/var/log/monitoreo/actions.log"

static int fd[2];     // comunicación
pid_t pid_monitoring; // PID del proceso de monitoreo
bool monitoring_active = false;

static void handler(int sig) // momentaneo en este main
{
    printf("\n\nApretaste ctrl+c, se va detener la interaccion con la shell en 3 segundos.\n");
    sleep(2);
    printf("1 segundos.\n");
    sleep(1);
    printf("End Shell.\n");
    exit(0);
}

void createDirectoryIfNotExists(const char* path)
{
    struct stat st = {0};
    if (stat(path, &st) == -1)
    {
        if (mkdir(path, 0755) == -1)
        {
            perror("Error al crear directorio");
        }
        else
        {
            printf("Directorio %s creado.\n", path);
        }
    }
}

// i
void startMonitoring()
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
            char fd_str[256];
            sprintf(fd_str, "%d", fd[1]);
            execl("./build/monitoring", "./monitoring", fd_str, NULL);
            perror("execl");
            exit(1);
        }
        else
        {
            printf("\nHaz iniciado la obtencion de metricas.\n\n");
            close(fd[1]); // padre solo LEE
            monitoring_active = true;
        }
    }
    else
    {
        printf("\nEl monitoreo ya esta activo!.\n\n");
    }
}

// s
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
        printf("\nHas parado la obtencion de metricas.\n\n");
    }
    else
    {
        printf("\nEl monitoreo no esta activo.\n\n");
    }
}

// u
void showLastMetric()
{
    if (monitoring_active)
    {
        kill(pid_monitoring, SIGUSR1); // pedir dato
        char lastMetric[256];
        ssize_t n = read(fd[0], &lastMetric, sizeof(lastMetric)); // se bloquea hasta leer
        if (n == -1)
        {
            perror("read");
            return;
        }
        printf("\nBuscando ultima metrica ...\n\n");
        sleep(2);
        printf("\nLa ultima metrica es: %s\n\n", lastMetric);
        lastMetric[0] = '\0'; // limpiar buffer
    }
    else
    {
        printf("\nEl monitoreo no esta activo. No se puede obtener la ultima metrica.\n\n");
    }
}

// p
void runPS()
{
    printf("\nEjecutando comando 'ps aux' ...\n\n");
    sleep(1);
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return;
    }
    if (pid == 0)
    {
        char* argv[] = {"ps", "aux", NULL};
        execvp("ps", argv);
        perror("execvp");
        exit(1); // mata al hijo si execvp falló
    }
    waitpid(pid, NULL, 0); // Espera que el hijo muera
    sleep(1);
    printf("\nFin del comando 'ps aux'.\n\n");
}

// l
void runLS()
{
    printf("\nEjecutando comando 'ls' ...\n\n");
    sleep(1);
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return;
    }
    if (pid == 0)
    {
        char* argv[] = {"ls", NULL};
        execvp("ls", argv);
        perror("execvp");
        exit(1); // mata al hijo si execvp falló
    }
    waitpid(pid, NULL, 0); // Espera que el hijo muera
    sleep(1);
    printf("\nFin del comando 'ls'.\n\n");
}

void saveOptions(char opcion)
{
    // Abro en modo append para guardar la opcion
    FILE* outACTIONS = fopen(LOG_PATH, "a");
    if (outACTIONS == NULL)
    {
        perror("Error abriendo archivo de salida");
        exit(EXIT_FAILURE);
    }
    time_t t = time(NULL);
    fprintf(outACTIONS, "%s : Opcion elegida: %c\n", ctime(&t), opcion);
    fclose(outACTIONS);
}

int main(int argc, char* argv[])
{
    int aux = option_entry(argc, argv);
    if (aux != 0)
    {
        printf("Hay un error en option_bash.\n");
        return 0;
    }
    signal(SIGINT, handler); // ctl-c handler
    char opcion;

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

    do
    {
        // mostrar opciones de interaccion
        printf("=======================================\n");
        printf("---------Opciones del programa---------\n");
        printf("\"i\"-> Iniciar monitoreo de metricas.\n");
        printf("\"s\"-> Stop monitoreo de metricas.\n");
        printf("\"u\"-> Mostrar ultima metrica obtenida.\n");
        printf("\"p\"-> Ejecutar comando \"ps aux\".\n");
        printf("\"l\"-> Ejecutar comando \"ls\".\n");
        printf("\"e\"-> Salir del programa.\n");
        printf("=======================================\n");
        printf("\nIngrese opcion:  ");
        // Reads character input from the user
        if (scanf(" %c", &opcion) != 1) // el espacio antes de %c es para ignorar espacios en blanco
        {
            fprintf(stderr, "Error al leer la opción\n");
            return 1;
        }
        printf("\nSu opcion fue: %c\n", opcion);
        sleep(1); // para que se vea mejor la interaccion
        switch (opcion)
        {
        case 'i':
            startMonitoring();
            break;
        case 's':
            stopMonitoring();
            break;
        case 'u':
            showLastMetric();
            break;
        case 'p':
            runPS();
            break;
        case 'l':
            runLS();
            break;
        case 'e':
            printf("\nEXIT: Saliendo del programa...\n");
            break;
        default:
            printf("Opcion no valida. Intente de nuevo.\n");
            printf("-----------------------------------\n");
            break;
        }
        saveOptions(opcion); // guardar opciones en var/log/monitoreo/actions.log
        sleep(2);            // para que se vea mejor la interaccion
    } while (opcion != 'e');

    close(fd[0]); // cierra LECTURA
    close(fd[1]); // cierra ESCRITURA

    return 0;
}
