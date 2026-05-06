/*!
    \file    cmd_gpio.c
    \brief   GPIO command shell for GD32VW55x SDK.

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/
#include "gd32vw55x_gpio.h"
#include "cmd_gpio.h"

/*
 * There are up to 29 general purpose I/O pins (GPIO) in GD32VW553xx, named PA0 ~
 * PA15, PB0 ~ PB4, PB11 ~ PB13, PB15, PC8 and PC13 ~ PC15 to implement logic
 * input/output functions.
*/
static int gpio_check(uint32_t periph, uint32_t pin)
{
    int result = GPIO_CHECK_FAIL;

    switch(periph) {
    case GPIOA:
        if ((pin >= 0) && (pin <= 15)) {
            result = GPIO_CHECK_SUCCESS;
        }
        break;
    case GPIOB:
        if (((pin >= 0) && (pin <= 4)) || ((pin >= 11) && (pin <= 13)) || (pin == 15)) {
            result = GPIO_CHECK_SUCCESS;
        }
        break;
    case GPIOC:
        if ((pin == 8) || ((pin >= 13) && (pin <= 15))) {
            result = GPIO_CHECK_SUCCESS;
        }
        break;
    default:
        break;
    }

    return result;
}

static int parse_gpio_pin(const char *arg, uint32_t *periph, uint32_t *pin)
{
    char *child = NULL;
    uint32_t gpio_periph, pin_number;

    if ((child = strstr(arg, "PA")) != NULL) {
        gpio_periph = GPIOA;
    } else if ((child = strstr(arg, "PB")) != NULL) {
        gpio_periph = GPIOB;
    } else if ((child = strstr(arg, "PC")) != NULL) {
        gpio_periph = GPIOC;
    } else {
        app_print("Unkonwn GPIO periph\r\n");
        return GPIO_PARSE_FAIL;
    }

    pin_number = atoi(child + 2);

    if (gpio_check(gpio_periph, pin_number) == GPIO_CHECK_SUCCESS) {
        *periph = gpio_periph;
        *pin = pin_number;
        return GPIO_PARSE_SUCCESS;
    }

    return GPIO_PARSE_FAIL;
}

static void gpio_mode_get(uint32_t gpio_periph, uint32_t *mode, uint32_t pin)
{
    uint16_t i;
    uint32_t ctl, value;

    ctl = GPIO_CTL(gpio_periph);

    for (i = 0U; i < 16U; i++) {
        if ((1U << i) & pin) {
            value = ctl & GPIO_MODE_MASK(i);
            *mode = (value >> (2 * i)) & 0x3;
        }
    }
}

static void gpio_pupd_get(uint32_t gpio_periph, uint32_t *pull_up_down, uint32_t pin)
{
    uint16_t i;
    uint32_t pupd, value;

    pupd = GPIO_PUD(gpio_periph);

    for (i = 0U; i < 16U; i++) {
        if ((1U << i) & pin) {
            value = pupd & GPIO_PUPD_MASK(i);
            *pull_up_down = (value >> (2 * i)) & 0x3;
        }
    }
}

void cmd_gpio_mode_set(int argc, char **argv)
{
    uint32_t gpio_periph, pin_number;
    uint32_t mode, pupd, pin_bit, af_num;

    if ((argc == 4) || (argc == 5)) {
        if (parse_gpio_pin(argv[1], &gpio_periph, &pin_number) != GPIO_PARSE_SUCCESS) {
            app_print("Invalid GPIO number\r\n");
            goto Usage;
        }

        mode = atoi(argv[2]);
        if (mode == 0) {
            mode = GPIO_MODE_INPUT;
        } else if (mode == 1) {
            mode = GPIO_MODE_OUTPUT;
        } else if (mode == 2) {
            mode = GPIO_MODE_AF;
        } else if (mode == 3) {
            mode = GPIO_MODE_ANALOG;
        } else {
            app_print("Invalid GPIO mode\r\n");
            goto Usage;
        }

        pupd = atoi(argv[3]);
        if (pupd == 0) {
            pupd = GPIO_PUPD_NONE;
        } else if (pupd == 1) {
            pupd = GPIO_PUPD_PULLUP;
        } else if (pupd == 2) {
            pupd = GPIO_PUPD_PULLDOWN;
        } else {
            app_print("Invalid pull up or pull down value\r\n");
            goto Usage;
        }

        pin_bit = BIT(pin_number);

        if (mode != GPIO_MODE_AF) {
            gpio_mode_set(gpio_periph, mode, pupd, pin_bit);
        } else if ((mode == GPIO_MODE_AF) && (argc == 5)) {
            af_num = atoi(argv[4]);
            if ((af_num >= 0) && (af_num <= 15)) {
                gpio_af_set(gpio_periph, (uint32_t)AF(af_num), pin_bit);
                gpio_mode_set(gpio_periph, mode, pupd, pin_bit);
                gpio_output_options_set(gpio_periph, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, pin_bit);
            } else {
                app_print("Invalid af num\r\n");
                goto Usage;
            }
        } else {
            app_print("Params error\r\n");
            goto Usage;
        }
    } else {
        goto Usage;
    }

    app_print("OK\r\n");
    return;

Usage:
    app_print("Usage: set_gpio_mode <pin> <mode> <pupd> [AF_NUM]\r\n");
    app_print("       <pin>: PA0-PA15, PB0-PB4, PB11-PB13, PB15, PC8, PC13-PC15\r\n");
    app_print("       <mode>: 0-INPUT, 1-OUTPUT, 2-AF, 3-Analog\r\n");
    app_print("       <pupd>: 0-NONE, 1-PULLUP, 2-PULLDOWN\r\n");
    app_print("       [AF_NUM]: 0-15, valid when mode is AF\r\n");
    app_print("Example:\r\n");
    app_print("       set_gpio_mode PA0 1 0\r\n");
}

