/*
 * STM32F100 SoC
 *
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
#include "qemu/module.h"
#include "qemu/log.h"
#include "hw/arm/boot.h"
#include "exec/address-spaces.h"
#include "hw/arm/stm32f100_soc.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-clock.h"
#include "hw/misc/unimp.h"
#include "sysemu/sysemu.h"

/* stm32f100_soc implementation is derived from stm32f205_soc */

static const uint32_t usart_addr[STM_NUM_USARTS] = { 0x40013800, 0x40004400,
    0x40004800 };
static const uint32_t spi_addr[STM_NUM_SPIS] = { 0x40013000, 0x40003800,
    0x40003C00 };
static const uint32_t fsmc_addr = 0xA0000000;

static const int usart_irq[STM_NUM_USARTS] = {37, 38, 39};
static const int spi_irq[STM_NUM_SPIS] = {35, 36, 51};
static const int fsmc_irq = 48;

static uint64_t stm32f100_rcc_read(void *h, hwaddr offset, unsigned size)
{
    STM32F100State *s = (STM32F100State *) h;
    switch (offset) {
    case 0x00:
        return s->rcc.cr;
    case 0x04:
        return s->rcc.cfgr;
    case 0x08:
        return s->rcc.cir;
    case 0x0C:
        return s->rcc.apb2rstr;
    case 0x10:
        return s->rcc.apb1rstr;
    case 0x14:
        return s->rcc.ahbenr;
    case 0x18:
        return s->rcc.apb2enr;
    case 0x1C:
        return s->rcc.apb1enr;
    case 0x20:
        return s->rcc.bdcr;
    case 0x24:
        return s->rcc.csr;
    case 0x2C:
        return s->rcc.cfgr2;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, offset);
    }
    return 0;
}

static void stm32f100_rcc_write(void *h, hwaddr offset, uint64_t value64,
                                unsigned size)
{
    STM32F100State *s = (STM32F100State *) h;
    uint32_t value = value64 & 0xffffffff;

    switch (offset) {
    case 0x00:
        s->rcc.cr = value;
    case 0x04:
        s->rcc.cfgr = value;
    case 0x08:
        s->rcc.cir = value;
    case 0x0C:
        s->rcc.apb2rstr = value;
    case 0x10:
        s->rcc.apb1rstr = value;
    case 0x14:
        s->rcc.ahbenr = value;
    case 0x18:
        s->rcc.apb2enr = value;
    case 0x1C:
        s->rcc.apb1enr = value;
    case 0x20:
        s->rcc.bdcr = value;
    case 0x24:
        s->rcc.csr = value;
    case 0x2C:
        s->rcc.cfgr2 = value;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, offset);
    }
}

