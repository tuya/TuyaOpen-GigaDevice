/*!
    \file    reg_access.h
    \brief   File implementing the basic primitives for register accesses.

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

#ifndef REG_ACCESS_H_
#define REG_ACCESS_H_

#include "util.h"

/*
 * MACROS
 ****************************************************************************************
 */
// Macro for register address
#define SYS_CFG_BASE            0x40013800
#define RCC_BASE                0x40023800
#define MAC_REG_BASE            0x40030000
#define PHY_MDM_REG_BASE        0x40040000
// rf interfece top base address
#define PHY_RFTOP_REG_BASE      0x4000cc00
#define PHY_LA_REG_BASE         0x4000cd00
#define PHY_K_MEM_ADDR          0x20048000

// Macro to read a platform register
#define REG_PL_RD(addr)              (*(volatile uint32_t *)(HW2CPU(addr)))

// Macro to write a platform register
#define REG_PL_WR(addr, value)       (*(volatile uint32_t *)(HW2CPU(addr))) = (value)

static inline uint8_t REG_PL_RD1(uint32_t addr)
{
    return ((REG_PL_RD((addr & (~0x03))) >> ((addr & 0x03) << 3)) & 0xFF);
}

static inline void REG_PL_WR1(uint32_t addr, uint32_t value)
{
    REG_PL_RD((addr & (~0x03))) = (REG_PL_RD((addr & (~0x03)))
                                & (~(0xFF << ((addr & 0x03) << 3))))
                                | (value << ((addr & 0x03) << 3));
}

static inline void REG_PL_WR4_MASK(uint32_t addr, uint32_t value, uint32_t mask)
{
    (*(volatile uint32_t *)(HW2CPU(addr))) = (REG_PL_RD((addr & (~0x03))) & ~mask) | (value & mask);
}

#define REG_SYS_CFG_WR(addr, value)             REG_PL_WR(SYS_CFG_BASE + (addr), value)
#define REG_SYS_CFG_RD(addr)                    REG_PL_RD(SYS_CFG_BASE + (addr))

#define REG_RCC_WR(addr, value)                 REG_PL_WR(RCC_BASE + (addr), value)
#define REG_RCC_RD(addr)                        REG_PL_RD(RCC_BASE + (addr))

#define REG_MAC_WR(addr, value)                 REG_PL_WR(MAC_REG_BASE + (addr), value)
#define REG_MAC_WR1(addr, value)                REG_PL_WR1(MAC_REG_BASE + (addr), value)
#define REG_MAC_RD(addr)                        REG_PL_RD(MAC_REG_BASE + (addr))
#define REG_MAC_RD1(addr)                       REG_PL_RD1(MAC_REG_BASE + (addr))

#define REG_PHY_MDM_WR(addr, value)             REG_PL_WR(PHY_MDM_REG_BASE + (addr), value)
#define REG_PHY_MDM_WR1(addr, value)            REG_PL_WR1(PHY_MDM_REG_BASE + (addr), value)
#define REG_PHY_MDM_WR_MASK(addr, value, mask)  REG_PL_WR4_MASK(PHY_MDM_REG_BASE + (addr), value, mask)
#define REG_PHY_MDM_RD(addr)                    REG_PL_RD(PHY_MDM_REG_BASE + (addr))
#define REG_PHY_MDM_RD1(addr)                   REG_PL_RD1(PHY_MDM_REG_BASE + (addr))

#define REG_PHY_RFTOP_WR(addr, value)           REG_PL_WR(PHY_RFTOP_REG_BASE + (addr), value)
#define REG_PHY_RFTOP_WR1(addr, value)          REG_PL_WR1(PHY_RFTOP_REG_BASE + (addr), value)
#define REG_PHY_RFTOP_RD(addr)                  REG_PL_RD(PHY_RFTOP_REG_BASE + (addr))
#define REG_PHY_RFTOP_RD1(addr)                 REG_PL_RD1(PHY_RFTOP_REG_BASE + (addr))

#define REG_PHY_LA_WR(addr, value)              REG_PL_WR(PHY_LA_REG_BASE + (addr), value)
#define REG_PHY_LA_RD(addr)                     REG_PL_RD(PHY_LA_REG_BASE + (addr))

#endif // REG_ACCESS_H_