void cmd_gpio_mode_get(int argc, char **argv)
{
    uint32_t gpio_periph;
    uint32_t pin_number, pin_bit, pupd = 0, mode = 0;

    if (argc == 2) {
        if (parse_gpio_pin(argv[1], &gpio_periph, &pin_number) != GPIO_PARSE_SUCCESS) {
            app_print("Invalid GPIO number\r\n");
            goto Usage;
        }

        pin_bit = BIT(pin_number);

        gpio_mode_get(gpio_periph, &mode, pin_bit);
        gpio_pupd_get(gpio_periph, &pupd, pin_bit);

        if (mode == GPIO_MODE_INPUT) {
            if (pupd == GPIO_PUPD_NONE) {
                app_print("+RDGPIO:%s,0,0\r\n", argv[1]);
            } else if (pupd == GPIO_PUPD_PULLUP) {
                app_print("+RDGPIO:%s,0,1\r\n", argv[1]);
            } else if (pupd == GPIO_PUPD_PULLDOWN) {
                app_print("+RDGPIO:%s,0,2\r\n", argv[1]);
            } else {
                app_print("Unknown pull up and pull down status\r\n");
                return;
            }
        } else if (mode == GPIO_MODE_OUTPUT) {
            if (pupd == GPIO_PUPD_NONE) {
                app_print("+RDGPIO:%s,1,0\r\n", argv[1]);
            } else if (pupd == GPIO_PUPD_PULLUP) {
                app_print("+RDGPIO:%s,1,1\r\n", argv[1]);
            } else if (pupd == GPIO_PUPD_PULLDOWN) {
                app_print("+RDGPIO:%s,1,2\r\n", argv[1]);
            } else {
                app_print("Unknown pull up and pull down status\r\n");
                return;
            }
        } else if (mode == GPIO_MODE_AF) {
            if (pupd == GPIO_PUPD_NONE) {
                app_print("+RDGPIO:%s,2,0\r\n", argv[1]);
            } else if (pupd == GPIO_PUPD_PULLUP) {
                app_print("+RDGPIO:%s,2,1\r\n", argv[1]);
            } else if (pupd == GPIO_PUPD_PULLDOWN) {
                app_print("+RDGPIO:%s,2,2\r\n", argv[1]);
            } else {
                app_print("Unknown pull up and pull down status\r\n");
                return;
            }
        } else if (mode == GPIO_MODE_ANALOG) {
            if (pupd == GPIO_PUPD_NONE) {
                app_print("+RDGPIO:%s,3,0\r\n", argv[1]);
            } else if (pupd == GPIO_PUPD_PULLUP) {
                app_print("+RDGPIO:%s,3,1\r\n", argv[1]);
            } else if (pupd == GPIO_PUPD_PULLDOWN) {
                app_print("+RDGPIO:%s,3,2\r\n", argv[1]);
            } else {
                app_print("Unknown pull up and pull down status\r\n");
                return;
            }
        } else {
            app_print("Unknow mode(%d)\r\n", mode);
            return;
        }
    } else {
        goto Usage;
    }

    app_print("OK\r\n");
    return;

Usage:
    app_print("Usage: get_gpio_mode <pin>\r\n");
    app_print("Example:\r\n");
    app_print("       get_gpio_mode PB15\r\n");
}

