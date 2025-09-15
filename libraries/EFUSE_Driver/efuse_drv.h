#ifndef _EFUSE_DEV_H_
#define _EFUSE_DEV_H_

#include "stdint.h"
#include "stdbool.h"

/**
 * @brief EFUSE_CTRL_REG
 *  bit0-6: efuse bit addr, bit 7:write bit ,8 : read
 */
#define REG_EFUSE_CTRL (*(volatile uint32_t *)(0x1a1070A0))
#define EFUSE_CTRL_WR_BIT (1 << 7)
#define EFUSE_CTRL_RD_BIT (1 << 8)

// #define EFUSE_CDIV_ADDR (*(volatile uint32_t *)(0x1a1070A4))
// #define EFUSE_PCTRL_ADDR (*(volatile uint32_t *)(0x1a1070A8))

/**
 * @brief 128 efuse  bit
 * @note bit 4:0  is protected, user are prohibited from using
 *       bit 7:5 used for soft start. delay time =  bit[7:5] * 2 second 
 */
#define EFUSE_DATA3_ADDR (0x1a1070AC) // 127:96
#define EFUSE_DATA2_ADDR (0x1a1070B0) // 95:64
#define EFUSE_DATA1_ADDR (0x1a1070B4) // 63:32
#define EFUSE_DATA0_ADDR (0x1a1070B8) // 31:0

#define EFUSE_WRITE_DLY (5000)
#define EFUSE_READ_DLY (5000)

/**
 * @brief read efuse bit
 * @param bit_addr:0-127
 * @return bit value
 */
uint32_t efuse_read_bit(int32_t bit_addr);
/**
 * @brief read efuse 32bit reg
 * @param reg_idx:0-3
 * @return 32bit reg value
 */
uint32_t efuse_read_reg(int32_t reg_idx);
/**
 * @brief write efuse bit,
 * @param bit_addr:0-127
 * @return true/false
 */
bool efuse_write_bit(int32_t bit_addr);
/**
 * @brief read efuse 32bit reg
 * @param reg_idx:0-3
 * @return true/false
 */
bool efuse_write_reg(int32_t reg_idx, uint32_t reg_data);

#endif /* _EFUSE_DEV_H_ */