static const MemoryRegionOps stm32f100_rcc_ops = {
    .read = stm32f100_rcc_read,
    .write = stm32f100_rcc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f100_soc_initfn(Object *obj)
{
    STM32F100State *s = STM32F100_SOC(obj);
    int i;

    object_initialize_child(obj, "armv7m", &s->armv7m, TYPE_ARMV7M);

    /*
     * All density lines feature the same number of USARTs, so they can be
     * initialized in this function. The number of SPIs is density-dependent,
     * though, so SPIs are initialized in stm32f100_soc_realize().
     */
    for (i = 0; i < STM_NUM_USARTS; i++) {
        object_initialize_child(obj, "usart[*]", &s->usart[i],
                                TYPE_STM32F2XX_USART);
    }

    s->sysclk = qdev_init_clock_in(DEVICE(s), "sysclk", NULL, NULL, 0);
    s->refclk = qdev_init_clock_in(DEVICE(s), "refclk", NULL, NULL, 0);

    /* Default density. May be overridden by the machine or cmdline option */
    s->density = STM32F100_DENSITY_HIGH;

    memset(&s->rcc, 0, sizeof(s->rcc));
    s->rcc.cr = 0x00000083;
    s->rcc.ahbenr = 0x00000014;
    s->rcc.csr = 0x0C000000;
}

static void stm32f100_soc_realize(DeviceState *dev_soc, Error **errp)
{
    STM32F100State *s = STM32F100_SOC(dev_soc);
    DeviceState *dev, *armv7m;
    SysBusDevice *busdev;
    int i;

    if (s->density == STM32F100_DENSITY_HIGH) {
        s->num_spis = 3;
        s->flash_size = FLASH_SIZE_HD;
    } else if (s->density == STM32F100_DENSITY_MEDIUM) {
        s->num_spis = 2;
        s->flash_size = FLASH_SIZE_MD;
    } else {
        s->num_spis = 2;
        s->flash_size = FLASH_SIZE_LD;
    }

    MemoryRegion *system_memory = get_system_memory();

    /*
     * We use s->refclk internally and only define it with qdev_init_clock_in()
     * so it is correctly parented and not leaked on an init/deinit; it is not
     * intended as an externally exposed clock.
     */
    if (clock_has_source(s->refclk)) {
        error_setg(errp, "refclk clock must not be wired up by the board code");
        return;
    }

    if (!clock_has_source(s->sysclk)) {
        error_setg(errp, "sysclk clock must be wired up by the board code");
        return;
    }

    /*
     * TODO: ideally we should model the SoC RCC and its ability to
     * change the sysclk frequency and define different sysclk sources.
     */

    /* The refclk always runs at frequency HCLK / 8 */
    clock_set_mul_div(s->refclk, 8, 1);
    clock_set_source(s->refclk, s->sysclk);

    /*
     * Init flash region
     * Flash starts at 0x08000000 and then is aliased to boot memory at 0x0
     */
    memory_region_init_rom(&s->flash, OBJECT(dev_soc), "STM32F100.flash",
                           s->flash_size, &error_fatal);
    memory_region_init_alias(&s->flash_alias, OBJECT(dev_soc),
                             "STM32F100.flash.alias", &s->flash, 0,
                             s->flash_size);
    memory_region_add_subregion(system_memory, FLASH_BASE_ADDRESS, &s->flash);
    memory_region_add_subregion(system_memory, 0, &s->flash_alias);

    /* Init SRAM region */
    memory_region_init_ram(&s->sram, NULL, "STM32F100.sram", SRAM_SIZE,
                           &error_fatal);
    memory_region_add_subregion(system_memory, SRAM_BASE_ADDRESS, &s->sram);

    /* Init ARMv7m */
    armv7m = DEVICE(&s->armv7m);
    qdev_prop_set_uint32(armv7m, "num-irq", 61);
    qdev_prop_set_string(armv7m, "cpu-type", s->cpu_type);
    qdev_prop_set_bit(armv7m, "enable-bitband", true);
    qdev_connect_clock_in(armv7m, "cpuclk", s->sysclk);
    qdev_connect_clock_in(armv7m, "refclk", s->refclk);
    object_property_set_link(OBJECT(&s->armv7m), "memory",
                             OBJECT(get_system_memory()), &error_abort);
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->armv7m), errp)) {
        return;
    }

    /* Attach UART (uses USART registers) and USART controllers */
    for (i = 0; i < STM_NUM_USARTS; i++) {
        dev = DEVICE(&(s->usart[i]));
        qdev_prop_set_chr(dev, "chardev", serial_hd(i));
        if (!sysbus_realize(SYS_BUS_DEVICE(&s->usart[i]), errp)) {
            return;
        }
        busdev = SYS_BUS_DEVICE(dev);
        sysbus_mmio_map(busdev, 0, usart_addr[i]);
        sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, usart_irq[i]));
    }

    /* Initialize all SPIs supported by the selected density line */
    for (i = 0; i < s->num_spis; i++) {
        object_initialize_child(OBJECT(dev_soc), "spi[*]", &s->spi[i],
                                TYPE_STM32F2XX_SPI);

        dev = DEVICE(&(s->spi[i]));
        if (!sysbus_realize(SYS_BUS_DEVICE(&s->spi[i]), errp)) {
            return;
        }
        busdev = SYS_BUS_DEVICE(dev);
        sysbus_mmio_map(busdev, 0, spi_addr[i]);
        sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, spi_irq[i]));
    }

    /* Declare a simple memory-mapped I/O region for RCC */
    memory_region_init_io(&s->iomem, OBJECT(dev_soc), &stm32f100_rcc_ops, s,
                          "STM32F100.mmio.rcc", 0x400);
    memory_region_add_subregion(system_memory, 0x40021000, &s->iomem);

    /* Declare an I/O region for FSMC */
    if (s->density == STM32F100_DENSITY_HIGH) {
        object_initialize_child(OBJECT(dev_soc), "fsmc", &s->fsmc,
                                TYPE_STM32F1XX_FSMC);

        dev = DEVICE(&s->fsmc);
        if (!sysbus_realize(SYS_BUS_DEVICE(&s->fsmc), errp)) {
            return;
        }
        busdev = SYS_BUS_DEVICE(dev);
        sysbus_mmio_map(busdev, 0, fsmc_addr);
        sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, fsmc_irq));
    }

    create_unimplemented_device("timer[2]",  0x40000000, 0x400);
    create_unimplemented_device("timer[3]",  0x40000400, 0x400);
    create_unimplemented_device("timer[4]",  0x40000800, 0x400);
    create_unimplemented_device("timer[6]",  0x40001000, 0x400);
    create_unimplemented_device("timer[7]",  0x40001400, 0x400);
    create_unimplemented_device("timer[12]", 0x40001800, 0x400);
    create_unimplemented_device("timer[13]", 0x40001C00, 0x400);
    create_unimplemented_device("timer[14]", 0x40002000, 0x400);
    create_unimplemented_device("RTC",       0x40002800, 0x400);
    create_unimplemented_device("WWDG",      0x40002C00, 0x400);
    create_unimplemented_device("IWDG",      0x40003000, 0x400);
    create_unimplemented_device("UART4",     0x40004C00, 0x400);
    create_unimplemented_device("UART5",     0x40005000, 0x400);
    create_unimplemented_device("I2C1",      0x40005400, 0x400);
    create_unimplemented_device("I2C2",      0x40005800, 0x400);
    create_unimplemented_device("BKP",       0x40006C00, 0x400);
    create_unimplemented_device("PWR",       0x40007000, 0x400);
    create_unimplemented_device("DAC",       0x40007400, 0x400);
    create_unimplemented_device("CEC",       0x40007800, 0x400);
    create_unimplemented_device("AFIO",      0x40010000, 0x400);
    create_unimplemented_device("EXTI",      0x40010400, 0x400);
    create_unimplemented_device("GPIOA",     0x40010800, 0x400);
    create_unimplemented_device("GPIOB",     0x40010C00, 0x400);
    create_unimplemented_device("GPIOC",     0x40011000, 0x400);
    create_unimplemented_device("GPIOD",     0x40011400, 0x400);
    create_unimplemented_device("GPIOE",     0x40011800, 0x400);
    create_unimplemented_device("GPIOF",     0x40011C00, 0x400);
    create_unimplemented_device("GPIOG",     0x40012000, 0x400);
    create_unimplemented_device("ADC1",      0x40012400, 0x400);
    create_unimplemented_device("timer[1]",  0x40012C00, 0x400);
    create_unimplemented_device("timer[15]", 0x40014000, 0x400);
    create_unimplemented_device("timer[16]", 0x40014400, 0x400);
    create_unimplemented_device("timer[17]", 0x40014800, 0x400);
    create_unimplemented_device("DMA1",      0x40020000, 0x400);
    create_unimplemented_device("DMA2",      0x40020400, 0x400);
    create_unimplemented_device("Flash Int", 0x40022000, 0x400);
    create_unimplemented_device("CRC",       0x40023000, 0x400);
}