void cmd_gpio_level_write(int argc, char **argv)
{
    uint32_t gpio_periph, pin_number;
    uint32_t pin, level, mode;

    if (argc == 3) {
        if (parse_gpio_pin(argv[1], &gpio_periph, &pin_number) != GPIO_PARSE_SUCCESS) {
            app_print("Invalid GPIO number\r\n");
            goto Usage;
        }

        level = atoi(argv[2]);
        if (level > 1) {
            app_print("Invalid level param\r\n");
            goto Usage;
        }

        pin = BIT(pin_number);

        gpio_mode_get(gpio_periph, &mode, pin);
        if (mode != GPIO_MODE_OUTPUT) {
            app_print("GPIO not output mode\r\n");
            return;
        }

        if (level == 0) {
            gpio_bit_write(gpio_periph, pin, RESET);
        } else if (level == 1) {
            gpio_bit_write(gpio_periph, pin, SET);
        }
    } else {
        goto Usage;
    }

    app_print("OK\r\n");
    return;

Usage:
    app_print("Usage: write_gpio_level <pin> <level>\r\n");
    app_print("       <pin>: PA0-PA15, PB0-PB4, PB11-PB13, PB15, PC8, PC13-PC15\r\n");
    app_print("       <level>: 0-RESET, 1-SET\r\n");
    app_print("Example:\r\n");
    app_print("       write_gpio_level PA0 1\r\n");
}

void cmd_gpio_level_read(int argc, char **argv)
{
    uint32_t gpio_periph, pin_number, pin_bit, mode;
    FlagStatus flag;

    if (argc == 2) {
        if (parse_gpio_pin(argv[1], &gpio_periph, &pin_number) != GPIO_PARSE_SUCCESS) {
            app_print("Invalid GPIO number\r\n");
            goto Usage;
        }

        pin_bit = BIT(pin_number);

        gpio_mode_get(gpio_periph, &mode, pin_bit);
        if (mode == GPIO_MODE_INPUT) {
            flag = gpio_input_bit_get(gpio_periph, pin_bit);
            if (flag == RESET) {
                app_print("+RDGPIO:%s,0,0\r\n", argv[1]);
            } else if (flag == SET) {
                app_print("+RDGPIO:%s,0,1\r\n", argv[1]);
            }
        } else if (mode == GPIO_MODE_OUTPUT) {
            flag = gpio_output_bit_get(gpio_periph, pin_bit);
            if (flag == RESET) {
                app_print("+RDGPIO:%s,1,0\r\n", argv[1]);
            } else if (flag == SET) {
                app_print("+RDGPIO:%s,1,1\r\n", argv[1]);
            }
        } else if (mode == GPIO_MODE_AF) {
            app_print("%s work in alternate function mode\r\n", argv[1]);
            return;
        } else if (mode == GPIO_MODE_ANALOG) {
            app_print("%s work in analog mode\r\n", argv[1]);
            return;
        } else {
            app_print("Unknow mode(%d)\r\n", mode);
            return;
        }
    } else {
        goto Usage;
    }

    app_print("OK\r\n");
    return;

Usage:
    app_print("Usage: read_gpio_level <pin>\r\n");
    app_print("Example:\r\n");
    app_print("       read_gpio_level PB15\r\n");
}

static void gpio_dir_set(uint32_t gpio_periph, uint32_t dir, uint32_t pin)
{
    uint16_t i;
    uint32_t ctl, mode = 0;

    ctl = GPIO_CTL(gpio_periph);

    if (dir == 0) {
        mode = GPIO_MODE_INPUT;
    } else if (dir == 1) {
        mode = GPIO_MODE_OUTPUT;
    }

    for (i = 0U;i < 16U;i++){
        if((1U << i) & pin){
            /* clear the specified pin mode bits */
            ctl &= ~GPIO_MODE_MASK(i);
            /* set the specified pin mode bits */
            ctl |= GPIO_MODE_SET(i, mode);
        }
    }

    GPIO_CTL(gpio_periph) = ctl;
}

void cmd_gpio_dir_set(int argc, char **argv)
{
    uint32_t gpio_periph, pin_number;
    uint32_t pin, dir;

   if (argc == 3) {
        if (parse_gpio_pin(argv[1], &gpio_periph, &pin_number) != GPIO_PARSE_SUCCESS) {
            app_print("Invalid GPIO number\r\n");
            return;
        }

        dir = atoi(argv[2]);
        if (dir > 1) {
            app_print("Invalid dir param\r\n");
            return;
        }

        pin = BIT(pin_number);

        gpio_dir_set(gpio_periph, dir, pin);
    } else {
        goto Usage;
    }

    app_print("OK\r\n");
    return;

Usage:
    app_print("Usage: set_gpio_dir <pin> <direction>\r\n");
    app_print("       <pin>: PA0-PA15, PB0-PB4, PB11-PB13, PB15, PC8, PC13-PC15\r\n");
    app_print("       <level>: 0-INPUT, 1-OUTPUT\r\n");
    app_print("Example:\r\n");
    app_print("       set_gpio_dir PA0 1\r\n");
}

