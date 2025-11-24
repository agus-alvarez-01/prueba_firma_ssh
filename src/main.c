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

#include <flags_bash.h> //flags para la bash
#include <shell_core.h> //flags para la bash

#define MAX_ARGS 32
#define ONE_SEC 1 // segundo de sleep

// Ejecuta un comando sin piping "|"
void run_simple(char* cmd)
{
    char* argv[MAX_ARGS];
    int argc = 0;

    char* token = strtok(cmd, " \t\n");
    while (token && argc < MAX_ARGS - 1)
    {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;

    pid_t pid = fork();
    if (pid == 0)
    {
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}

// para ejecutar un comando directamente (sin fork), y no romper el pipe de monitoring
void exec_direct(char* cmd)
{
    char* argv[MAX_ARGS];
    int argc = 0;

    char* token = strtok(cmd, " \t\n");
    while (token && argc < MAX_ARGS - 1)
    {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;

    execvp(argv[0], argv);
    perror("execvp");
    exit(1);
}

// Ejecuta dos comandos conectados por un pipe: cmd1 | cmd2
void run_pipe(char* left_cmd, char* right_cmd)
{
    int piping[2];
    if (pipe(piping) == -1)
    {
        perror("pipe");
        exit(1);
    }
    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        // hijo 1: stdout -> pipe
        close(piping[0]);
        dup2(piping[1], STDOUT_FILENO);
        close(piping[1]);

        exec_direct(left_cmd);
    }

    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        // hijo 2: stdin <- pipe
        close(piping[1]);
        dup2(piping[0], STDIN_FILENO);
        close(piping[0]);

        exec_direct(right_cmd);
    }

    close(piping[0]);
    close(piping[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void run_command(char* input)
{
    // ¿Hay pipe?
    char* pipe_pos = strchr(input, '|');

    if (!pipe_pos)
    {
        // Comando simple
        run_simple(input);
    }
    else
    {
        // Separar comando izquierdo y derecho
        *pipe_pos = '\0';
        char* left = input;
        char* right = pipe_pos + 1;

        run_pipe(left, right);
    }
}

int main(int argc, char* argv[])
{
    int aux = flag_entry(argc, argv);
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
    int counterOpcion = 0;
    do
    {
        printf("\n>>  ");
        if (fgets(opcion, sizeof(opcion), stdin) != NULL)
        {
            int len = strlen(opcion);
            if (len > 0 && opcion[len - 1] == '\n')
            {
                opcion[len - 1] = '\0'; // Elimina el salto de linea si esta presente
                len--;
            }
        }
        // las opciones se envian al logger, sean o no de un caracter, se registra todo
        int n = write(logpipe[1], opcion, strlen(opcion)); // enviar opcion al logger
        if (n == -1)
        {
            perror("write to logger pipe");
            return 0;
        }
        if (strlen(opcion) != 1)
        {
            printf("\n"); // dejo un renglón para que se lea mejor
            run_command(opcion);
            sleep(ONE_SEC); // para que se vea mejor la interaccion
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
        case 'e':
            printf("\nEXIT: Saliendo del programa...\n");
            break;
        default:
            if (counterOpcion < 2)
            {
                counterOpcion++;
            }
            else
            {
                printf("\nOpcion no valida. Intente de nuevo.\n");
                printf("-----------------------------------\n");
                break;
            }
            sleep(ONE_SEC); // para que se vea mejor la interaccion
        }
        while (opcion[0] != 'e')
            ;

        return 0;
    }
