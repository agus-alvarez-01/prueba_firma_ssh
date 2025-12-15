#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_PATH "/var/log/monitoreo/actions.log"

const char* program_name;
void print_help(FILE* stream, int exit_code)
{
    fprintf(stream, "Bienvenido a la shell interactiva SO1-TPs-2025\n");
    fprintf(stream, "---------Opciones del programa---------\n"
                    "\"start\"-> Iniciar monitoreo de metricas.\n"
                    "\"stop\"-> Para monitoreo de metricas.\n"
                    "\"status\"-> Mostrar ultima metrica obtenida.\n"
                    "\"psnode\"-> Muestra el estado de los procesos del programa.\n"
                    "\"exit\"-> Salir del programa.\n"
                    "---------------------------------------\n");
    fprintf(stream, "Las opciones de flags al ejecutar son: \n"
                    " -h o --help -> Muestra informacion de uso.\n"
                    " -a o --actions -> Muestra el archivo actions.log, \n"
                    " que contiene la secuencia de interacciones anteriores de la shell.\n");
    exit(exit_code);
}

void showActionsLog()
{
    FILE* outACTIONS = fopen(LOG_PATH, "r");
    if (outACTIONS == NULL)
    {
        printf("=======================================\n");
        printf("Aun no han habido interacciones.\n");
        printf("=======================================\n");
        return;
    }
    char c;
    while ((c = fgetc(outACTIONS)) != EOF)
    {
        putchar(c);
    }
    fclose(outACTIONS);
    exit(0);
}

int flag_entry(int argc, char* argv[])
{
    int next_option;
    const char* const short_options = "ha";
    const struct option long_options[] = {{"help", 0, NULL, 'h'}, {"actions", 0, NULL, 'a'}, {NULL, 0, NULL, 0}};
    program_name = argv[0];

    do
    {
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);
        switch (next_option)
        {
        case 'h':
            print_help(stdout, 0);
        case 'a':
            showActionsLog();
            break;
        case -1:
            break;
        default:
            abort();
        }
    } while (next_option != -1);
    return 0;
}
