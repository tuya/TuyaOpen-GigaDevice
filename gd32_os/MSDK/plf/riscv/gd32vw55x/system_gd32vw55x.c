/*!
    \file    system_gd32vw55x.c
    \brief   RISC-V Device Peripheral Access Layer Source File for
             GD32VW55x Device Series

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

/* This file refers the RISC-V standard, some adjustments are made according to GigaDevice chips */

#include "app_cfg.h"
#include "gd32vw55x.h"
#include "system_gd32vw55x.h"

/* system frequency define */
#define __IRC16M          (IRC16M_VALUE)            /* internal 16 MHz RC oscillator frequency */
#define __HXTAL           (HXTAL_VALUE)             /* high speed crystal oscillator frequency */
#define __SYS_OSC_CLK     (__IRC16M)                /* main oscillator frequency */

/* select a system clock by uncommenting the following line */
//#define __SYSTEM_CLOCK_IRC16M                     (uint32_t)(__IRC16M)
//#define __SYSTEM_CLOCK_HXTAL                      (uint32_t)(__HXTAL)
//#define __SYSTEM_CLOCK_48M_PLLDIG_IRC16M          (uint32_t)(48000000)
//#define __SYSTEM_CLOCK_160M_PLLDIG_IRC16M         (uint32_t)(160000000)
//#define __SYSTEM_CLOCK_160M_PLLDIG_HXTAL          (uint32_t)(160000000)
//#define __SYSTEM_CLOCK_48M_PLLDIG_HXTAL           (uint32_t)(48000000)
#ifdef CONFIG_PLATFORM_FPGA
#define __SYSTEM_CLOCK_50M_PLLDIG                   (uint32_t)(__HXTAL)
#else
#if PLATFORM_CRYSTAL == CRYSTAL_40M
#define __SYSTEM_CLOCK_160M_PLLDIG_40M_HXTAL        (uint32_t)(160000000)
#elif PLATFORM_CRYSTAL == CRYSTAL_26M
#define __SYSTEM_CLOCK_160M_PLLDIG_26M_HXTAL        (uint32_t)(160000000)
#elif PLATFORM_CRYSTAL == CRYSTAL_48M
#define __SYSTEM_CLOCK_160M_PLLDIG_48M_HXTAL        (uint32_t)(160000000)
#endif
// #define __SYSTEM_CLOCK_40M_PLLDIG_40M_HXTAL         (uint32_t)(40000000)
#endif

#ifdef CONFIG_RF_TEST_SUPPORT
#undef __SYSTEM_CLOCK_160M_PLLDIG_40M_HXTAL
#define __SYSTEM_CLOCK_40M_PLLDIG_40M_HXTAL         (uint32_t)(40000000)
#endif

#define HXTALSTB_DELAY    {                                   \
                              volatile uint32_t i;            \
                              for(i=0; i<0x20; i++){          \
                              }                               \
                          }

/* set the system clock frequency and declare the system clock configuration function */
#ifdef __SYSTEM_CLOCK_IRC16M
uint32_t SystemCoreClock = __SYSTEM_CLOCK_IRC16M;
static void system_clock_16m_irc16m(void);
#elif defined (__SYSTEM_CLOCK_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_HXTAL;
static void system_clock_hxtal(void);
#elif defined (__SYSTEM_CLOCK_48M_PLLDIG_IRC16M)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_48M_PLLDIG_IRC16M;
static void system_clock_48m_irc16m(void);
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_IRC16M)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_160M_PLLDIG_IRC16M;
static void system_clock_160m_irc16m(void);
#elif defined (__SYSTEM_CLOCK_48M_PLLDIG_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_48M_PLLDIG_HXTAL;
static void system_clock_48m_hxtal(void);
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_160M_PLLDIG_HXTAL;
static void system_clock_160m_hxtal(void);
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_40M_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_160M_PLLDIG_40M_HXTAL;
static void system_clock_160m_40m_hxtal(void);
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_26M_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_160M_PLLDIG_26M_HXTAL;
static void system_clock_160m_26m_hxtal(void);
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_48M_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_160M_PLLDIG_48M_HXTAL;
static void system_clock_160m_48m_hxtal(void);
#elif defined (__SYSTEM_CLOCK_40M_PLLDIG_40M_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_40M_PLLDIG_40M_HXTAL;
static void system_clock_40m_40m_hxtal(void);
#elif defined (__SYSTEM_CLOCK_50M_PLLDIG)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_50M_PLLDIG;
static void system_clock_50m_plldig(void);
#endif /* __SYSTEM_CLOCK_IRC16M */

