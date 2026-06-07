#include <check.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Include the production code header/types */
#include "app_reporting.h"

/* We need access to APP_u8GetRecordIndex and the array size constant */
extern uint8 APP_u8GetRecordIndex(uint16 u16ClusterID, uint16 u16AttributeEnum);

#ifndef ZCL_NUMBER_OF_REPORTS
#define ZCL_NUMBER_OF_REPORTS 10
#endif

START_TEST(test_record_index_bounds_check)
{
    /* Invariant: APP_u8GetRecordIndex must NEVER return an index >= ZCL_NUMBER_OF_REPORTS
       unless it returns 0xFF (not found). Any other value would cause OOB access. */

    /* Adversarial cluster/attribute combinations designed to probe bounds */
    struct { uint16_t cluster; uint16_t attr; } payloads[] = {
        {0xFFFF, 0xFFFF},  /* Max values - exploit case */
        {0x0000, 0xFFFE},  /* Boundary attribute enum */
        {0x0402, 0x0000},  /* Valid temperature cluster, measured value */
        {0xDEAD, 0xBEEF},  /* Arbitrary adversarial values */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        uint8 idx = APP_u8GetRecordIndex(payloads[i].cluster, payloads[i].attr);
        /* The index must either be 0xFF (not found) or within array bounds */
        ck_assert_msg(idx == 0xFF || idx < ZCL_NUMBER_OF_REPORTS,
                      "APP_u8GetRecordIndex returned out-of-bounds index %u for cluster=0x%04X attr=0x%04X",
                      (unsigned)idx, payloads[i].cluster, payloads[i].attr);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_record_index_bounds_check);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}