static Property stm32f100_soc_properties[] = {
    DEFINE_PROP_STRING("cpu-type", STM32F100State, cpu_type),
    DEFINE_PROP_END_OF_LIST(),
};

static char *stm32f100_get_density(Object *obj, Error **errp)
{
    STM32F100State *s = STM32F100_SOC(obj);

    switch (s->density) {
    case STM32F100_DENSITY_LOW:
        return g_strdup("low");
    case STM32F100_DENSITY_MEDIUM:
        return g_strdup("medium");
    case STM32F100_DENSITY_HIGH:
        return g_strdup("high");
    default:
        g_assert_not_reached();
    }
}

static void stm32f100_set_density(Object *obj, const char *value, Error **errp)
{
    STM32F100State *s = STM32F100_SOC(obj);

    if (!strcmp(value, "low")) {
        s->density = STM32F100_DENSITY_LOW;
    } else if (!strcmp(value, "medium")) {
        s->density = STM32F100_DENSITY_MEDIUM;
    } else if (!strcmp(value, "high")) {
        s->density = STM32F100_DENSITY_HIGH;
    } else {
        error_setg(errp, "Invalid density value '%s'", value);
        error_append_hint(errp, "Valid values: 'low', 'medium', 'high'\n");
    }
}

static void stm32f100_soc_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = stm32f100_soc_realize;
    device_class_set_props(dc, stm32f100_soc_properties);

    object_class_property_add_str(oc, "density", stm32f100_get_density,
        stm32f100_set_density);
    object_class_property_set_description(oc, "density",
        "Set the STM32F100 density line device. "
        "Valid values are 'low', 'medium', and 'high' (default).");
}

static const TypeInfo stm32f100_soc_info = {
    .name          = TYPE_STM32F100_SOC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F100State),
    .instance_init = stm32f100_soc_initfn,
    .class_init    = stm32f100_soc_class_init,
};

static void stm32f100_soc_types(void)
{
    type_register_static(&stm32f100_soc_info);
}

type_init(stm32f100_soc_types)