/* configure the system clock */
void system_clock_config(void);

/*!
    \brief      setup the microcontroller system, initialize the system
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SystemInit (void)
{
    /* reset the RCU clock configuration to the default reset state */
    /* set IRC16MEN bit */
    RCU_CTL |= RCU_CTL_IRC16MEN;
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    /* reset CFG0 register */
    RCU_CFG0 = 0x00000000U;
    /* reset RCU_CFG1_RFPLLCALEN RCU_CFG1_RFPLLPU */
    RCU_CFG1 &= ~(RCU_CFG1_RFPLLCALEN | RCU_CFG1_RFPLLPU);

    /* reset PLLDIGEN, PLLDIGPU, RCU_CTL_RFCKMEN and HXTALEN bits */
    RCU_CTL &= ~(RCU_CTL_PLLDIGEN | RCU_CTL_PLLDIGPU | RCU_CTL_RFCKMEN | RCU_CTL_HXTALEN);

    /* reset PLLCFGR register */
    RCU_PLL = 0x00000000U;
    RCU_PLLDIGCFG0 = 0x00000000U;
    RCU_PLLDIGCFG1 = 0x07800000U;

    /* disable all interrupts */
    RCU_INT = 0x00000000U;

    /* configure the System clock source, PLL Multiplier and Divider factors,
       AHB/APBx prescalers and Flash settings */
    system_clock_config();

    /* set mtime clock clksrc 1:systemclock, 0:systemclock/4 */
    SysTimer_SetControlValue(SysTimer_GetControlValue() | SysTimer_MTIMECTL_CLKSRC_Msk);
    /* __ICACHE_PRESENT and __DCACHE_PRESENT are defined in demosoc.h */
    MInvalICache();
}
/*!
    \brief      configure the system clock
    \param[in]  none
    \param[out] none
    \retval     none
*/
void system_clock_config(void)
{
#ifdef __SYSTEM_CLOCK_IRC16M
    system_clock_16m_irc16m();
#elif defined (__SYSTEM_CLOCK_HXTAL)
    system_clock_hxtal();
#elif defined (__SYSTEM_CLOCK_48M_PLLDIG_IRC16M)
    system_clock_48m_irc16m();
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_IRC16M)
    system_clock_160m_irc16m();
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_HXTAL)
    system_clock_160m_hxtal();
#elif defined (__SYSTEM_CLOCK_48M_PLLDIG_HXTAL)
    system_clock_48m_hxtal();
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_40M_HXTAL)
    system_clock_160m_40m_hxtal();
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_26M_HXTAL)
    system_clock_160m_26m_hxtal();
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_48M_HXTAL)
    system_clock_160m_48m_hxtal();
#elif defined (__SYSTEM_CLOCK_40M_PLLDIG_40M_HXTAL)
    system_clock_40m_40m_hxtal();
#elif defined (__SYSTEM_CLOCK_50M_PLLDIG)
    system_clock_50m_plldig();
#endif /* __SYSTEM_CLOCK_IRC16M */
}

#ifdef __SYSTEM_CLOCK_IRC16M
/*!
    \brief      configure the system clock to 16M by IRC16M
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_16m_irc16m(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    /* enable IRC16M */
    RCU_CTL |= RCU_CTL_IRC16MEN;

    /* wait until IRC16M is stable or the startup time is longer than IRC16M_STARTUP_TIMEOUT */
    do{
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_IRC16MSTB);
    }while((0U == stab_flag) && (IRC16M_STARTUP_TIMEOUT != timeout));

    if(0U == (RCU_CTL & RCU_CTL_IRC16MSTB)){
        /* if fail */
        while(1){
        }
    }

    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV1;

    /* select IRC16M as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_IRC16M;

    /* wait until IRC16M is selected as system clock */
    while(RCU_SCSS_IRC16M != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}

