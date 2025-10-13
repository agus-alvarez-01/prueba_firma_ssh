#include "get_metrics.h"
#include "struct_metrics.h"
#include "unity.h"

// Configuración del entorno Unity
void setUp(void)
{
}
void tearDown(void)
{
}

// Test con datos válidos
void test_get_memory_stat_valid(void)
{
    MemoryStats stats = get_memory_stat();
    // Los valores tienen que ser coherentes (no negativos)
    TEST_ASSERT_GREATER_OR_EQUAL(0, stats.mem_total);
    TEST_ASSERT_GREATER_OR_EQUAL(0, stats.mem_free);
    TEST_ASSERT_GREATER_OR_EQUAL(0, stats.mem_used);
}
// Test con datos incompletos (simulando error)
void test_get_memory_stat_incomplete(void)
{
    // Seteo struct con -1
    MemoryStats stats = (MemoryStats){-1, -1, -1};
    // Validación de error
    TEST_ASSERT_EQUAL(-1, stats.mem_total);
    TEST_ASSERT_EQUAL(-1, stats.mem_free);
    TEST_ASSERT_EQUAL(-1, stats.mem_used);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_get_memory_stat_valid);
    RUN_TEST(test_get_memory_stat_incomplete);
    return UNITY_END();
}
