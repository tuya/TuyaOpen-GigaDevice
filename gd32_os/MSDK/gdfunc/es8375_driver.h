
#ifndef _ES8375_DRIVER_H
#define _ES8375_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gd32vw55x.h"

// Registors
#define ES8375_RESET1_0x00              0x00
#define ES8375_MCLK_SEL_0x01            0x01
#define ES8375_CLK_MGR_0x02             0x02
#define ES8375_CLK_MGR_0x03             0x03
#define ES8375_CLK_MGR_0x04             0x04
#define ES8375_CLK_MGR_0x05             0x05
#define ES8375_CLK_MGR_0x06             0x06
#define ES8375_CLK_MGR_0x07             0x07
#define ES8375_CLK_MGR_0x08             0x08
#define ES8375_CLK_MGR_0x09             0x09
#define ES8375_CLK_MGR_0x0A             0x0A
#define ES8375_CLK_MGR_0x0B             0x0B
#define ES8375_CLK_MGR_0x0C             0x0C
#define ES8375_DIV_SPKCLK_0x0E          0x0E
#define ES8375_CSM1_0x0F                0x0F
#define ES8375_CSM2_0x10                0x10
#define ES8375_VMID_CHARGE2_T_0x11      0x11
#define ES8375_VMID_CHARGE3_T_0x12      0x12
#define ES8375_SDP_0x15                 0x15
#define ES8375_SDP2_0x16                0x16
#define ES8375_ADC1_0x17                0x17
#define ES8375_ADC2_0x18                0x18
#define ES8375_ADC_OSR_GAIN_0x19        0x19
#define ES8375_ADC_VOLUME_0x1A          0x1A
#define ES8375_ADC_AUTOMUTE_0x1B        0x1B
#define ES8375_ADC_AUTOMUTE_ATTN_0x1C   0x1C
#define ES8375_HPF1_0x1D                0x1D
#define ES8375_DAC1_0x1F                0x1F
#define ES8375_DAC2_0x20                0x20
#define ES8375_DAC_VOLUME_0x21          0x21
#define ES8375_DAC_VPPSCALE_0x22        0x22
#define ES8375_DAC_AUTOMUTE1_0x23       0x23
#define ES8375_DAC_AUTOMUTE_0x24        0x24
#define ES8375_DAC_CAL_0x25             0x25
#define ES8375_DAC_OTP_0x27             0x27
#define ES8375_ANALOG_SPK1_0x28         0x28
#define ES8375_ANALOG_SPK2_0x29         0x29
#define ES8375_ANALOG_SPKVOL_0x2B       0x2B
#define ES8375_VMID_SEL_0x2D            0x2D
#define ES8375_ANALOG_0x2E              0x2E
#define ES8375_ANALOG_0x32              0x32
#define ES8375_ANALOG_0x37              0x37
#define ES8375_ADC2DAC_CLKTRI           0xF8
#define ES8375_SYS_CTRL2_0xF9           0xF9
#define ES8375_FLAGS2_0xFB              0xFB
#define ES8375_SPK_OFFSET_0xFC          0xFC
#define ES8375_CHIP_ID1_0xFD            0xFD
#define ES8375_CHIP_ID0_0xFE            0xFE
#define ES8375_CHIP_VERSION_0xFF        0xFF

