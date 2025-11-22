#include "prom_metrics.h"

// Variables para las métricas de Prometheus, se instancian en init_metrics()
static prom_gauge_t* timestamp;  // Metric for the timestamp of the metrics
static prom_gauge_t* mem_total;  // Metric for the total memory
static prom_gauge_t* mem_free;   // Metric for the free memory
static prom_gauge_t* mem_used;   // Metric for the used memory
static prom_gauge_t* cpu_user;   // Metric for CPU user time
static prom_gauge_t* cpu_system; // Metric for CPU system time
static prom_gauge_t* cpu_rate;   // Metric for CPU usage rate
static prom_gauge_t* load_1;     // Metric for 1-minute load average
static prom_gauge_t* load_5;     // Metric for 5-minute load average
static prom_gauge_t* load_15;    // Metric for 15-minute load average

// Mutex for synchronizing access to shared metrics
pthread_mutex_t lock;

/// Updates the Prometheus gauges.
void update_prom_cpu_stat(CpuStats cpu_stats)
{
    pthread_mutex_lock(&lock); // Locks the mutex to ensure exclusive access
    // Actualiza las métricas de CPU desde el JSON leído
    prom_gauge_set(cpu_user, cpu_stats.cpu_user, NULL);     // Sets CPU user time metric
    prom_gauge_set(cpu_system, cpu_stats.cpu_system, NULL); // Sets CPU system time metric
    prom_gauge_set(cpu_rate, cpu_stats.cpu_rate, NULL);     // Sets CPU usage rate metric
    pthread_mutex_unlock(&lock);                            // Releases the mutex after updating
}

void update_prom_memory_stat(MemoryStats mem_stats)
{
    pthread_mutex_lock(&lock);                            // Locks the mutex for exclusive access
    prom_gauge_set(mem_total, mem_stats.mem_total, NULL); // Sets total memory metric
    prom_gauge_set(mem_free, mem_stats.mem_free, NULL);   // Sets free memory metric
    prom_gauge_set(mem_used, mem_stats.mem_used, NULL);   // Sets used memory metric
    pthread_mutex_unlock(&lock);                          // Releases the mutex after updating
}

void update_prom_loadavg_stat(LoadavgStats load_stats)
{
    pthread_mutex_lock(&lock);                         // Locks the mutex for exclusive access
    prom_gauge_set(load_1, load_stats.load_1, NULL);   // Sets 1-minute load average metric
    prom_gauge_set(load_5, load_stats.load_5, NULL);   // Sets 5-minute load average metric
    prom_gauge_set(load_15, load_stats.load_15, NULL); // Sets 15-minute load average metric
    pthread_mutex_unlock(&lock);                       // Releases the mutex after updating
}

void update_prom_timestamp_stat(TimestampStats timestamp_stats)
{
    pthread_mutex_lock(&lock);                                  // Locks the mutex for exclusive access
    prom_gauge_set(timestamp, timestamp_stats.timestamp, NULL); // Sets timestamp metric
    pthread_mutex_unlock(&lock);                                // Releases the mutex after updating
}

void* expose_metrics(void* arg) // funcion con puntero porque se usa con pthread_create()
{
    // Ensures the HTTP handler is attached to the default Prometheus registry
    promhttp_set_active_collector_registry(NULL);

    // Starts an HTTP server on port 8000 to expose metrics
    struct MHD_Daemon* daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL);
    if (daemon == NULL)
    {
        fprintf(stderr, "Error al iniciar el servidor HTTP\n"); // Error message for HTTP server start failure
        return NULL;
    }
    // Keeps the server running indefinitely
    while (1)
    {
        sleep(1);
    }
    // Stops the daemon (this code will likely never be reached)
    MHD_stop_daemon(daemon);
    return NULL;
}

