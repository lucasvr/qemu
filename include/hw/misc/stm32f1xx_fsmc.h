/*
 * STM32F1xx FSMC
 *
 * Copyright (c) 2023 Lucas C. Villa Real <lucas@osdyne.com>
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

#ifndef HW_STM32F1XX_FSMC_H
#define HW_STM32F1XX_FSMC_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define FSMC_BCR1  0x000
#define FSMC_BCR2  0x008
#define FSMC_BCR3  0x010
#define FSMC_BCR4  0x018
#define FSMC_BTR1  0x004
#define FSMC_BTR2  0x00C
#define FSMC_BTR3  0x014
#define FSMC_BTR4  0x01C
#define FSMC_BWTR1 0x104
#define FSMC_BWTR2 0x10C
#define FSMC_BWTR3 0x114
#define FSMC_BWTR4 0x11C
#define NUM_BANKS  4

#define TYPE_STM32F1XX_FSMC "stm32f1xx-fsmc"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F1XXFsmcState, STM32F1XX_FSMC)

struct STM32F1XXFsmcState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint32_t fsmc_bcr[NUM_BANKS];
    uint32_t fsmc_btr[NUM_BANKS];
    uint32_t fsmc_bwtr[NUM_BANKS];

    qemu_irq irq;
};

#endif