#elif defined (__SYSTEM_CLOCK_HXTAL)
/*!
    \brief      configure the system clock to HXTAL
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_hxtal(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    /* power up the HXTAL */
    RCU_CTL |= RCU_CTL_HXTALPU;
    /* enable HXTAL */
    RCU_CTL |= RCU_CTL_HXTALEN;
    HXTALSTB_DELAY
    RCU_CTL |= RCU_CTL_HXTALREADY;

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    // do {
    //     timeout++;
    //     stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    // } while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

    // if(0U == (RCU_CTL & RCU_CTL_HXTALSTB)) {
    //     /* if fail */
    //     while(1) {
    //     }
    // }

    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV1;

    /* select HXTAL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_HXTAL;

    /* wait until HXTAL is selected as system clock */
    while(RCU_SCSS_HXTAL != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}
#elif defined (__SYSTEM_CLOCK_48M_PLLDIG_IRC16M)
/*!
    \brief      configure the system clock to 48M by PLL which selects IRC16M as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_48m_irc16m(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    /* enable IRC16M */
    RCU_CTL |= (RCU_CTL_IRC16MEN | RCU_CTL_IRC16MRFON);

    /* wait until IRC16M is stable or the startup time is longer than IRC16M_STARTUP_TIMEOUT */
    do{
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_IRC16MSTB);
    }while((0U == stab_flag) && (IRC16M_STARTUP_TIMEOUT != timeout));

    if(0U == (RCU_CTL & RCU_CTL_IRC16MSTB)){
        /* if fail */
        while(1){
        }
    }

    /* IRC16M is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB/1 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV1;

    RCU_PLL |= RCU_PLLSRC_IRC16M;

    /* 960M = 16MHz*60 */
    RCU_PLLDIGCFG1 = ((960 << 21) / 16) & 0x7FFFFFFF;
    /* PLLDIG OUT = 480M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_480M );
    /* SYS clock = 48M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_SYS_DIV10 );

    /* enable PLLDIG */
    RCU_CFG1 |= (RCU_CFG1_BGPU);
    RCU_CTL |= (RCU_CTL_PLLDIGEN | RCU_CTL_PLLDIGPU);

    /* wait until PLLDIG is stable */
    while(0U == (RCU_CTL & RCU_CTL_PLLDIGSTB)){
    }

    /* select PLLDIG as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLDIG;

    /* wait until PLLDIG is selected as system clock */
    while(RCU_SCSS_PLLDIG != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_IRC16M)
/*!
    \brief      configure the system clock to 160M by PLL which selects IRC16M as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_160m_irc16m(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    /* enable IRC16M */
    RCU_CTL |= (RCU_CTL_IRC16MEN | RCU_CTL_IRC16MRFON);

    /* wait until IRC16M is stable or the startup time is longer than IRC16M_STARTUP_TIMEOUT */
    do{
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_IRC16MSTB);
    }while((0U == stab_flag) && (IRC16M_STARTUP_TIMEOUT != timeout));

    /* if fail */
    if(0U == (RCU_CTL & RCU_CTL_IRC16MSTB)){
        while(1){
        }
    }

    /* IRC16M is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    RCU_PLL |= RCU_PLLSRC_IRC16M;

    /* 960M = 16MHz*60 */
    RCU_PLLDIGCFG1 = ((960 << 21) / 16) & 0x7FFFFFFF;
    /* PLLDIG OUT = 480M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_480M );
    /* SYS clock = 160M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_SYS_DIV3 );

    /* enable PLLDIG */
    RCU_CFG1 |= (RCU_CFG1_BGPU);
    RCU_CTL |= (RCU_CTL_PLLDIGEN | RCU_CTL_PLLDIGPU);

    /* wait until PLLDIG is stable */
    while(0U == (RCU_CTL & RCU_CTL_PLLDIGSTB)){
    }

    /* select PLLDIG as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLDIG;

    /* wait until PLLDIG is selected as system clock */
    while(RCU_SCSS_PLLDIG != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_HXTAL)
