#include <monitoring.h> //para metricas
#include <pthread.h>    //para hilos
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //para sleep

#include <errno.h>       //para errno
#include <option_bash.h> //para opciones de la bash
#include <stdbool.h>
#include <string.h>    //para strcpy
#include <sys/prctl.h> //para prctl

static int fd[2];     // comunicación Hijo1 → Hijo2
pid_t pid_monitoring; // PID del proceso de monitoreo
bool monitoring_active = false;

static void handler(int sig) // momentaneo en este main
{
    printf("\n\nApretaste ctrl+c, se va detener la obtencion de metricas en 3 segundos.\n");
    sleep(2);
    printf("1 segundos.\n");
    sleep(1);
    printf("End Metrics.\n");
    exit(0);
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
            /*Proceso Monitoring.*/
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
void stopMonitoring()
{
    if (monitoring_active)
    {
        kill(pid_monitoring, SIGTERM); // terminar proceso monitoring
        close(fd[0]);
        close(fd[1]); // cerrar pipe, para cuando se inicie otra vez
        monitoring_active = false;
        printf("\nHaz parado la obtencion de metricas.\n\n");
    }
    else
    {
        printf("\nEl monitoreo no esta activo.\n\n");
    }
}

int main(int argc, char* argv[])
{
    /* opciones para la bash*/
    int aux = option_entry(argc, argv);
    if (aux != 0)
    {
        printf("Hay un error en option_bash.\n");
        return 0;
    }

    /* ctl-c handler, post creación de hijo (si opcion es 'i')*/
    signal(SIGINT, handler);
    char opcion;
    do
    {
        // mostrar opciones de interaccion
        printf("=======================================\n");
        printf("---------Opciones del programa---------\n");
        printf("\"i\"-> Iniciar monitoreo de metricas.\n");
        printf("\"p\"-> Parar monitoreo de metricas.\n");
        printf("\"u\"-> Mostrar ultima metrica obtenida.\n");
        printf("\"s\"-> Salir del programa.\n");
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
        case 'u':
            showLastMetric();
            break;
        case 'p':
            stopMonitoring();
            break;
        case 's':
            printf("\nHaz salido del programa.\n");
            break;
        default:
            printf("Opcion no valida. Intente de nuevo.\n");
            printf("-----------------------------------\n");
            break;
        }
        sleep(2); // para que se vea mejor la interaccion
    } while (opcion != 's');

    close(fd[0]); // cierra LECTURA
    // close(fd[1]);     // cierra ESCRITURA
    return 0;
}
