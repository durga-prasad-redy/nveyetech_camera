#ifndef GPIO_H
#define GPIO_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
// GPIO Pin Definitions
#define GPIO_PIN_IR_INIT_0   0
#define GPIO_PIN_IR_INIT_4   4
#define GPIO_PIN_IR_INIT_5   5
#define GPIO_PIN_IR_INIT_33  33

#define GPIO_PIN_IR_CUT_59   59
#define GPIO_PIN_IR_CUT_60   60

#define GPIO_PIN_B_LED       1
#define GPIO_PIN_G_LED       2
#define GPIO_PIN_R_LED       3

// GPIO Direction Macros
#define GPIO_DIRECTION_OUT   "out"

// GPIO Value Macros
#define SET_GPIO 1
#define CLEAR_GPIO 0

// Function Declarations
int gpio_init();
void update_ir_cut_filter_off();
void update_ir_cut_filter_on();
uint8_t get_ir_cut_filter();

#ifdef __cplusplus
}
#endif

#endif // GPIO_H