/*!
    \brief      configure the system clock to 160M by PLL which selects HXTAL as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_160m_hxtal(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    /* power up HXTAL */
    RCU_CTL |= RCU_CTL_HXTALPU;
    /* enable HXTAL */
    RCU_CTL |= RCU_CTL_HXTALEN;
    HXTALSTB_DELAY
    RCU_CTL |= RCU_CTL_HXTALREADY;

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    // do {
    //     timeout++;
    //     stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    // } while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

    // if(0U == (RCU_CTL & RCU_CTL_HXTALSTB)) {
    //     /* if fail */
    //     while(1) {
    //     }
    // }

    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    RCU_PLL |= RCU_PLLSRC_HXTAL;

    /* PLLDIG = 960/HXTAL */
    RCU_PLLDIGCFG1 = ((960 << 21) / (HXTAL_VALUE/1000000)) & 0x7FFFFFFF;

    /* PLLDIG OUT = 480M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_480M );
    /* SYS clock = 160M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_SYS_DIV3 );
    /* enable PLLDIG */
    RCU_CFG1 |= (RCU_CFG1_BGPU);

    rcu_periph_clock_enable(RCU_RF);
    /* enable PLLS_CFG_PLLS_VREF_SEL_BG */
    REG32(0x40017814) |= 0x4000000;
    /* set PLLS_CFG2_PLLS_SD_MOD_MASK to 0x3 */
    REG32(0x400178D8) |= 0x6;

    /* enable PLL */
    RCU_CFG1 |= (RCU_CFG1_RFPLLCALEN | RCU_CFG1_BGPU);
    RCU_CTL |= (RCU_CTL_PLLDIGEN | RCU_CTL_PLLDIGPU);
    /* wait until PLLDIG is stable */
    while(0U == (RCU_CTL & RCU_CTL_PLLDIGSTB)){
    }
    /* select PLLDIG as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLDIG;

    /* wait until PLLDIG is selected as system clock */
    while(RCU_SCSS_PLLDIG != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}
#elif defined (__SYSTEM_CLOCK_48M_PLLDIG_HXTAL)
/*!
    \brief      configure the system clock to 48M by PLL which selects HXTAL as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_48m_hxtal(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    /* power up HXTAL */
    RCU_CTL |= RCU_CTL_HXTALPU;
    /* enable HXTAL */
    RCU_CTL |= RCU_CTL_HXTALEN;
    HXTALSTB_DELAY
    RCU_CTL |= RCU_CTL_HXTALREADY;

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    // do {
    //     timeout++;
    //     stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    // } while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

    // if (0U == (RCU_CTL & RCU_CTL_HXTALSTB)) {
    //     /* if fail */
    //     while(1) {
    //     }
    // }

    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    RCU_PLL |= RCU_PLLSRC_HXTAL;

    /* PLLDIG = 960/HXTAL */
    RCU_PLLDIGCFG1 = ((960 << 21) / (HXTAL_VALUE/1000000)) & 0x7FFFFFFF;

    /* PLLDIG OUT = 480M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_480M );
    /* SYS clock = 48M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_SYS_DIV10 );

    rcu_periph_clock_enable(RCU_RF);
    /* enable PLLS_CFG_PLLS_VREF_SEL_BG */
    REG32(0x40017814) |= 0x4000000;
    /* set PLLS_CFG2_PLLS_SD_MOD_MASK to 0x3 */
    REG32(0x400178D8) |= 0x6;

    /* enable PLL */
    RCU_CFG1 |= (RCU_CFG1_RFPLLCALEN | RCU_CFG1_BGPU);
    RCU_CTL |= (RCU_CTL_PLLDIGEN | RCU_CTL_PLLDIGPU);
    /* wait until PLL is stable */
    while(0U == (RCU_CTL & RCU_CTL_PLLDIGSTB)){
    }
    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLDIG;

    /* wait until PLL is selected as system clock */
    while(RCU_SCSS_PLLDIG != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}
