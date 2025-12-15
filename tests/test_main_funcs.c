#include "shell_core.h"
#include "unity.h"
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_TIME 64
#define BUFFER_READ 512

void setUp(void)
{
}
void tearDown(void)
{
}

void test_ontime_one_hour_one_min_one_sec(void)
{
    time_t now = time(NULL);
    time_t t = now - (1 * 3600 + 1 * 60 + 1);
    char buf[BUFFER_TIME];
    ontime(t, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_CHAR_ARRAY("01h 01m 01s", buf, 8);
}

void test_showStatus_no_monitoring(void)
{
    monitoring_active = false;
    t_start = time(NULL) - 3600;

    const char* tmp = "./tests_main_stdout.txt";
    FILE* f = freopen(tmp, "w+", stdout);
    TEST_ASSERT_NOT_NULL(f);

    showStatus();

    fflush(stdout);

    if (freopen("/dev/tty", "w", stdout) == NULL)
    {
    }
    FILE* r = fopen(tmp, "r");
    TEST_ASSERT_NOT_NULL(r);
    char buf[BUFFER_READ];
    size_t n = fread(buf, 1, sizeof(buf) - 1, r);
    buf[n] = '\0';
    fclose(r);
    remove(tmp);

    TEST_ASSERT_NOT_EQUAL(0, strstr(buf, "La shell esta activa hace:") != NULL);
    TEST_ASSERT_NOT_EQUAL(0, strstr(buf, "El monitoreo no esta activo.") != NULL);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ontime_one_hour_one_min_one_sec);
    RUN_TEST(test_showStatus_no_monitoring);
    return UNITY_END();
}