// Bit Shifts
#define ADC_OSR_GAIN_SHIFT_0        0
#define ADC_RAMPRATE_SHIFT_0        0
#define ADC_VOLUME_SHIFT_0          0
#define ADC_AUTOMUTE_NG_SHIFT_0     0
#define ADC_AUTOMUTE_ATTN_SHIFT_0   0
#define DAC_RAMPRATE_SHIFT_0        0
#define DAC_VOLUME_SHIFT_0          0
#define DAC_VPPSCALE_SHIFT_0        0
#define DAC_AUTOMUTE_NG_SHIFT_0     0
#define DAC_AUTOMUTE_ATTN_SHIFT_0   0
#define DMIC_GAIN_SHIFT_2           2
#define ADC_AUTOMUTE_WS_SHIFT_3     3
#define DMIC_POL_SHIFT_4            4
#define DAC_RAMCLR_SHIFT_4          4
#define ES8375_EN_MODL_SHIFT_4      4
#define ADC_RAMCLR_SHIFT_5          5
#define ADC_HPF_SHIFT_5             5
#define DAC_INV_SHIFT_5             5
#define DAC_AUTOMUTE_WS_SHIFT_5     5
#define ES8375_EN_PGAL_SHIFT_5      5
#define ES8375_ADC_P2S_MUTE_SHIFT_5 5
#define ADC_INV_SHIFT_6             6
#define DAC_DEMMUTE_SHIFT_6         6
#define ES8375_DAC_S2P_MUTE_SHIFT_6 6
#define ADC_SPKVOL_SHIFT_6          6
#define ADC_SRC_SHIFT_7             7
#define ADC_AUTOMUTE_SHIFT_7        7
#define DAC_DSMMUTE_SHIFT_7         7
#define DAC_AUTOMUTE_EN_SHIFT_7     7


// Function values
#define ES8375_ADC_OSR_GAIN_MAX         0x3F
#define ES8375_ADC_AUTOMUTE_ATTN_MAX    0x1F
#define ES8375_ADC_VOLUME_MAX           0xFF
#define ES8375_DAC_VOLUME_MAX           0xFF
#define ES8375_DAC_VPPSCALE_MAX         0x3F
#define ES8375_DAC_AUTOMUTE_ATTN_MAX    0x17   // 0 ~ 23: gain step is 4dB per level, 24~31 are reserved, valid gain range is up to 0x17 (23)
#define ES8375_REG_MAX                  0xFF

// Properties
#define ES8375_3V3  1
#define ES8375_1V8  0
#define ES8375_AVDD ES8375_3V3
#define ES8375_DVDD ES8375_3V3

#define ES8375_MCLK_PIN	0
#define ES8375_BCLK_PIN 1
#define ES8375_MCLK_SOURCE	ES8375_MCLK_PIN

#define DMIC_POSITIVE_EDGE  0  // Low = Left channel ; High = Right channel
#define DMIC_NEGATIVE_EDGE  1  // Low = Right channel ; High = Left channel
#define DMIC_POL  DMIC_POSITIVE_EDGE

#define PA_SHUTDOWN     0
#define PA_ENABLE       1


typedef struct _coeff_div {
	uint16_t mclk_lrck_ratio;
	uint32_t mclk;
	uint32_t rate;
	uint8_t	Reg0x04;	// Adjustable based on actual configuration
	uint8_t	Reg0x05;	// Adjustable based on actual configuration
	uint8_t	Reg0x06;	// Adjustable based on actual configuration
	uint8_t	Reg0x07;	// Adjustable based on actual configuration
	uint8_t	Reg0x08;	// Adjustable based on actual configuration
	uint8_t	Reg0x09;	// Adjustable based on actual configuration
	uint8_t	Reg0x0A;	// Adjustable based on actual configuration
	uint8_t	Reg0x0B;	// Adjustable based on actual configuration
	uint8_t	Reg0x19;	// Adjustable based on actual configuration
	uint8_t	dvdd_vol;	// Fixed template: 2: 1V8&3V3, 1: 3V3, 0: 1V8
	uint8_t	dmic_sel;	// Fixed template: 2: AMIC&DMIC, 1: DMIC, 0: AMIC
} _coeff_div;

void es8375_init(void);
bool es8375_volume_set(uint8_t vol_idx);

bool es8375_volume_get(uint8_t *vol_idx);

bool es8375_vppcale_set(uint8_t vpp_idx);
#ifdef __cplusplus
}
#endif

#endif // _ES8375_DRIVER_H