#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_40M_HXTAL)
/*!
    \brief      configure the system clock to 120M by PLL which selects HXTAL(40M) as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_160m_40m_hxtal(void)
{
#if 0
    uint32_t stab_flag = 0U;
    uint32_t timeout = 0U;
#endif
    /* power up HXTAL */
    RCU_CTL |= RCU_CTL_HXTALPU;
    /* enable HXTAL */
    RCU_CTL |= RCU_CTL_HXTALEN;

    /*
     * HW team said that waiting HXTAL stable is not necessary to wait.
     * But it's best to wait it for safety.
     * We hope to setup HSE as fastly for LPDS, we mark it do more test.
     */
#if 0
    HXTALSTB_DELAY

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    do{
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    }while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

    /* if fail */
    if(0U == (RCU_CTL & RCU_CTL_HXTALSTB)){
        while(1){
        }
    }
#endif

    RCU_CTL |= RCU_CTL_HXTALREADY;

    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    RCU_PLL |= RCU_PLLSRC_HXTAL;

    /* PLLDIG = 960/HSE */
    RCU_PLLDIGCFG1 = ((960 << 21) / (HXTAL_VALUE/1000000)) & 0x7FFFFFFF;
    /* PLLDIG OUT = 480M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_480M );
    /* SYS clock = 160M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_SYS_DIV3 );

    /* enable PLL */
    RCU_CFG1 |= (RCU_CFG1_RFPLLCALEN | RCU_CFG1_BGPU);
    RCU_CTL |= (RCU_CTL_PLLDIGEN | RCU_CTL_PLLDIGPU);
    /* wait until PLL is stable */
    while(0U == (RCU_CTL & RCU_CTL_PLLDIGSTB)){
    }
    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLDIG;

    /* wait until PLL is selected as system clock */
    while(RCU_SCSS_PLLDIG != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}

#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_26M_HXTAL)
/*!
    \brief      configure the system clock to 120M by PLL which selects HXTAL(40M) as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_160m_26m_hxtal(void)
{
#if 0
    uint32_t stab_flag = 0U;
    uint32_t timeout = 0U;
#endif
    /* power up HXTAL */
    RCU_CTL |= RCU_CTL_HXTALPU;
    /* enable HXTAL */
    RCU_CTL |= RCU_CTL_HXTALEN;

    /*
     * HW team said that waiting HXTAL stable is not necessary to wait.
     * But it's best to wait it for safety.
     * We hope to setup HSE as fastly for LPDS, we mark it do more test.
     */
#if 0
    HXTALSTB_DELAY

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    do{
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    }while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

    /* if fail */
    if(0U == (RCU_CTL & RCU_CTL_HXTALSTB)){
        while(1){
        }
    }
#endif

    RCU_CTL |= RCU_CTL_HXTALREADY;

    rcu_periph_clock_enable(RCU_RF);

    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    RCU_PLL |= RCU_PLLSRC_HXTAL;

    /* PLLDIG = 960/HSE */
    RCU_PLLDIGCFG1 = ((960 << 21) / (HXTAL_VALUE/1000000)) & 0x7FFFFFFF;
    /* PLLDIG OUT = 480M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_480M );
    /* SYS clock = 160M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_SYS_DIV3 );

    REG32(0x40017814) |= 0x4000000;
    REG32(0x400178D8) |= 0x6;

    REG32(0x400178D0) &= (~0xF000);
    REG32(0x400178D0) |= 0x7000;

    /* enable PLL */
    RCU_CFG1 |= (RCU_CFG1_RFPLLCALEN | RCU_CFG1_BGPU);
    RCU_CTL |= (RCU_CTL_PLLDIGEN | RCU_CTL_PLLDIGPU);
    /* wait until PLL is stable */
    while(0U == (RCU_CTL & RCU_CTL_PLLDIGSTB)){
    }
    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLDIG;

    /* wait until PLL is selected as system clock */
    while(RCU_SCSS_PLLDIG != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}

