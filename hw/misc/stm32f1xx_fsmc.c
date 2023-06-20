/*
 * STM32F1XX FSMC
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

#include "qemu/osdep.h"
#include "hw/misc/stm32f1xx_fsmc.h"
#include "qemu/log.h"
#include "qemu/module.h"

static void stm32f1xx_fsmc_reset(DeviceState *dev)
{
    STM32F1XXFsmcState *s = STM32F1XX_FSMC(dev);

    s->fsmc_bcr[0] = 0x000030DB;
    for (int i=1; i<4; ++i)
        s->fsmc_bcr[i] = 0x000030D2;
    for (int i=0; i<4; ++i) {
        s->fsmc_btr[i] = 0xffffffff;
        s->fsmc_bwtr[i] = 0xffffffff;
    }
}

static uint64_t stm32f1xx_fsmc_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F1XXFsmcState *s = opaque;

    switch (addr) {
    case FSMC_BCR1:
        return s->fsmc_bcr[0];
    case FSMC_BCR2:
        return s->fsmc_bcr[1];
    case FSMC_BCR3:
        return s->fsmc_bcr[2];
    case FSMC_BCR4:
        return s->fsmc_bcr[3];
    case FSMC_BTR1:
        return s->fsmc_btr[0];
    case FSMC_BTR2:
        return s->fsmc_btr[1];
    case FSMC_BTR3:
        return s->fsmc_btr[2];
    case FSMC_BTR4:
        return s->fsmc_btr[3];
    case FSMC_BWTR1:
        return s->fsmc_bwtr[0];
    case FSMC_BWTR2:
        return s->fsmc_bwtr[1];
    case FSMC_BWTR3:
        return s->fsmc_bwtr[2];
    case FSMC_BWTR4:
        return s->fsmc_bwtr[3];
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
    }

    return 0;
}

static void stm32f1xx_fsmc_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F1XXFsmcState *s = opaque;
    uint32_t value = val64 & 0xffffffff;

    switch (addr) {
    case FSMC_BCR1:
        s->fsmc_bcr[0] = value;
    case FSMC_BCR2:
        s->fsmc_bcr[1] = value;
    case FSMC_BCR3:
        s->fsmc_bcr[2] = value;
    case FSMC_BCR4:
        s->fsmc_bcr[3] = value;
    case FSMC_BTR1:
        s->fsmc_btr[0] = value;
    case FSMC_BTR2:
        s->fsmc_btr[1] = value;
    case FSMC_BTR3:
        s->fsmc_btr[2] = value;
    case FSMC_BTR4:
        s->fsmc_btr[3] = value;
    case FSMC_BWTR1:
        s->fsmc_bwtr[0] = value;
    case FSMC_BWTR2:
        s->fsmc_bwtr[1] = value;
    case FSMC_BWTR3:
        s->fsmc_bwtr[2] = value;
    case FSMC_BWTR4:
        s->fsmc_bwtr[3] = value;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
    }
}

static const MemoryRegionOps stm32f1xx_fsmc_ops = {
    .read = stm32f1xx_fsmc_read,
    .write = stm32f1xx_fsmc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f1xx_fsmc_init(Object *obj)
{
    STM32F1XXFsmcState *s = STM32F1XX_FSMC(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f1xx_fsmc_ops, s,
                          TYPE_STM32F1XX_FSMC, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);
}

static void stm32f1xx_fsmc_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->reset = stm32f1xx_fsmc_reset;
}

static const TypeInfo stm32f1xx_fsmc_info = {
    .name          = TYPE_STM32F1XX_FSMC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F1XXFsmcState),
    .instance_init = stm32f1xx_fsmc_init,
    .class_init    = stm32f1xx_fsmc_class_init,
};

static void stm32f1xx_fsmc_register_types(void)
{
    type_register_static(&stm32f1xx_fsmc_info);
}

type_init(stm32f1xx_fsmc_register_types)
