#include "create_directory.h"
#define PERM_DIR 0755

void createDirectoryIfNotExists(const char* path)
{
    struct stat st = {0};
    if (stat(path, &st) == -1)
    {
        if (mkdir(path, PERM_DIR) == -1)
        {
            perror("Error al crear directorio");
        }
        else
        {
            printf("Directorio %s creado.\n", path);
        }
    }
}
