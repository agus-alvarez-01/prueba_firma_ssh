#include <errno.h>   //para errno
#include <pthread.h> //para hilos
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    //para strcpy
#include <sys/prctl.h> //para prctl

#include <sys/wait.h> //para waitpid
#include <time.h>
#include <unistd.h> //para sleep

#include <create_directory.h> //crear directorio si no existe
#include <flags_bash.h>       //flags para la bash

#define PATH_DIR "/var/log/monitoreo" // directorio para el archivo de actions
#define LOG_PATH "/var/log/monitoreo/actions.log"

static int fd[2];      // comunicación con proceso monitoring
static int logpipe[2]; // comunicación con hilo logger
pid_t pid_monitoring;  // PID del proceso de monitoreo
bool monitoring_active = false;
time_t t_start;            // tiempo de inicio de la shell
time_t t_monitoring_start; // tiempo de inicio del monitoreo

static void handler(int sig) // ctl-c handler
{
    printf("\n\nApretaste ctrl+c, se va detener la interaccion con la shell en 3 segundos.\n");
    sleep(2);
    printf("1 segundos.\n");
    sleep(1);
    printf("End Shell.\n");
    exit(0);
}

void ontime(time_t t, char* buffer, size_t size)
{
    time_t now = time(NULL);
    time_t diff = now - t;
    int hours = diff / 3600;
    int minutes = (diff % 3600) / 60;
    int seconds = diff % 60;
    snprintf(buffer, size, "%02d:%02d:%02d", hours, minutes, seconds);
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
            t_monitoring_start = time(NULL);
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
        int n = read(fd[0], &lastMetric, sizeof(lastMetric)); // se bloquea hasta leer
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

// t
void showStatus()
{
    char tiempo[64];
    ontime(t_start, tiempo, sizeof(tiempo));
    printf("\n\nLa shell esta activa hace: %s\n", tiempo);
    printf("PID de shell: %d\n", getpid());
    if (monitoring_active)
    {
        char tiempo_monitoring[64];
        ontime(t_monitoring_start, tiempo_monitoring, sizeof(tiempo_monitoring));
        printf("El monitoreo esta activo hace: %s\n", tiempo_monitoring);
        printf("PID de Monitoring: %d\n\n", pid_monitoring);
    }
    else
    {
        printf("El monitoreo no esta activo.\n\n");
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
    char opcion[32]; // se espera solo un caracter, pero para que tambien guarde opciones incorrectas
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

int main(int argc, char* argv[])
{
    int aux = option_entry(argc, argv);
    if (aux != 0)
    {
        printf("Hay un error en flags_bash.\n");
        return 0;
    }
    signal(SIGINT, handler); // ctl-c handler
    char opcion[32];         // vector para la opcion ingresada
    t_start = time(NULL);    // tiempo de inicio de la shell
    pthread_t t;             // hilo logger
    if (pipe(logpipe) == -1) // crea pipe para logger
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pthread_create(&t, NULL, loggerDaemon, NULL); // crea hilo daemon
    pthread_detach(t);                            // daemon puro
    do
    {
        // mostrar opciones de interaccion
        printf("=======================================\n");
        printf("---------Opciones del programa---------\n");
        printf("\"i\"-> Iniciar monitoreo de metricas.\n");
        printf("\"s\"-> Stop monitoreo de metricas.\n");
        printf("\"u\"-> Mostrar ultima metrica obtenida.\n");
        printf("\"t\"-> Mostrar estado de la shell y del monitoreo.\n");
        printf("\"p\"-> Ejecutar comando \"ps aux\".\n");
        printf("\"l\"-> Ejecutar comando \"ls\".\n");
        printf("\"e\"-> Salir del programa.\n");
        printf("=======================================\n");
        printf("\nIngrese opcion:  ");
        if (fgets(opcion, sizeof(opcion), stdin) != NULL)
        {
            int len = strlen(opcion);
            if (len > 0 && opcion[len - 1] == '\n')
            {
                opcion[len - 1] = '\0'; // Elimina el salto de linea si esta presente
                len--;
            }
        }
        printf("\nSu opcion fue: %s\n", opcion);
        sleep(1); // para que se vea mejor la interaccion
        // las opciones se envian al logger, sean o no de un caracter, se registra todo
        int n = write(logpipe[1], opcion, strlen(opcion)); // enviar opcion al logger
        if (n == -1)
        {
            perror("write to logger pipe");
            return 0;
        }
        if (strlen(opcion) != 1)
        {
            printf("\nTiene que ingresar solo un caracter.\n");
            printf("-----------------------------------\n\n");
            sleep(2);
            continue;
        }
        switch (opcion[0])
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
        case 't':
            showStatus();
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
            printf("\nOpcion no valida. Intente de nuevo.\n");
            printf("-----------------------------------\n");
            break;
        }
        sleep(2); // para que se vea mejor la interaccion
    } while (opcion[0] != 'e');

    return 0;
}
