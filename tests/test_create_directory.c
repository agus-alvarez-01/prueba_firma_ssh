#include "create_directory.h"
#include "unity.h"
#include <sys/stat.h>
#include <unistd.h>

void setUp(void)
{
}
void tearDown(void)
{
}

void test_create_directory(void)
{
    const char* path = "./tests_tmp_dir_for_unittest";
    // remove if exists
    rmdir(path);
    createDirectoryIfNotExists(path);
    struct stat st = {0};
    int res = stat(path, &st);
    // debe existir y ser un directorio
    TEST_ASSERT_EQUAL_INT(0, res);
    TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));
    rmdir(path);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_directory);
    return UNITY_END();
}
