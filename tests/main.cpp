#include "test_runner.h"

int g_tests_run = 0;
int g_tests_passed = 0;
int g_tests_failed = 0;

extern void run_test_settings();
extern void run_test_timeline();
extern void run_test_account();
extern void run_test_toot_format();
extern void run_test_url_builder();
extern void run_test_creds();

int main() {
    printf("=== Amidon2 Tests ===\n\n");

    run_test_settings();
    printf("\n");

    run_test_timeline();
    printf("\n");

    run_test_account();
    printf("\n");

    run_test_toot_format();
    printf("\n");

    run_test_url_builder();
    printf("\n");

    run_test_creds();
    printf("\n");

    TEST_SUMMARY();
}
