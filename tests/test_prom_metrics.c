#include "prom_metrics.h"
#include "struct_metrics.h"
#include "unity.h"
#include <pthread.h>

// Configuración del entorno Unity
void setUp(void)
{
}
void tearDown(void)
{
}

// Inicialización básica de métricas
void test_init_metrics(void)
{
    // No debería fallar ni provocar errores
    init_metrics();
    TEST_PASS(); // Si no hubo errores, pasa
}

// Test con datos válidos
void test_update_prom_valid_stats(void)
{
    CpuStats cpu = {183127, 50105, 78.51710};
    MemoryStats mem = {12039248, 5131132, 3911484};
    LoadavgStats load = {1.36, 0.79, 0.78};
    TimestampStats ts = {1760206371};

    update_prom_cpu_stat(cpu);
    update_prom_memory_stat(mem);
    update_prom_loadavg_stat(load);
    update_prom_timestamp_stat(ts);

    TEST_PASS(); // Si no hubo errores, pasa
}

//  Test con datos de error
void test_update_prom_invalid_stats(void)
{
    CpuStats cpu = {-1, -1, -1.0};
    MemoryStats mem = {-1, -1, -1};
    LoadavgStats load = {-0.5, -0.5, -0.5};
    TimestampStats ts = {0};

    update_prom_cpu_stat(cpu);
    update_prom_memory_stat(mem);
    update_prom_loadavg_stat(load);
    update_prom_timestamp_stat(ts);

    TEST_PASS(); // Si hubo errores, pasa
}

void test_expose_metrics_thread(void)
{
    pthread_t thread;
    // No deberíamos bloquear ni causar errores al crear el hilo
    int ret = pthread_create(&thread, NULL, expose_metrics, NULL);
    TEST_ASSERT_EQUAL(0, ret);
    pthread_cancel(thread);
    pthread_join(thread, NULL);
    TEST_PASS(); // Si no hubo errores, pasa
}

void test_destroy_mutex(void)
{
    destroy_mutex();
    TEST_PASS(); // Si no hubo errores, pasa
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_metrics);
    RUN_TEST(test_update_prom_valid_stats);
    RUN_TEST(test_update_prom_invalid_stats);
    RUN_TEST(test_expose_metrics_thread);
    RUN_TEST(test_destroy_mutex);
    return UNITY_END();
}