#elif defined (__SYSTEM_CLOCK_160M_PLLDIG_48M_HXTAL)
/*!
    \brief      configure the system clock to 120M by PLL which selects HXTAL(40M) as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_160m_48m_hxtal(void)
{
#if 0
    uint32_t stab_flag = 0U;
    uint32_t timeout = 0U;
#endif
    /* power up HXTAL */
    RCU_CTL |= RCU_CTL_HXTALPU;
    /* enable HXTAL */
    RCU_CTL |= RCU_CTL_HXTALEN;

    /*
     * HW team said that waiting HXTAL stable is not necessary to wait.
     * But it's best to wait it for safety.
     * We hope to setup HSE as fastly for LPDS, we mark it do more test.
     */
#if 0
    HXTALSTB_DELAY

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    do{
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    }while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

    /* if fail */
    if(0U == (RCU_CTL & RCU_CTL_HXTALSTB)){
        while(1){
        }
    }
#endif

    RCU_CTL |= RCU_CTL_HXTALREADY;

    rcu_periph_clock_enable(RCU_RF);

    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    RCU_PLL |= RCU_PLLSRC_HXTAL;

    /* PLLDIG = 960/HSE */
    RCU_PLLDIGCFG1 = ((960 << 21) / (HXTAL_VALUE/1000000)) & 0x7FFFFFFF;
    /* PLLDIG OUT = 480M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_480M );
    /* SYS clock = 160M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_SYS_DIV3 );

    REG32(0x400178D0) &= (~0xF000);
    REG32(0x400178D0) |= 0xC000;

    /* enable PLL */
    RCU_CFG1 |= (RCU_CFG1_RFPLLCALEN | RCU_CFG1_BGPU);
    RCU_CTL |= (RCU_CTL_PLLDIGEN | RCU_CTL_PLLDIGPU);
    /* wait until PLL is stable */
    while(0U == (RCU_CTL & RCU_CTL_PLLDIGSTB)){
    }
    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLDIG;

    /* wait until PLL is selected as system clock */
    while(RCU_SCSS_PLLDIG != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}

#elif defined (__SYSTEM_CLOCK_40M_PLLDIG_40M_HXTAL)
/*!
    \brief      configure the system clock to 120M by PLL which selects HXTAL(40M) as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_40m_40m_hxtal(void)
{
#if 0
    uint32_t stab_flag = 0U;
    uint32_t timeout = 0U;
#endif
    /* power up HXTAL */
    RCU_CTL |= RCU_CTL_HXTALPU;
    /* enable HXTAL */
    RCU_CTL |= RCU_CTL_HXTALEN;

    /*
     * HW team said that waiting HXTAL stable is not necessary to wait.
     * But it's best to wait it for safety.
     * We hope to setup HSE as fastly for LPDS, we mark it do more test.
     */
#if 0
    HXTALSTB_DELAY

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    do{
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    }while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

    /* if fail */
    if(0U == (RCU_CTL & RCU_CTL_HXTALSTB)){
        while(1){
        }
    }
#endif

    RCU_CTL |= RCU_CTL_HXTALREADY;

    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    RCU_PLL |= RCU_PLLSRC_HXTAL;

    /* PLLDIG = 960/HSE */
    RCU_PLLDIGCFG1 = ((960 << 21) / (HXTAL_VALUE/1000000)) & 0x7FFFFFFF;

    /* PLLDIG OUT = 480M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_480M );
    /* SYS clock = 40M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_SYS_DIV12 );

    /* enable PLL */
    RCU_CFG1 |= (RCU_CFG1_RFPLLCALEN | RCU_CFG1_BGPU);
    RCU_CTL |= (RCU_CTL_PLLDIGEN | RCU_CTL_PLLDIGPU);
    /* wait until PLL is stable */
    while(0U == (RCU_CTL & RCU_CTL_PLLDIGSTB)){
    }
    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLDIG;

    /* wait until PLL is selected as system clock */
    while(RCU_SCSS_PLLDIG != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}
