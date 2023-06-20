/*
 * ST generic STM32F1 board
 *
 * Copyright (c) 2023 Lucas C. Villa Real <lucas@osdyne.com>
 * Copyright (c) 2021 Alexandre Iooss <erdnaxe@crans.org>
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
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

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-clock.h"
#include "qemu/error-report.h"
#include "exec/address-spaces.h"
#include "hw/arm/stm32f100_soc.h"
#include "hw/arm/boot.h"

/* Main SYSCLK frequency in Hz (24MHz) */
#define SYSCLK_FRQ 24000000ULL

static void stm32f1_generic_init(MachineState *machine)
{
    MemoryRegion *psram1;
    STM32F100State *s;
    DeviceState *dev;
    Clock *sysclk;

    /* This clock doesn't need migration because it is fixed-frequency */
    sysclk = clock_new(OBJECT(machine), "SYSCLK");
    clock_set_hz(sysclk, SYSCLK_FRQ);

    /*
     * Note that we don't set the "density" property so that the default
     * value ("high") can be changed via "-global stm32f100-soc.density=..."
     */
    dev = qdev_new(TYPE_STM32F100_SOC);
    qdev_prop_set_string(dev, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m3"));
    qdev_connect_clock_in(dev, "sysclk", sysclk);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    s = STM32F100_SOC(OBJECT(dev));
    armv7m_load_kernel(ARM_CPU(first_cpu),
                       machine->kernel_filename,
                       0, s->flash_size);

    /* Allow assigning more RAM via FSMC on high-density devices */
    if (s->density == STM32F100_DENSITY_HIGH) {
        assert(machine->ram_size <= PSRAM1_SIZE);
        psram1 = g_new(MemoryRegion, 1);
        memory_region_init_ram(psram1, NULL, "STM32F1-generic.psram1",
                               machine->ram_size, &error_fatal);
        memory_region_add_subregion(get_system_memory(),
                                    PSRAM1_BASE_ADDRESS, psram1);
    }
}

static void stm32f1_generic_machine_init(MachineClass *mc)
{
    mc->desc = "STM32F1 generic (Cortex-M3)";
    mc->init = stm32f1_generic_init;
}

DEFINE_MACHINE("stm32f1-generic", stm32f1_generic_machine_init)
