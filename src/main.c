#include <errno.h>   //para errno
#include <fcntl.h>   // para open()
#include <pthread.h> //para hilos
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    //para strcpy
#include <sys/prctl.h> //para prctl
#include <sys/wait.h>  //para waitpid
#include <time.h>
#include <unistd.h> //para sleep

#include "flags_bash.h" //flags para el bash
#include "shell_core.h" //funciones de la shell

#define MAX_ARGS 32           // para input
#define ONE_SEC 1             // segundo de sleep
#define CNF 127               // comando no encontrado
static int counterCNF = 0;    // contador de comandos invalidos
#define MAX_COMMAND_INVALID 5 // maximo de comandos invalidos permitidos
#define PERM_FILE 0644

// Ejecuta un comando sin piping "|"
void run_simple(char* cmd)
{
    char* argv[MAX_ARGS];
    int argc = 0;

    char* token = strtok(cmd, " \t\n"); // separo por espacios, tabs o saltos de linea
    while (token && argc < MAX_ARGS - 1)
    {
        argv[argc++] = token; // guardo el los argumentos
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    pid_t pid = fork();
    if (pid == 0)
    {
        execvp(argv[0], argv);
        exit(CNF); // 127, error estándar de "command not found"
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == CNF)
        // si es distinto de cero, hubo un error, y si es por comando no encontrado
        {
            printf("Comando inválido: %s\n", argv[0]);
            counterCNF++;
        }
        else
        {
            counterCNF = 0; // comando correcto, reseteo contador
        }
    }
}

// para ejecutar un comando directamente, y no romper el pipe de monitoring
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
    exit(CNF);
}

// Ejecuta dos comandos conectados por un pipe: cmd1 | cmd2
void run_pipe(char* left_cmd, char* right_cmd)
{
    int piping[2];
    if (pipe(piping) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
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

    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);

    if (WIFEXITED(status1) && WEXITSTATUS(status1) == CNF)
    {
        printf("Comando inválido: %s\n", left_cmd);
        counterCNF++;
    }

    if (WIFEXITED(status2) && WEXITSTATUS(status2) == CNF)
    {
        printf("Comando inválido: %s\n", right_cmd);
        counterCNF++;
    }
    if ((WIFEXITED(status1) && WEXITSTATUS(status1) != CNF) && (WIFEXITED(status2) && WEXITSTATUS(status2) != CNF))
    {
        counterCNF = 0; // ambos comandos fueron correctos, reseteo contador
    }
}

void run_redirect(char* cmd)
{
    char* args[MAX_ARGS];
    char* filename = NULL;
    int append = 0;
    int input_redirect = 0;
    char* out = strstr(cmd, ">>"); // para modo append
    if (out)
    {
        append = 1;
        *out = '\0';
        filename = out + 2; // incrementa el puntero despues de >>
    }
    else if ((out = strchr(cmd, '>')))
    {
        *out = '\0';
        filename = out + 1;
    }
    char* in = strchr(cmd, '<');
    if (in)
    {
        *in = '\0';
        filename = in + 1;
        input_redirect = 1;
    }
    if (filename) // limpia espacios, para obtener el nombre del archivo
    {
        while (*filename == ' ' || *filename == '\t')
            filename++;
    }
    // parsea el comando principal (antes de > o <)
    int argc = 0;
    char* token = strtok(cmd, " \t\n");
    while (token && argc < MAX_ARGS - 1)
    {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;

    pid_t pid = fork();
    if (pid == 0)
    {
        // redirección de salida >
        if (out)
        {
            int fd;
            if (append)
                fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, PERM_FILE);
            else
                fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, PERM_FILE);

            if (fd < 0)
            {
                perror("open"); // error al abrir el archivo
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        // redirección de entrada <
        if (in)
        {
            int fd = open(filename, O_RDONLY);
            if (fd < 0)
            {
                perror("open"); // error al abrir el archivo
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        execvp(args[0], args);
        exit(CNF);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == CNF)
        {
            printf("Comando inválido: %s\n", args[0]);
            counterCNF++;
        }
        else
        {
            counterCNF = 0; // comando correcto, reseteo contador
        }
    }
}

void run_command(char* input)
{
    char* pipe_pos = strchr(input, '|');
    if (pipe_pos)
    {
        *pipe_pos = '\0';
        char* left = input;
        char* right = pipe_pos + 1;
        run_pipe(left, right);
        return;
    }
    // redirección de salida o entrada
    if (strchr(input, '>') || strchr(input, '<'))
    {
        run_redirect(input);
        return;
    }
    run_simple(input); // comando simple
}

int main(int argc, char* argv[])
{
    int flag = flag_entry(argc, argv);
    if (flag != 0)
    {
        printf("Hay un error en flags_bash.\n");
        return 0;
    }
    signal(SIGINT, handler); // ctl-c handler
    char opcion[MAX_ARGS];   // vector para la opcion ingresada
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
        printf("\n>> ");
        if (fgets(opcion, sizeof(opcion), stdin) != NULL)
        {
            int len = strlen(opcion);
            if (len > 0 && opcion[len - 1] == '\n')
            {
                opcion[len - 1] = '\0'; // Elimina el salto de linea (el enter)
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
        // comparo las opciones ingresadas
        if (strcmp(opcion, "start") == 0)
        {
            startMonitoring();
            counterCNF = 0;
        }
        else if (strcmp(opcion, "stop") == 0)
        {
            stopMonitoring();
            counterCNF = 0;
        }
        else if (strcmp(opcion, "psnode") == 0)
        {
            showStatus();
            counterCNF = 0;
        }
        else if (strcmp(opcion, "status") == 0)
        {
            showLastMetric();
            counterCNF = 0;
        }
        else if (strcmp(opcion, "exit") == 0)
        {
            exitProgram();
            break;
        }
        else
        {
            printf("\n"); // dejo un renglón para que se lea mejor
            run_command(opcion);
        }
        // verifico si se supero el maximo de comandos invalidos
        if (counterCNF >= MAX_COMMAND_INVALID)
        {
            printf("\nDemasiados intentos fallidos.\n");
            printf("Ejecute el programa con el flag --help o -h para ver instrucciones.\n");
            exitProgram();
            break;
        }
        sleep(ONE_SEC); // para que se vea mejor la interaccion
    } while (true);
    return 0;
}