void init_metrics()
{
    // Initializes the mutex for synchronizing access
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        fprintf(stderr, "Error al inicializar el mutex\n"); // Error message for mutex initialization failure
        // return EXIT_FAILURE;
    }
    // Initializes the default Prometheus collector registry
    if (prom_collector_registry_default_init() != 0)
    {
        fprintf(
            stderr,
            "Error al inicializar el registro de Prometheus\n"); // Error message for registry initialization failure
        // return EXIT_FAILURE;
    }

    // Creates and registers for metrics
    mem_total = prom_gauge_new("memory_total_kb", "Total memory in kilobytes", 0, NULL);
    if (mem_total == NULL || prom_collector_registry_must_register_metric(mem_total) == NULL)
    {
        fprintf(stderr,
                "Error al crear la métrica de memoria total\n"); // Error message for memory metric creation failure
    }
    mem_free = prom_gauge_new("memory_free_kb", "Free memory in kilobytes", 0, NULL);
    if (mem_free == NULL || prom_collector_registry_must_register_metric(mem_free) == NULL)
    {
        fprintf(stderr,
                "Error al crear la métrica de memoria libre\n"); // Error message for memory metric creation failure
    }
    mem_used = prom_gauge_new("memory_used_kb", "Used memory in kilobytes", 0, NULL);
    if (mem_used == NULL || prom_collector_registry_must_register_metric(mem_used) == NULL)
    {
        fprintf(stderr,
                "Error al crear la métrica de memoria usada\n"); // Error message for memory metric creation failure
    }
    cpu_user = prom_gauge_new("cpu_user", "CPU user time", 0, NULL);
    if (cpu_user == NULL || prom_collector_registry_must_register_metric(cpu_user) == NULL)
    {
        fprintf(stderr,
                "Error al crear la métrica de CPU user\n"); // Error message for memory metric creation failure
    }
    cpu_system = prom_gauge_new("cpu_system", "CPU system time", 0, NULL);
    if (cpu_system == NULL || prom_collector_registry_must_register_metric(cpu_system) == NULL)
    {
        fprintf(stderr,
                "Error al crear la métrica de CPU system\n"); // Error message for memory metric creation failure
    }
    cpu_rate = prom_gauge_new("cpu_rate", "CPU usage rate", 0, NULL);
    if (cpu_rate == NULL || prom_collector_registry_must_register_metric(cpu_rate) == NULL)
    {
        fprintf(stderr,
                "Error al crear la métrica de CPU rate\n"); // Error message for memory metric creation failure
    }
    load_1 = prom_gauge_new("load_1", "1-minute load average", 0, NULL);
    if (load_1 == NULL || prom_collector_registry_must_register_metric(load_1) == NULL)
    {
        fprintf(stderr,
                "Error al crear la métrica de carga promedio 1\n"); // Error message for memory metric creation failure
    }
    load_5 = prom_gauge_new("load_5", "5-minute load average", 0, NULL);
    if (load_5 == NULL || prom_collector_registry_must_register_metric(load_5) == NULL)
    {
        fprintf(stderr,
                "Error al crear la métrica de carga promedio 5\n"); // Error message for memory metric creation failure
    }
    load_15 = prom_gauge_new("load_15", "15-minute load average", 0, NULL);
    if (load_15 == NULL || prom_collector_registry_must_register_metric(load_15) == NULL)
    {
        fprintf(stderr,
                "Error al crear la métrica de carga promedio 15\n"); // Error message for memory metric creation failure
    }
    timestamp = prom_gauge_new("timestamp", "Timestamp of the metrics", 0, NULL);
    if (timestamp == NULL || prom_collector_registry_must_register_metric(timestamp) == NULL)
    {
        fprintf(stderr,
                "Error al crear la métrica de timestamp\n"); // Error message for memory metric creation failure
    }
}

void destroy_mutex()
{
    pthread_mutex_destroy(&lock);
}

///////////////
// Hilo para iniciar las metricas
// pthread_t metrics;
// if (pthread_create(&metrics, NULL, monitoring, NULL) != 0)
// {
//     fprintf(stderr, "Error al crear el hilo del monitoring\n");
//     exit(EXIT_FAILURE);
//     //return EXIT_FAILURE;
// }
