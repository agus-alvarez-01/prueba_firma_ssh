#include "struct_metrics.h" //para los update de métricas
#include <prom.h>           //para prom_gauge_*
#include <promhttp.h>       //para promhttp_start_daemon
#include <pthread.h>        //para mutex
#include <stdio.h>          //para FILE, fopen, fclose, perror, fprintf, stderr
#include <stdlib.h>         //para EXIT_FAILURE
#include <unistd.h>         //para sleep

#define PORT 8000 // Puerto para el servidor HTTP

void update_prom_cpu_stat(CpuStats cpu_stats);
void update_prom_memory_stat(MemoryStats mem_stats);
void update_prom_loadavg_stat(LoadavgStats load_stats);
void update_prom_timestamp_stat(TimestampStats timestamp_stats);
void* expose_metrics(void* arg);
void init_metrics();
void destroy_mutex();
