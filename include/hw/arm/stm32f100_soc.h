/*
 * STM32F100 SoC
 *
 * Copyright (c) 2021 Alexandre Iooss <erdnaxe@crans.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HW_ARM_STM32F100_SOC_H
#define HW_ARM_STM32F100_SOC_H

#include "hw/char/stm32f2xx_usart.h"
#include "hw/misc/stm32f1xx_fsmc.h"
#include "hw/ssi/stm32f2xx_spi.h"
#include "hw/arm/armv7m.h"
#include "qom/object.h"
#include "hw/clock.h"

#define TYPE_STM32F100_SOC "stm32f100-soc"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F100State, STM32F100_SOC)

/* Definitions for high-density value line devices */
#define STM_NUM_USARTS 3
#define STM_NUM_SPIS 3

#define FLASH_BASE_ADDRESS 0x08000000
#define FLASH_SIZE_LD (32 * 1024)
#define FLASH_SIZE_MD (128 * 1024)
#define FLASH_SIZE_HD (512 * 1024)

#define SRAM_BASE_ADDRESS 0x20000000
#define SRAM_SIZE (32 * 1024)

#define PSRAM1_BASE_ADDRESS 0x60000000
#define PSRAM1_SIZE (256 * 1024 * 1024)

/* Supported density value lines */
typedef enum {
    STM32F100_DENSITY_LOW,
    STM32F100_DENSITY_MEDIUM,
    STM32F100_DENSITY_HIGH,
} STM32F100Density;

typedef struct {
    uint32_t cr;
    uint32_t cfgr;
    uint32_t cir;
    uint32_t apb2rstr;
    uint32_t apb1rstr;
    uint32_t ahbenr;
    uint32_t apb2enr;
    uint32_t apb1enr;
    uint32_t bdcr;
    uint32_t csr;
    uint32_t cfgr2;
} STM32F1XXRccState;

struct STM32F100State {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    char *cpu_type;

    ARMv7MState armv7m;

    STM32F2XXUsartState usart[STM_NUM_USARTS];
    STM32F2XXSPIState spi[STM_NUM_SPIS];
    STM32F1XXFsmcState fsmc;
    STM32F1XXRccState rcc;

    MemoryRegion sram;
    MemoryRegion flash;
    MemoryRegion flash_alias;
    MemoryRegion iomem;

    Clock *sysclk;
    Clock *refclk;

    STM32F100Density density;
    uint8_t num_spis;
    uint32_t flash_size;
};

#endif
