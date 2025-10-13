#include <sys/stat.h> //para mkdir

#include "get_metrics.h"  //obteniendo métricas del /proc
#include "prom_metrics.h" //para exponer métricas vía HTTP

#define SLEEP_SECONDS 5
#define PATH_DIR "/var/lib/monitoreo" // directorio para el archivo de metrics
#define LOG_PATH "/var/lib/monitoreo/metrics.log"

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

void updateMetrics()
{
    // Obtengo las estadísticas
    CpuStats cpu_stats = get_cpu_stats();
    MemoryStats mem_stats = get_memory_stat();
    LoadavgStats load_stats = get_loadavg_stat();
    TimestampStats timestamp_stats = get_timestamp_stat();
    // Controlo que ninguna métrica tenga error (-1)
    if (cpu_stats.cpu_user == -1 || cpu_stats.cpu_system == -1 || cpu_stats.cpu_rate == -1 ||
        mem_stats.mem_total == -1 || mem_stats.mem_free == -1 || mem_stats.mem_used == -1 || load_stats.load_1 == -1 ||
        load_stats.load_5 == -1 || load_stats.load_15 == -1 || timestamp_stats.timestamp == -1)
    {
        fprintf(stderr, "Error al obtener las métricas del sistema\n");
        return;
    }
    // Abro el archivo en modo append para escribir las métricas
    FILE* outNDJSON = fopen(LOG_PATH, "a");
    if (outNDJSON == NULL)
    {
        perror("Error abriendo archivo de salida");
        exit(EXIT_FAILURE);
    }
    // inicio del NDJSON
    fprintf(outNDJSON, "{");
    // Timestamp
    fprintf(outNDJSON, "\"timestamp\":%ld,", timestamp_stats.timestamp);
    // Memoria
    fprintf(outNDJSON, "\"mem_total\":%ld,", mem_stats.mem_total);
    fprintf(outNDJSON, "\"mem_free\":%ld,", mem_stats.mem_free);
    fprintf(outNDJSON, "\"mem_used\":%ld,", mem_stats.mem_used);
    // CPU
    fprintf(outNDJSON, "\"cpu_user\":%ld,", cpu_stats.cpu_user);
    fprintf(outNDJSON, "\"cpu_system\":%ld,", cpu_stats.cpu_system);
    fprintf(outNDJSON, "\"cpu_rate\":%.5f,", cpu_stats.cpu_rate); //.5 para ver diferencias
    // Load Average
    fprintf(outNDJSON, "\"load_1\":%.2f,", load_stats.load_1);
    fprintf(outNDJSON, "\"load_5\":%.2f,", load_stats.load_5);
    fprintf(outNDJSON, "\"load_15\":%.2f", load_stats.load_15);
    // fin del NDJSON
    fprintf(outNDJSON, "}\n");
    // cierro archivo
    fclose(outNDJSON);
    // Una vez que se actualizan las métricas en el log, se actualizan las métricas de Prometheus
    update_prom_cpu_stat(cpu_stats);
    update_prom_memory_stat(mem_stats);
    update_prom_loadavg_stat(load_stats);
    update_prom_timestamp_stat(timestamp_stats);
}

int main()
{
    // Creo el directorio si no existe
    createDirectoryIfNotExists(PATH_DIR);
    // Abro y cierro el archivo en modo escritura para que se limpie cada vez
    FILE* outNDJSON = fopen(LOG_PATH, "w");
    if (outNDJSON == NULL)
    {
        perror("Error abriendo archivo de salida");
        exit(EXIT_FAILURE);
    }
    fclose(outNDJSON);
    // Hilo para exponer las métricas vía HTTP
    pthread_t tid;
    if (pthread_create(&tid, NULL, expose_metrics, NULL) != 0)
    {
        fprintf(stderr, "Error al crear el hilo del servidor HTTP\n");
        return EXIT_FAILURE;
    }
    // Se inicializan las métricas de Prometheus
    init_metrics();
    // Bucle principal para actualizar las métricas cada 5 segundos
    while (1)
    {
        // Actualizar métricas para el log y Prometheus
        updateMetrics();
        sleep(SLEEP_SECONDS);
    }
    // Destroy the mutex
    destroy_mutex();
    // El daemon del servidor HTTP se detiene al finalizar el programa
    return 0;
}

// http://localhost:3000/ (para ver las métricas en Grafana)
// http://localhost:8000/metrics (para ver las métricas en Prometheus)