#elif defined (__SYSTEM_CLOCK_50M_PLLDIG)
/*!
    \brief      configure the system clock to 120M by PLL which selects HXTAL(40M) as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_50m_plldig(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    // REG32(0x400238a0) = REG32(0x400238a0) | BIT(30) | BIT(29);
    /* power up HXTAL */
    RCU_CTL |= RCU_CTL_HXTALPU;
    /* enable HXTAL */
    RCU_CTL |= RCU_CTL_HXTALEN;
    HXTALSTB_DELAY
    RCU_CTL |= RCU_CTL_HXTALREADY;

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    do{
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    }while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

    /* if fail */
    if(0U == (RCU_CTL & RCU_CTL_HXTALSTB)){
        while(1){
        }
    }

    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    RCU_PLL |= RCU_PLLSRC_HXTAL;

    /* PLLDIG = 960/HSE */
    RCU_PLLDIGCFG1 = ((960 << 21) / (HXTAL_VALUE/1000000)) & 0x7FFFFFFF;

    /* PLLDIG OUT = 480M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_480M );
    /* SYS clock = 160M */
    RCU_PLLDIGCFG0 |= ( RCU_PLLDIG_SYS_DIV3 );

    /* enable PLL */
    // RCU_CFG1 |= (RCU_CFG1_RFPLLCALEN | RCU_CFG1_RFPLLPU | RCU_CFG1_BGPU);
    RCU_CFG1 |= (RCU_CFG1_BGPU);
    RCU_CTL |= (RCU_CTL_PLLDIGEN | RCU_CTL_PLLDIGPU);
    /* wait until PLL is stable */
     while(0U == (RCU_CTL & RCU_CTL_PLLDIGSTB)){
     }
    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLDIG;

    /* wait until PLL is selected as system clock */
    while(RCU_SCSS_PLLDIG != (RCU_CFG0 & RCU_CFG0_SCSS)){
    }
}
#endif /* __SYSTEM_CLOCK_IRC16M */

/*!
    \brief      update the SystemCoreClock with current core clock retrieved from cpu registers
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SystemCoreClockUpdate (void)
{
    uint32_t sws;
    uint32_t fresel, plldigdiv, outputfre, idx, clk_exp;

    /* exponent of AHB, APB1 and APB2 clock divider */
    const uint8_t ahb_exp[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};

    sws = GET_BITS(RCU_CFG0, 2, 3);
    switch(sws){
    /* IRC16M is selected as CK_SYS */
    case 0:
        SystemCoreClock = IRC16M_VALUE;
        break;
    /* HXTAL is selected as CK_SYS */
    case 1:
        SystemCoreClock = HXTAL_VALUE;
        break;
    /* PLLDIG is selected as CK_SYS */
    case 2:
        /* get the value of PLLDIGOSEL[1:0] */
        fresel = GET_BITS(RCU_PLLDIGCFG0, 24U, 25U);
        switch(fresel){
        case 0:
            outputfre = 192000000;
            break;
        case 1:
            outputfre = 240000000;
            break;
        case 2:
            outputfre = 320000000;
            break;
        case 3:
            outputfre = 480000000;
            break;
        default:
            outputfre = 192000000;
            break;
        }
        plldigdiv = (GET_BITS(RCU_PLLDIGCFG0, 26U, 31U) + 1U);
        SystemCoreClock = outputfre/plldigdiv;
        // SystemCoreClock = 48000000;//for FPGA
        break;
    /* IRC16M is selected as CK_SYS */
    default:
        SystemCoreClock = IRC16M_VALUE;
        break;
    }
    /* calculate AHB clock frequency */
    idx = GET_BITS(RCU_CFG0, 4, 7);
    clk_exp = ahb_exp[idx];
    SystemCoreClock = SystemCoreClock >> clk_exp;
}
