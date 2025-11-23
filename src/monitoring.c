#include <signal.h>
#include <sys/stat.h> //para mkdir

#include "get_metrics.h"  //obteniendo métricas del /proc
#include "monitoring.h"   //para exponer métricas vía HTTP
#include "prom_metrics.h" //para exponer métricas vía HTTP

#define SLEEP_SECONDS 5
#define PATH_DIR "/var/lib/monitoreo" // directorio para el archivo de metrics
#define LOG_PATH "/var/lib/monitoreo/metrics.log"

static char lastMetric[256] =
    "Aun no hay metricas"; // almacena la última métrica en formato NDJSON, y son aproximadamente 185 caracteres
// Pipe para comunicar la última métrica al proceso padre
int pipe_fd; // descriptor de archivo del pipe

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
    snprintf(lastMetric, sizeof(lastMetric),
             "{"                                                      // inicio del NDJSON
             "\"timestamp\":%ld,"                                     // Timestamp
             "\"mem_total\":%ld,\"mem_free\":%ld,\"mem_used\":%ld,"   // Memoria
             "\"cpu_user\":%ld,\"cpu_system\":%ld,\"cpu_rate\":%.5f," // CPU
             "\"load_1\":%.2f,\"load_5\":%.2f,\"load_15\":%.2f"       // Load Average
             "}\n",                                                   // fin del NDJSON
             timestamp_stats.timestamp, mem_stats.mem_total, mem_stats.mem_free, mem_stats.mem_used, cpu_stats.cpu_user,
             cpu_stats.cpu_system, cpu_stats.cpu_rate, load_stats.load_1, load_stats.load_5, load_stats.load_15);
    // Escribo la última métrica en el archivo
    fprintf(outNDJSON, "%s", lastMetric);
    // cierro archivo
    fclose(outNDJSON);
    // Una vez que se actualizan las métricas en el log, se actualizan las métricas de Prometheus
    update_prom_cpu_stat(cpu_stats);
    update_prom_memory_stat(mem_stats);
    update_prom_loadavg_stat(load_stats);
    update_prom_timestamp_stat(timestamp_stats);
}

static void handler(int sig) // handler para el escribir en pipe
{
    ssize_t n = write(pipe_fd, &lastMetric, sizeof(lastMetric));
    if (n == -1)
    {
        perror("write");
        return;
    }
}

int main(int argc, char* argv[])
{
    // El pipe se pasa como argumento
    pipe_fd = atoi(argv[1]);
    signal(SIGUSR1, handler);
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
        // return EXIT_FAILURE;
        exit(EXIT_FAILURE);
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
    // return 0;
}

// http://localhost:3000/ (para ver las métricas en Grafana)
// http://localhost:8000/metrics (para ver las métricas en Prometheus)
