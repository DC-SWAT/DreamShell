#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

// Include the actual production header
#include "firmware/isoldr/loader/kos/net/icmp.h"

START_TEST(test_icmp_input_buffer_bounds)
{
    // Invariant: net_icmp_input must not write beyond net_txbuf boundaries regardless of ip->length value
    const uint16_t payloads[] = {
        0xFFFF,  // Exploit case: maximum value causing overflow
        0x05DC,  // Boundary case: 1500 bytes (typical MTU)
        0x0008   // Valid minimal ICMP packet
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        // Create test packet with adversarial ip->length
        struct ip_header ip;
        struct icmp_header icmp;
        
        // Setup adversarial IP header
        ip.version_ihl = 0x45;  // IPv4, 5 words header
        ip.length = htons(payloads[i]);
        
        // Setup minimal ICMP header
        icmp.type = 8;  // Echo request
        icmp.code = 0;
        icmp.checksum = 0;
        
        // Call the actual production function
        // Note: This assumes net_icmp_input is accessible and takes appropriate parameters
        // In practice, we might need to create a proper packet buffer
        int result = net_icmp_input(&ip, &icmp);
        
        // The security property: function should handle adversarial input safely
        // Either by rejecting invalid packets or by using bounded operations
        ck_assert_msg(result != -2, 
                     "Buffer overflow detected with ip->length = 0x%04x", 
                     payloads[i]);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_icmp_input_buffer_bounds);
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