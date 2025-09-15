
#include "efuse_drv.h"

uint32_t efuse_read_bit(int32_t bit_addr)
{
    if (bit_addr > 127)
        return 0;
    uint32_t reg_idx = (bit_addr >> 5);
    uint32_t reg_addr = EFUSE_DATA0_ADDR - reg_idx * 4;
    uint16_t reg_bit = bit_addr & 0x1F;
    return ((*(uint32_t *)(reg_addr)) >> (reg_bit)) & 1;
}

uint32_t efuse_read_reg(int32_t reg_idx)
{
    if (reg_idx > 3)
        return 0;
    return (*(uint32_t *)(EFUSE_DATA0_ADDR - reg_idx * 4));
}

bool efuse_write_bit(int32_t bit_addr)
{
    if (bit_addr > 127 || bit_addr <= 4)
    {
        return false;
    }
    if (1 == efuse_read_bit(bit_addr))
    {
        return true;
    }

    // write
    REG_EFUSE_CTRL = bit_addr | EFUSE_CTRL_WR_BIT;
    for (uint32_t i = 0; i < EFUSE_WRITE_DLY; i++)
    {
        asm("nop");
    }

    // read
    REG_EFUSE_CTRL = EFUSE_CTRL_RD_BIT;
    for (uint32_t i = 0; i < EFUSE_READ_DLY; i++)
    {
        asm("nop");
    }
    if (1 == efuse_read_bit(bit_addr))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool efuse_write_reg(int32_t reg_idx, uint32_t reg_data)
{
    if (reg_idx > 3)
    {
        return false;
    }

    uint32_t bit_addr_offset = reg_idx * 32, bit_addr;
    for (uint32_t i = 0; i < 32; i++)
    {
        if (1 & (reg_data >> i))
        {
            bit_addr = bit_addr_offset + i;
            if (bit_addr <= 4)
                continue;
            if (!efuse_write_bit(bit_addr))
            {
                return false;
            }
        }
    }
    return true;
}