/**
 * DOB Validation Test Examples
 * 
 * This file contains test examples for the DOB-enhanced APIs:
 * - SET_USER_DOB
 * - FACTORY_RESET
 * - CONFIG_RESET
 * - SET_LOGIN
 * 
 * Compile and run these tests to verify the DOB validation functionality.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "include/l1/motocam_system_api_l1.h"

// Helper function to print test results
void print_test_result(const char* test_name, int8_t result, int8_t expected) {
    printf("\n[TEST] %s\n", test_name);
    printf("  Result: %d, Expected: %d\n", result, expected);
    if (result == expected) {
        printf("  ✓ PASSED\n");
    } else {
        printf("  ✗ FAILED\n");
    }
}

// Test 1: Set DOB (First Time Setup)
void test_set_dob() {
    printf("\n========================================\n");
    printf("TEST 1: Setting DOB\n");
    printf("========================================\n");
    
    uint8_t dob[] = "15-01-1990";
    uint8_t dob_length = 10;
    
    int8_t result = set_system_user_dob_l1(dob_length, dob);
    print_test_result("Set DOB (15-01-1990)", result, 0);
}

// Test 2: Get DOB
void test_get_dob() {
    printf("\n========================================\n");
    printf("TEST 2: Getting DOB\n");
    printf("========================================\n");
    
    uint8_t *dob_response;
    uint8_t dob_length;
    
    int8_t result = get_system_user_dob_l1(0, NULL, &dob_response, &dob_length);
    
    if (result == 0) {
        printf("  Retrieved DOB: %.*s (length: %d)\n", dob_length, dob_response, dob_length);
        free(dob_response);
    }
    
    print_test_result("Get DOB", result, 0);
}

// Test 3: Factory Reset without DOB (should fail with -5)
void test_factory_reset_no_dob() {
    printf("\n========================================\n");
    printf("TEST 3: Factory Reset without DOB\n");
    printf("========================================\n");
    
    int8_t result = set_system_factory_reset_l1(0, NULL);
    print_test_result("Factory Reset (no DOB)", result, -5);
}

// Test 4: Factory Reset with wrong DOB (should fail with -7)
void test_factory_reset_wrong_dob() {
    printf("\n========================================\n");
    printf("TEST 4: Factory Reset with Wrong DOB\n");
    printf("========================================\n");
    
    uint8_t wrong_dob[] = "20-05-1985";
    uint8_t dob_length = 10;
    
    int8_t result = set_system_factory_reset_l1(dob_length, wrong_dob);
    print_test_result("Factory Reset (wrong DOB)", result, -7);
}

// Test 5: Factory Reset with correct DOB (should succeed)
void test_factory_reset_correct_dob() {
    printf("\n========================================\n");
    printf("TEST 5: Factory Reset with Correct DOB\n");
    printf("========================================\n");
    
    uint8_t correct_dob[] = "15-01-1990";
    uint8_t dob_length = 10;
    
    int8_t result = set_system_factory_reset_l1(dob_length, correct_dob);
    print_test_result("Factory Reset (correct DOB)", result, 0);
}

// Test 6: Config Reset with invalid DOB format (should fail)
void test_config_reset_invalid_format() {
    printf("\n========================================\n");
    printf("TEST 6: Config Reset with Invalid DOB Format\n");
    printf("========================================\n");
    
    uint8_t invalid_dob[] = "15/01/1990";  // Wrong separator
    uint8_t dob_length = 10;
    
    int8_t result = set_system_config_reset_l1(dob_length, invalid_dob);
    // Should fail with format validation error (-1 or -2) or mismatch (-7)
    printf("  Result: %d (Expected: -1, -2, or -7)\n", result);
    printf("  %s\n", (result < 0) ? "✓ PASSED (Failed as expected)" : "✗ FAILED");
}

// Test 7: Config Reset with correct DOB
void test_config_reset_correct_dob() {
    printf("\n========================================\n");
    printf("TEST 7: Config Reset with Correct DOB\n");
    printf("========================================\n");
    
    uint8_t correct_dob[] = "15-01-1990";
    uint8_t dob_length = 10;
    
    int8_t result = set_system_config_reset_l1(dob_length, correct_dob);
    print_test_result("Config Reset (correct DOB)", result, 0);
}

// Test 8: Set Login without DOB (should fail with -5)
void test_set_login_no_dob() {
    printf("\n========================================\n");
    printf("TEST 8: Set Login without DOB\n");
    printf("========================================\n");
    
    // Old format: just PIN without DOB
    uint8_t data[] = {
        4,                      // PIN length
        '1', '2', '3', '4'      // PIN
    };
    uint8_t data_length = 5;
    
    int8_t result = set_system_login_l1(data_length, data);
    print_test_result("Set Login (no DOB)", result, -5);
}

// Test 9: Set Login with wrong DOB (should fail with -7)
void test_set_login_wrong_dob() {
    printf("\n========================================\n");
    printf("TEST 9: Set Login with Wrong DOB\n");
    printf("========================================\n");
    
    uint8_t data[] = {
        4,                      // PIN length
        '1', '2', '3', '4',     // PIN
        '2', '0', '-', '0', '5', '-', '1', '9', '8', '5'  // Wrong DOB
    };
    uint8_t data_length = 15;  // 1 + 4 + 10
    
    int8_t result = set_system_login_l1(data_length, data);
    print_test_result("Set Login (wrong DOB)", result, -7);
}

// Test 10: Set Login with correct DOB (should succeed)
void test_set_login_correct_dob() {
    printf("\n========================================\n");
    printf("TEST 10: Set Login with Correct DOB\n");
    printf("========================================\n");
    
    uint8_t data[] = {
        4,                      // PIN length
        '1', '2', '3', '4',     // PIN
        '1', '5', '-', '0', '1', '-', '1', '9', '9', '0'  // Correct DOB
    };
    uint8_t data_length = 15;  // 1 + 4 + 10
    
    int8_t result = set_system_login_l1(data_length, data);
    print_test_result("Set Login (correct DOB)", result, 0);
}

// Test 11: Set DOB with invalid date (e.g., 32-13-2023)
void test_set_invalid_dob() {
    printf("\n========================================\n");
    printf("TEST 11: Set Invalid DOB\n");
    printf("========================================\n");
    
    uint8_t invalid_dob[] = "32-13-2023";  // Invalid day and month
    uint8_t dob_length = 10;
    
    int8_t result = set_system_user_dob_l1(dob_length, invalid_dob);
    printf("  Result: %d (Expected: -1 or -2)\n", result);
    printf("  %s\n", (result == -1 || result == -2) ? "✓ PASSED" : "✗ FAILED");
}

// Test 12: Set DOB with wrong length
void test_set_dob_wrong_length() {
    printf("\n========================================\n");
    printf("TEST 12: Set DOB with Wrong Length\n");
    printf("========================================\n");
    
    uint8_t short_dob[] = "15-01-90";  // Only 8 characters
    uint8_t dob_length = 8;
    
    int8_t result = set_system_user_dob_l1(dob_length, short_dob);
    print_test_result("Set DOB (wrong length)", result, -5);
}

// Test 13: Set Login with different PIN lengths
void test_set_login_various_pin_lengths() {
    printf("\n========================================\n");
    printf("TEST 13: Set Login with Various PIN Lengths\n");
    printf("========================================\n");
    
    // Test with 1-character PIN
    uint8_t data_1char[] = {
        1,                      // PIN length
        '1',                    // PIN
        '1', '5', '-', '0', '1', '-', '1', '9', '9', '0'  // DOB
    };
    int8_t result1 = set_system_login_l1(12, data_1char);
    print_test_result("Set Login (1-char PIN)", result1, 0);
    
    // Test with 32-character PIN
    uint8_t data_32char[] = {
        32,                     // PIN length
        '1','2','3','4','5','6','7','8','9','0',
        '1','2','3','4','5','6','7','8','9','0',
        '1','2','3','4','5','6','7','8','9','0',
        '1','2',                // 32 characters
        '1', '5', '-', '0', '1', '-', '1', '9', '9', '0'  // DOB
    };
    int8_t result2 = set_system_login_l1(43, data_32char);
    print_test_result("Set Login (32-char PIN)", result2, 0);
}

// Test 14: Attempt operations before setting DOB
void test_operations_before_dob_set() {
    printf("\n========================================\n");
    printf("TEST 14: Operations Before DOB is Set\n");
    printf("========================================\n");
    printf("NOTE: Run this test on a fresh system without DOB set\n");
    
    uint8_t dob[] = "15-01-1990";
    
    // Try factory reset without DOB set in system
    int8_t result1 = set_system_factory_reset_l1(10, dob);
    printf("  Factory Reset (DOB not set): %d (Expected: -6)\n", result1);
    
    // Try config reset without DOB set in system
    int8_t result2 = set_system_config_reset_l1(10, dob);
    printf("  Config Reset (DOB not set): %d (Expected: -6)\n", result2);
    
    // Try set login without DOB set in system
    uint8_t login_data[] = {
        4, '1', '2', '3', '4',
        '1', '5', '-', '0', '1', '-', '1', '9', '9', '0'
    };
    int8_t result3 = set_system_login_l1(15, login_data);
    printf("  Set Login (DOB not set): %d (Expected: -6)\n", result3);
    
    printf("\n  All operations should fail with -6 if DOB is not set\n");
}

// Main test runner
int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                 DOB Validation Test Suite                 ║\n");
    printf("║                                                            ║\n");
    printf("║  Testing DOB-enhanced APIs:                               ║\n");
    printf("║  - SET_USER_DOB                                           ║\n");
    printf("║  - FACTORY_RESET                                          ║\n");
    printf("║  - CONFIG_RESET                                           ║\n");
    printf("║  - SET_LOGIN                                              ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    // Run all tests
    printf("\n\n=== SETUP TESTS ===\n");
    test_set_dob();
    test_get_dob();
    
    printf("\n\n=== FACTORY RESET TESTS ===\n");
    test_factory_reset_no_dob();
    test_factory_reset_wrong_dob();
    test_factory_reset_correct_dob();
    
    printf("\n\n=== CONFIG RESET TESTS ===\n");
    test_config_reset_invalid_format();
    test_config_reset_correct_dob();
    
    printf("\n\n=== SET LOGIN TESTS ===\n");
    test_set_login_no_dob();
    test_set_login_wrong_dob();
    test_set_login_correct_dob();
    test_set_login_various_pin_lengths();
    
    printf("\n\n=== VALIDATION TESTS ===\n");
    test_set_invalid_dob();
    test_set_dob_wrong_length();
    
    printf("\n\n=== EDGE CASE TESTS ===\n");
    printf("NOTE: Comment out test_operations_before_dob_set() if DOB is already set\n");
    
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                   Test Suite Complete                     ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    return 0;
}

/**
 * Compilation Instructions:
 * 
 * To compile this test file:
 * 
 * gcc -o dob_test DOB_TEST_EXAMPLES.c \
 *     -I./include \
 *     -L./build \
 *     -lmotocam_api \
 *     -lpthread
 * 
 * To run:
 * ./dob_test
 * 
 * Expected Output:
 * - All PASSED tests indicate correct implementation
 * - Any FAILED tests indicate issues that need to be addressed
 */
