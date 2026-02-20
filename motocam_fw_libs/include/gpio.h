#ifndef GPIO_H
#define GPIO_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
// GPIO Pin Definitions

typedef enum
{
    GPIO_PIN_IR_INIT_0  = 0,
    GPIO_PIN_IR_INIT_4  = 4,
    GPIO_PIN_IR_INIT_5  = 5,
    GPIO_PIN_IR_INIT_33 = 33,

    GPIO_PIN_IR_CUT_59  = 59,
    GPIO_PIN_IR_CUT_60  = 60,

    GPIO_PIN_B_LED      = 1,
    GPIO_PIN_G_LED      = 2,
    GPIO_PIN_R_LED      = 3
} gpio_pin_t;

// GPIO Direction Macros
static const char GPIO_DIRECTION_OUT[] = "out";

// GPIO Value Macros
typedef enum
{
    CLEAR_GPIO = 0,
    SET_GPIO   = 1
} gpio_value_t;

// Function Declarations
int gpio_init();
void update_ir_cut_filter_off();
void update_ir_cut_filter_on();
uint8_t get_ir_cut_filter();

#ifdef __cplusplus
}
#endif

#endif // GPIO_H
