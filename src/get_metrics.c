#include "get_metrics.h"

CpuStats get_cpu_stats()
{
    CpuStats stats = {0};
    // stat
    FILE* dataStat = fopen("/proc/stat", "r");
    if (dataStat == NULL)
    {
        perror("Error abriendo /proc/stat");
        stats = (CpuStats){-1, -1, -1};
        return stats;
    }
    // variables para dataStat
    long int cpu_user = -1, cpu_system = -1; // Inicializa con valor de error
    char buffer[256];
    // inicio de dataStat
    while (fgets(buffer, sizeof(buffer), dataStat) != NULL)
    {
        if (sscanf(buffer, "cpu  %ld %*s %ld", &cpu_user, &cpu_system) == 2)
        {
            break;
        }
    }
    // set de stats
    stats.cpu_user = cpu_user;
    stats.cpu_system = cpu_system;
    // Ejemplo de cpu_rate
    if (cpu_system != 0)
    {
        stats.cpu_rate = (float)(cpu_user / (float)(cpu_user + cpu_system)) * 100;
    }
    else
    {
        stats.cpu_rate = 0.0;
    }
    // cierro archivo
    fclose(dataStat);
    return stats;
}

MemoryStats get_memory_stat()
{
    MemoryStats stats = {0};
    // meminfo
    FILE* dataMeminfo = fopen("/proc/meminfo", "r");
    if (dataMeminfo == NULL)
    {
        perror("Error abriendo /proc/meminfo");
        stats = (MemoryStats){-1, -1, -1};
        return stats;
    }
    // variables para dataMeminfo
    char buffer[256];
    long mem_total = -1, mem_free = -1, mem_buffers = -1, mem_cached = -1; // Inicializa con valor de error
    long mem_used = 0;                                                     // total - libre - buffers - caché
    // inicio de dataMeminfo
    while (fgets(buffer, sizeof(buffer), dataMeminfo) != NULL)
    {
        if (sscanf(buffer, "MemTotal: %ld kB", &mem_total) == 1)
            ;
        if (sscanf(buffer, "MemFree: %ld kB", &mem_free) == 1)
            ;
        if (sscanf(buffer, "Buffers: %ld kB", &mem_buffers) == 1)
            ;
        if (sscanf(buffer, "Cached: %ld kB", &mem_cached) == 1)
        {
            break;
        }
    }
    // calculo de memoria usada
    mem_used = mem_total - mem_free - mem_buffers - mem_cached;
    // set de stats
    stats.mem_total = mem_total;
    stats.mem_free = mem_free;
    stats.mem_used = mem_used;
    // cierro archivo
    fclose(dataMeminfo);
    return stats;
}

LoadavgStats get_loadavg_stat()
{
    LoadavgStats stats = {0};
    // loadavg
    FILE* dataLoadavg = fopen("/proc/loadavg", "r");
    if (dataLoadavg == NULL)
    {
        perror("Error abriendo /proc/loadavg");
        return stats;
    }
    // variables para dataLoadavg
    char buffer[256];
    float load_1 = -1, load_5 = -1, load_15 = -1; // Inicializa con valor de error
    // inicio de dataLoadavg
    while (fgets(buffer, sizeof(buffer), dataLoadavg) != NULL)
    {
        if (sscanf(buffer, "%f %f %f", &load_1, &load_5, &load_15) == 3)
        {
            break;
        }
    }
    // set de stats
    stats.load_1 = load_1;
    stats.load_5 = load_5;
    stats.load_15 = load_15;
    // cierro archivo
    fclose(dataLoadavg);
    return stats;
}

TimestampStats get_timestamp_stat()
{
    TimestampStats stats = {0};
    // set de stats
    stats.timestamp = time(NULL);
    return stats;
}
