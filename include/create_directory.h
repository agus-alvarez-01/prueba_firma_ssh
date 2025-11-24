#include <stdio.h>
#include <sys/stat.h> //para mkdir

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
