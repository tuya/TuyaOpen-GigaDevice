/*!
    \file    models.h
    \brief   Header file for BLE mesh models.

    \version 2024-09-09, V1.0.2, firmware for GD32VW55x
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

#ifndef MODELS_H_
#define MODELS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define OP_DUMMY_2_BYTE                             BT_MESH_MODEL_OP_2(0xff, 0xff)

#define OP_GEN_ONOFF_GET                            BT_MESH_MODEL_OP_2(0x82, 0x01)
#define OP_GEN_ONOFF_SET                            BT_MESH_MODEL_OP_2(0x82, 0x02)
#define OP_GEN_ONOFF_SET_UNACK                      BT_MESH_MODEL_OP_2(0x82, 0x03)
#define OP_GEN_ONOFF_STATUS                         BT_MESH_MODEL_OP_2(0x82, 0x04)

#define OP_GEN_LEVEL_GET                            BT_MESH_MODEL_OP_2(0x82, 0x05)
#define OP_GEN_LEVEL_SET                            BT_MESH_MODEL_OP_2(0x82, 0x06)
#define OP_GEN_LEVEL_SET_UNACK                      BT_MESH_MODEL_OP_2(0x82, 0x07)
#define OP_GEN_LEVEL_STATUS                         BT_MESH_MODEL_OP_2(0x82, 0x08)
#define OP_GEN_LEVEL_DELTA_SET                      BT_MESH_MODEL_OP_2(0x82, 0x09)
#define OP_GEN_LEVEL_DELTA_SET_UNACK                BT_MESH_MODEL_OP_2(0x82, 0x0A)
#define OP_GEN_LEVEL_MOVE_SET                       BT_MESH_MODEL_OP_2(0x82, 0x0B)
#define OP_GEN_LEVEL_MOVE_SET_UNACK                 BT_MESH_MODEL_OP_2(0x82, 0x0C)

#define OP_GEN_DEF_TRANS_TIME_GET                   BT_MESH_MODEL_OP_2(0x82, 0x0D)
#define OP_GEN_DEF_TRANS_TIME_SET                   BT_MESH_MODEL_OP_2(0x82, 0x0E)
#define OP_GEN_DEF_TRANS_TIME_SET_UNACK             BT_MESH_MODEL_OP_2(0x82, 0x0F)
#define OP_GEN_DEF_TRANS_TIME_STATUS                BT_MESH_MODEL_OP_2(0x82, 0x10)

#define OP_GEN_ONPOWERUP_GET                        BT_MESH_MODEL_OP_2(0x82, 0x11)
#define OP_GEN_ONPOWERUP_STATUS                     BT_MESH_MODEL_OP_2(0x82, 0x12)
#define OP_GEN_ONPOWERUP_SET                        BT_MESH_MODEL_OP_2(0x82, 0x13)
#define OP_GEN_ONPOWERUP_SET_UNACK                  BT_MESH_MODEL_OP_2(0x82, 0x14)

#define OP_GEN_POWER_LEVEL_GET                      BT_MESH_MODEL_OP_2(0x82, 0x15)
#define OP_GEN_POWER_LEVEL_SET                      BT_MESH_MODEL_OP_2(0x82, 0x16)
#define OP_GEN_POWER_LEVEL_SET_UNACK                BT_MESH_MODEL_OP_2(0x82, 0x17)
#define OP_GEN_POWER_LEVEL_STATUS                   BT_MESH_MODEL_OP_2(0x82, 0x18)
#define OP_GEN_POWER_LAST_GET                       BT_MESH_MODEL_OP_2(0x82, 0x19)
#define OP_GEN_POWER_LAST_STATUS                    BT_MESH_MODEL_OP_2(0x82, 0x1A)
#define OP_GEN_POWER_DEFAULT_GET                    BT_MESH_MODEL_OP_2(0x82, 0x1B)
#define OP_GEN_POWER_DEFAULT_STATUS                 BT_MESH_MODEL_OP_2(0x82, 0x1C)
#define OP_GEN_POWER_RANGE_GET                      BT_MESH_MODEL_OP_2(0x82, 0x1D)
#define OP_GEN_POWER_RANGE_STATUS                   BT_MESH_MODEL_OP_2(0x82, 0x1E)
#define OP_GEN_POWER_DEFAULT_SET                    BT_MESH_MODEL_OP_2(0x82, 0x1F)
#define OP_GEN_POWER_DEFAULT_SET_UNACK              BT_MESH_MODEL_OP_2(0x82, 0x20)
#define OP_GEN_POWER_RANGE_SET                      BT_MESH_MODEL_OP_2(0x82, 0x21)
#define OP_GEN_POWER_RANGE_SET_UNACK                BT_MESH_MODEL_OP_2(0x82, 0x22)

#define OP_GEN_BATTERY_GET                          BT_MESH_MODEL_OP_2(0x82, 0x23)
#define OP_GEN_BATTERY_STATUS                       BT_MESH_MODEL_OP_2(0x82, 0x24)

#define OP_GEN_LOCATION_GLOBAL_GET                  BT_MESH_MODEL_OP_2(0x82, 0x25)
#define OP_GEN_LOCATION_GLOBAL_STATUS               BT_MESH_MODEL_OP_1(0x40)
#define OP_GEN_LOCATION_GLOBAL_SET                  BT_MESH_MODEL_OP_1(0x41)
#define OP_GEN_LOCATION_GLOBAL_SET_UNACK            BT_MESH_MODEL_OP_1(0x42)
#define OP_GEN_LOCATION_LOCAL_GET                   BT_MESH_MODEL_OP_2(0x82, 0x26)
#define OP_GEN_LOCATION_LOCAL_STATUS                BT_MESH_MODEL_OP_2(0x82, 0x27)
#define OP_GEN_LOCATION_LOCAL_SET                   BT_MESH_MODEL_OP_2(0x82, 0x28)
#define OP_GEN_LOCATION_LOCAL_SET_UNACK             BT_MESH_MODEL_OP_2(0x82, 0x29)

#define OP_GEN_MFR_PROPS_GET                        BT_MESH_MODEL_OP_2(0x82, 0x2A)
#define OP_GEN_MFR_PROP_GET                         BT_MESH_MODEL_OP_2(0x82, 0x2B)
#define OP_GEN_MFR_PROPS_STATUS                     BT_MESH_MODEL_OP_1(0x43)
#define OP_GEN_MFR_PROP_SET                         BT_MESH_MODEL_OP_1(0x44)
#define OP_GEN_MFR_PROP_SET_UNACK                   BT_MESH_MODEL_OP_1(0x45)
#define OP_GEN_MFR_PROP_STATUS                      BT_MESH_MODEL_OP_1(0x46)
#define OP_GEN_ADMIN_PROPS_GET                      BT_MESH_MODEL_OP_2(0x82, 0x2C)
#define OP_GEN_ADMIN_PROP_GET                       BT_MESH_MODEL_OP_2(0x82, 0x2D)
#define OP_GEN_ADMIN_PROPS_STATUS                   BT_MESH_MODEL_OP_1(0x47)
#define OP_GEN_ADMIN_PROP_SET                       BT_MESH_MODEL_OP_1(0x48)
#define OP_GEN_ADMIN_PROP_SET_UNACK                 BT_MESH_MODEL_OP_1(0x49)
#define OP_GEN_ADMIN_PROP_STATUS                    BT_MESH_MODEL_OP_1(0x4A)
#define OP_GEN_USER_PROPS_GET                       BT_MESH_MODEL_OP_2(0x82, 0x2E)
#define OP_GEN_USER_PROP_GET                        BT_MESH_MODEL_OP_2(0x82, 0x2F)
#define OP_GEN_USER_PROPS_STATUS                    BT_MESH_MODEL_OP_1(0x4B)
#define OP_GEN_USER_PROP_SET                        BT_MESH_MODEL_OP_1(0x4C)
#define OP_GEN_USER_PROP_SET_UNACK                  BT_MESH_MODEL_OP_1(0x4D)
#define OP_GEN_USER_PROP_STATUS                     BT_MESH_MODEL_OP_1(0x4E)
#define OP_GEN_CLIENT_PROPS_GET                     BT_MESH_MODEL_OP_1(0x4F)
#define OP_GEN_CLIENT_PROPS_STATUS                  BT_MESH_MODEL_OP_1(0x50)

#define OP_LIGHT_LIGHTNESS_GET                      BT_MESH_MODEL_OP_2(0x82, 0x4B)
#define OP_LIGHT_LIGHTNESS_SET                      BT_MESH_MODEL_OP_2(0x82, 0x4C)
#define OP_LIGHT_LIGHTNESS_SET_UNACK                BT_MESH_MODEL_OP_2(0x82, 0x4D)
#define OP_LIGHT_LIGHTNESS_STATUS                   BT_MESH_MODEL_OP_2(0x82, 0x4E)
#define OP_LIGHT_LIGHTNESS_LINEAR_GET               BT_MESH_MODEL_OP_2(0x82, 0x4F)
#define OP_LIGHT_LIGHTNESS_LINEAR_SET               BT_MESH_MODEL_OP_2(0x82, 0x50)
#define OP_LIGHT_LIGHTNESS_LINEAR_SET_UNACK         BT_MESH_MODEL_OP_2(0x82, 0x51)
#define OP_LIGHT_LIGHTNESS_LINEAR_STATUS            BT_MESH_MODEL_OP_2(0x82, 0x52)
#define OP_LIGHT_LIGHTNESS_LAST_GET                 BT_MESH_MODEL_OP_2(0x82, 0x53)
#define OP_LIGHT_LIGHTNESS_LAST_STATUS              BT_MESH_MODEL_OP_2(0x82, 0x54)
#define OP_LIGHT_LIGHTNESS_DEFAULT_GET              BT_MESH_MODEL_OP_2(0x82, 0x55)
#define OP_LIGHT_LIGHTNESS_DEFAULT_STATUS           BT_MESH_MODEL_OP_2(0x82, 0x56)
#define OP_LIGHT_LIGHTNESS_RANGE_GET                BT_MESH_MODEL_OP_2(0x82, 0x57)
#define OP_LIGHT_LIGHTNESS_RANGE_STATUS             BT_MESH_MODEL_OP_2(0x82, 0x58)
#define OP_LIGHT_LIGHTNESS_DEFAULT_SET              BT_MESH_MODEL_OP_2(0x82, 0x59)
#define OP_LIGHT_LIGHTNESS_DEFAULT_SET_UNACK        BT_MESH_MODEL_OP_2(0x82, 0x5A)
#define OP_LIGHT_LIGHTNESS_RANGE_SET                BT_MESH_MODEL_OP_2(0x82, 0x5B)
#define OP_LIGHT_LIGHTNESS_RANGE_SET_UNACK          BT_MESH_MODEL_OP_2(0x82, 0x5C)

#define OP_LIGHT_CTL_GET                            BT_MESH_MODEL_OP_2(0x82, 0x5D)
#define OP_LIGHT_CTL_SET                            BT_MESH_MODEL_OP_2(0x82, 0x5E)
#define OP_LIGHT_CTL_SET_UNACK                      BT_MESH_MODEL_OP_2(0x82, 0x5F)
#define OP_LIGHT_CTL_STATUS                         BT_MESH_MODEL_OP_2(0x82, 0x60)
#define OP_LIGHT_CTL_TEMPERATURE_GET                BT_MESH_MODEL_OP_2(0x82, 0x61)
#define OP_LIGHT_CTL_TEMPERATURE_RANGE_GET          BT_MESH_MODEL_OP_2(0x82, 0x62)
#define OP_LIGHT_CTL_TEMPERATURE_RANGE_STATUS       BT_MESH_MODEL_OP_2(0x82, 0x63)
#define OP_LIGHT_CTL_TEMPERATURE_SET                BT_MESH_MODEL_OP_2(0x82, 0x64)
#define OP_LIGHT_CTL_TEMPERATURE_SET_UNACK          BT_MESH_MODEL_OP_2(0x82, 0x65)
#define OP_LIGHT_CTL_TEMPERATURE_STATUS             BT_MESH_MODEL_OP_2(0x82, 0x66)
#define OP_LIGHT_CTL_DEFAULT_GET                    BT_MESH_MODEL_OP_2(0x82, 0x67)
#define OP_LIGHT_CTL_DEFAULT_STATUS                 BT_MESH_MODEL_OP_2(0x82, 0x68)
#define OP_LIGHT_CTL_DEFAULT_SET                    BT_MESH_MODEL_OP_2(0x82, 0x69)
#define OP_LIGHT_CTL_DEFAULT_SET_UNACK              BT_MESH_MODEL_OP_2(0x82, 0x6A)
#define OP_LIGHT_CTL_TEMPERATURE_RANGE_SET          BT_MESH_MODEL_OP_2(0x82, 0x6B)
#define OP_LIGHT_CTL_TEMPERATURE_RANGE_SET_UNACK    BT_MESH_MODEL_OP_2(0x82, 0x6C)

#define OP_LIGHT_HSL_GET                            BT_MESH_MODEL_OP_2(0x82, 0x6D)
#define OP_LIGHT_HSL_HUE_GET                        BT_MESH_MODEL_OP_2(0x82, 0x6E)
#define OP_LIGHT_HSL_HUE_SET                        BT_MESH_MODEL_OP_2(0x82, 0x6F)
#define OP_LIGHT_HSL_HUE_SET_UNACK                  BT_MESH_MODEL_OP_2(0x82, 0x70)
#define OP_LIGHT_HSL_HUE_STATUS                     BT_MESH_MODEL_OP_2(0x82, 0x71)
#define OP_LIGHT_HSL_SATURATION_GET                 BT_MESH_MODEL_OP_2(0x82, 0x72)
#define OP_LIGHT_HSL_SATURATION_SET                 BT_MESH_MODEL_OP_2(0x82, 0x73)
#define OP_LIGHT_HSL_SATURATION_SET_UNACK           BT_MESH_MODEL_OP_2(0x82, 0x74)
#define OP_LIGHT_HSL_SATURATION_STATUS              BT_MESH_MODEL_OP_2(0x82, 0x75)
#define OP_LIGHT_HSL_SET                            BT_MESH_MODEL_OP_2(0x82, 0x76)
#define OP_LIGHT_HSL_SET_UNACK                      BT_MESH_MODEL_OP_2(0x82, 0x77)
#define OP_LIGHT_HSL_STATUS                         BT_MESH_MODEL_OP_2(0x82, 0x78)
#define OP_LIGHT_HSL_TARGET_GET                     BT_MESH_MODEL_OP_2(0x82, 0x79)
#define OP_LIGHT_HSL_TARGET_STATUS                  BT_MESH_MODEL_OP_2(0x82, 0x7A)
#define OP_LIGHT_HSL_DEFAULT_GET                    BT_MESH_MODEL_OP_2(0x82, 0x7B)
#define OP_LIGHT_HSL_DEFAULT_STATUS                 BT_MESH_MODEL_OP_2(0x82, 0x7C)
#define OP_LIGHT_HSL_RANGE_GET                      BT_MESH_MODEL_OP_2(0x82, 0x7D)
#define OP_LIGHT_HSL_RANGE_STATUS                   BT_MESH_MODEL_OP_2(0x82, 0x7E)
#define OP_LIGHT_HSL_DEFAULT_SET                    BT_MESH_MODEL_OP_2(0x82, 0x7F)
#define OP_LIGHT_HSL_DEFAULT_SET_UNACK              BT_MESH_MODEL_OP_2(0x82, 0x80)
#define OP_LIGHT_HSL_RANGE_SET                      BT_MESH_MODEL_OP_2(0x82, 0x81)
#define OP_LIGHT_HSL_RANGE_SET_UNACK                BT_MESH_MODEL_OP_2(0x82, 0x82)

#define OP_LIGHT_XYL_GET                            BT_MESH_MODEL_OP_2(0x82, 0x83)
#define OP_LIGHT_XYL_SET                            BT_MESH_MODEL_OP_2(0x82, 0x84)
#define OP_LIGHT_XYL_SET_UNACK                      BT_MESH_MODEL_OP_2(0x82, 0x85)
#define OP_LIGHT_XYL_STATUS                         BT_MESH_MODEL_OP_2(0x82, 0x86)
#define OP_LIGHT_XYL_TARGET_GET                     BT_MESH_MODEL_OP_2(0x82, 0x87)
#define OP_LIGHT_XYL_TARGET_STATUS                  BT_MESH_MODEL_OP_2(0x82, 0x88)
#define OP_LIGHT_XYL_DEFAULT_GET                    BT_MESH_MODEL_OP_2(0x82, 0x89)
#define OP_LIGHT_XYL_DEFAULT_STATUS                 BT_MESH_MODEL_OP_2(0x82, 0x8A)
#define OP_LIGHT_XYL_RANGE_GET                      BT_MESH_MODEL_OP_2(0x82, 0x8B)
#define OP_LIGHT_XYL_RANGE_STATUS                   BT_MESH_MODEL_OP_2(0x82, 0x8C)
#define OP_LIGHT_XYL_DEFAULT_SET                    BT_MESH_MODEL_OP_2(0x82, 0x8D)
#define OP_LIGHT_XYL_DEFAULT_SET_UNACK              BT_MESH_MODEL_OP_2(0x82, 0x8E)
#define OP_LIGHT_XYL_RANGE_SET                      BT_MESH_MODEL_OP_2(0x82, 0x8F)
#define OP_LIGHT_XYL_RANGE_SET_UNACK                BT_MESH_MODEL_OP_2(0x82, 0x90)

#ifdef __cplusplus
}
#endif

#endif /* MODELS_H_ */
