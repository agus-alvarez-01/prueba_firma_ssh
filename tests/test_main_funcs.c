#include "../include/shell_core.h"
#include "unity.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    char buf[64];
    ontime(t, buf, sizeof(buf));
    // Expect approximately 01:01:01
    TEST_ASSERT_EQUAL_CHAR_ARRAY("01:01:01", buf, 8);
}

void test_showStatus_no_monitoring(void)
{
    // Arrange: ensure monitoring not active and t_start 1 hour ago
    monitoring_active = false;
    t_start = time(NULL) - 3600;

    // Capture stdout to temporary file
    const char* tmp = "./tests_main_stdout.txt";
    FILE* f = freopen(tmp, "w+", stdout);
    TEST_ASSERT_NOT_NULL(f);

    showStatus();

    fflush(stdout);
    freopen("/dev/tty", "w", stdout); // restore (best-effort)

    // Read file
    FILE* r = fopen(tmp, "r");
    TEST_ASSERT_NOT_NULL(r);
    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf) - 1, r);
    buf[n] = '\0';
    fclose(r);
    remove(tmp);

    TEST_ASSERT_NOT_EQUAL(0, strstr(buf, "La shell esta activa hace:") != NULL);
    TEST_ASSERT_NOT_EQUAL(0, strstr(buf, "El monitoreo no esta activo.") != NULL);
}

void test_loggerDaemon_writes_file(void)
{
    // Prepare log pipe and start daemon thread
    if (pipe(logpipe) == -1)
    {
        TEST_FAIL_MESSAGE("pipe failed");
    }
    // Ensure test log dir is cleaned
    system("rm -rf ./.test_mon && mkdir -p ./.test_mon");

    pthread_t th;
    int rc = pthread_create(&th, NULL, loggerDaemon, NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);

    const char* msg = "z";
    int w = write(logpipe[1], msg, strlen(msg));
    TEST_ASSERT_TRUE(w > 0);

    // give it some time to write
    sleep(1);

    // cancel thread and join
    pthread_cancel(th);
    pthread_join(th, NULL);

    // Check file
    FILE* f = fopen("./.test_mon/actions.log", "r");
    TEST_ASSERT_NOT_NULL(f);
    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);

    TEST_ASSERT_NOT_EQUAL(0, strstr(buf, "Se ingreso por consola") != NULL);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ontime_one_hour_one_min_one_sec);
    RUN_TEST(test_showStatus_no_monitoring);
    RUN_TEST(test_loggerDaemon_writes_file);
    return UNITY_END();
}
