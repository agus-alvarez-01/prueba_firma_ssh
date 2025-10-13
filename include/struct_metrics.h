#include <time.h>
// Guardas para evitar inclusiones múltiples (en prom_metrics.h y get_metrics.h)
#ifndef STRUCT_METRICS_H
#define STRUCT_METRICS_H

// Definicion de structs para obtener las estadisticas
typedef struct
{
    long int cpu_user;   /**< CPU user time. */
    long int cpu_system; /**< CPU system time. */
    float cpu_rate;      /**< CPU usage rate. */
} CpuStats;

typedef struct
{
    long int mem_total; /**< Total memory. */
    long int mem_free;  /**< Free memory. */
    long int mem_used;  /**< Used memory. */
} MemoryStats;

typedef struct
{
    float load_1;  /**< 1-minute load average. */
    float load_5;  /**< 5-minute load average. */
    float load_15; /**< 15-minute load average. */
} LoadavgStats;

typedef struct
{
    time_t timestamp; /**< Timestamp of the metrics. */
} TimestampStats;

#endif // STRUCT_METRICS_H
