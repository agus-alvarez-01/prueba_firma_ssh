#include "../src/flags_bash.c"
#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

// Llamar a flag_entry con argc=1 debe devolver 0 y no salir
void test_flag_entry_no_args(void)
{
    char* argv[] = {"prog", NULL};
    int ret = flag_entry(1, argv);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_flag_entry_no_args);
    return UNITY_END();
}
