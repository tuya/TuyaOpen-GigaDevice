/*!
    \file    device_properties.h
    \brief   Header file for BLE mesh device properties.

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

#ifndef DEVICE_PROPERTIES_H_
#define DEVICE_PROPERTIES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Prohibited. */
#define MESH_PROPERTY_ID_PROHIBITED 0x0000
/* Average ambient temperature in a period of day. */
#define MESH_PROPERTY_ID_AVG_AMB_TEMPERATURE_IN_A_PERIOD_OF_DAY 0x0001
/* Average input current. */
#define MESH_PROPERTY_ID_AVG_INPUT_CURRENT 0x0002
/* Average input voltage. */
#define MESH_PROPERTY_ID_AVG_INPUT_VOLTAGE 0x0003
/* Average output current. */
#define MESH_PROPERTY_ID_AVG_OUTPUT_CURRENT 0x0004
/* Average output voltage. */
#define MESH_PROPERTY_ID_AVG_OUTPUT_VOLTAGE 0x0005
/* Center beam intensity at full power. */
#define MESH_PROPERTY_ID_CENTER_BEAM_INTENSITY_AT_FULL_POWER 0x0006
/* Chromaticity tolerance. */
#define MESH_PROPERTY_ID_CHROMATICITY_TOLERANCE 0x0007
/* Color rendering index R9. */
#define MESH_PROPERTY_ID_COL_RENDERING_INDEX_R9 0x0008
/* Color rendering index RA. */
#define MESH_PROPERTY_ID_COL_RENDERING_INDEX_RA 0x0009
/* Device appearance. */
#define MESH_PROPERTY_ID_DEV_APPEARANCE 0x000A
/* Device country of origin. */
#define MESH_PROPERTY_ID_DEV_COUNTRY_OF_ORIGIN 0x000B
/* Device date of manufacture. */
#define MESH_PROPERTY_ID_DEV_DATE_OF_MANUFACTURE 0x000C
/* Device energy use since turn on. */
#define MESH_PROPERTY_ID_DEV_ENERGY_USE_SINCE_TURN_ON 0x000D
/* Device firmware revision. */
#define MESH_PROPERTY_ID_DEV_FW_REVISION 0x000E
/* Device global trade item number. */
#define MESH_PROPERTY_ID_DEV_GLOBAL_TRADE_ITEM_NUM 0x000F
/* Device hardware revision. */
#define MESH_PROPERTY_ID_DEV_HW_REVISION 0x0010
/* Device manufacturer name. */
#define MESH_PROPERTY_ID_DEV_MFR_NAME 0x0011
/* Device model number. */
#define MESH_PROPERTY_ID_DEV_MODEL_NUM 0x0012
/* Device operating temperature range specification. */
#define MESH_PROPERTY_ID_DEV_OP_TEMPERATURE_RANGE_SPEC 0x0013
/* Device operating temperature statistical values. */
#define MESH_PROPERTY_ID_DEV_OP_TEMPERATURE_STAT_VALUES 0x0014
/* Device over temperature event statistics. */
#define MESH_PROPERTY_ID_DEV_OVER_TEMPERATURE_EVT_STAT 0x0015
/* Device power range specification. */
#define MESH_PROPERTY_ID_DEV_POWER_RANGE_SPEC 0x0016
/* Device runtime since turn on. */
#define MESH_PROPERTY_ID_DEV_RUNTIME_SINCE_TURN_ON 0x0017
/* Device runtime warranty. */
#define MESH_PROPERTY_ID_DEV_RUNTIME_WARRANTY 0x0018
/* Device serial number. */
#define MESH_PROPERTY_ID_DEV_SERIAL_NUM 0x0019
/* Device software revision. */
#define MESH_PROPERTY_ID_DEV_SW_REVISION 0x001A
/* Device under temperature event statistics. */
#define MESH_PROPERTY_ID_DEV_UNDER_TEMPERATURE_EVT_STAT 0x001B
/* Indoor ambient temperature statistical values. */
#define MESH_PROPERTY_ID_INDOOR_AMB_TEMPERATURE_STAT_VALUES 0x001C
/* Initial CIE-1931 chromaticity coordinates. */
#define MESH_PROPERTY_ID_INITIAL_CIE_1931_CHROMATICITY_COORDS 0x001D
/* Initial correlated color temperature. */
#define MESH_PROPERTY_ID_INITIAL_CORRELATED_COL_TEMPERATURE 0x001E
/* Initial luminous flux. */
#define MESH_PROPERTY_ID_INITIAL_LUMINOUS_FLUX 0x001F
/* Initial planckian distance. */
#define MESH_PROPERTY_ID_INITIAL_PLANCKIAN_DISTANCE 0x0020
/* Input current range specification. */
#define MESH_PROPERTY_ID_INPUT_CURRENT_RANGE_SPEC 0x0021
/* Input current statistics. */
#define MESH_PROPERTY_ID_INPUT_CURRENT_STAT 0x0022
/* Input over current event statistics. */
#define MESH_PROPERTY_ID_INPUT_OVER_CURRENT_EVT_STAT 0x0023
/* Input over ripple voltage event statistics. */
#define MESH_PROPERTY_ID_INPUT_OVER_RIPPLE_VOLTAGE_EVT_STAT 0x0024
/* Input over voltage event statistics. */
#define MESH_PROPERTY_ID_INPUT_OVER_VOLTAGE_EVT_STAT 0x0025
/* Input under current event statistics. */
#define MESH_PROPERTY_ID_INPUT_UNDER_CURRENT_EVT_STAT 0x0026
/* Input under voltage event statistics. */
#define MESH_PROPERTY_ID_INPUT_UNDER_VOLTAGE_EVT_STAT 0x0027
/* Input voltage range specification. */
#define MESH_PROPERTY_ID_INPUT_VOLTAGE_RANGE_SPEC 0x0028
/* Input voltage ripple specification. */
#define MESH_PROPERTY_ID_INPUT_VOLTAGE_RIPPLE_SPEC 0x0029
/* Input voltage statistics. */
#define MESH_PROPERTY_ID_INPUT_VOLTAGE_STAT 0x002A
/* Light control ambient luxlevel on. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_AMB_LUXLEVEL_ON 0x002B
/* Light control ambient luxlevel prolong. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_AMB_LUXLEVEL_PROLONG 0x002C
/* Light control ambient luxlevel standby. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_AMB_LUXLEVEL_STANDBY 0x002D
/* Light control lightness on. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_LIGHTNESS_ON 0x002E
/* Light control lightness prolong. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_LIGHTNESS_PROLONG 0x002F
/* Light control lightness standby. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_LIGHTNESS_STANDBY 0x0030
/* Light control regulator accuracy. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_REG_ACCURACY 0x0031
/* Light control regulator kid. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_REG_KID 0x0032
/* Light control regulator kiu. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_REG_KIU 0x0033
/* Light control regulator kpd. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_REG_KPD 0x0034
/* Light control regulator kpu. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_REG_KPU 0x0035
/* Light control time fade. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_TIME_FADE 0x0036
/* Light control time fade on. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_TIME_FADE_ON 0x0037
/* Light control time fade standby auto. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_TIME_FADE_STANDBY_AUTO 0x0038
/* Light control time fade standby manual. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_TIME_FADE_STANDBY_MANUAL 0x0039
/* Light control time occupancy delay. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_TIME_OCCUPANCY_DELAY 0x003A
/* Light control time prolong. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_TIME_PROLONG 0x003B
/* Light control time run on. */
#define MESH_PROPERTY_ID_LIGHT_CTRL_TIME_RUN_ON 0x003C
/* Lumen maintenance factor. */
#define MESH_PROPERTY_ID_LUMEN_MAINTENANCE_FACTOR 0x003D
/* Luminous efficacy. */
#define MESH_PROPERTY_ID_LUMINOUS_EFFICACY 0x003E
/* Luminous energy since turn on. */
#define MESH_PROPERTY_ID_LUMINOUS_ENERGY_SINCE_TURN_ON 0x003F
/* Luminous exposure. */
#define MESH_PROPERTY_ID_LUMINOUS_EXPOSURE 0x0040
/* Luminous flux range. */
#define MESH_PROPERTY_ID_LUMINOUS_FLUX_RANGE 0x0041
/* Motion sensed. */
#define MESH_PROPERTY_ID_MOTION_SENSED 0x0042
/* Motion threshold. */
#define MESH_PROPERTY_ID_MOTION_THRESHOLD 0x0043
/* Open circuit event statistics. */
#define MESH_PROPERTY_ID_OPEN_CIRCUIT_EVT_STAT 0x0044
/* Outdoor statistical values. */
#define MESH_PROPERTY_ID_OUTDOOR_STAT_VALUES 0x0045
/* Output current range. */
#define MESH_PROPERTY_ID_OUTPUT_CURRENT_RANGE 0x0046
/* Output current statistics. */
#define MESH_PROPERTY_ID_OUTPUT_CURRENT_STAT 0x0047
/* Output ripple voltage specification. */
#define MESH_PROPERTY_ID_OUTPUT_RIPPLE_VOLTAGE_SPEC 0x0048
/* Output voltage range. */
#define MESH_PROPERTY_ID_OUTPUT_VOLTAGE_RANGE 0x0049
/* Output voltage statistics. */
#define MESH_PROPERTY_ID_OUTPUT_VOLTAGE_STAT 0x004A
/* Over output ripple voltage event statistics. */
#define MESH_PROPERTY_ID_OVER_OUTPUT_RIPPLE_VOLTAGE_EVT_STAT 0x004B
/* People count. */
#define MESH_PROPERTY_ID_PEOPLE_COUNT 0x004C
/* Presence detected. */
#define MESH_PROPERTY_ID_PRESENCE_DETECTED 0x004D
/* Present ambient light level. */
#define MESH_PROPERTY_ID_PRESENT_AMB_LIGHT_LEVEL 0x004E
/* Present ambient temperature. */
#define MESH_PROPERTY_ID_PRESENT_AMB_TEMPERATURE 0x004F
/* Present CIE-1931 chromaticity coordinates. */
#define MESH_PROPERTY_ID_PRESENT_CIE_1931_CHROMATICITY_COORDS 0x0050
/* Present correlated color temperature. */
#define MESH_PROPERTY_ID_PRESENT_CORRELATED_COL_TEMPERATURE 0x0051
/* Present device input power. */
#define MESH_PROPERTY_ID_PRESENT_DEV_INPUT_POWER 0x0052
/* Present device operating efficiency. */
#define MESH_PROPERTY_ID_PRESENT_DEV_OP_EFFICIENCY 0x0053
/* Present device operating temperature. */
#define MESH_PROPERTY_ID_PRESENT_DEV_OP_TEMPERATURE 0x0054
/* Present illuminance. */
#define MESH_PROPERTY_ID_PRESENT_ILLUMINANCE 0x0055
/* Present indoor ambient temperature. */
#define MESH_PROPERTY_ID_PRESENT_INDOOR_AMB_TEMPERATURE 0x0056
/* Present input current. */
#define MESH_PROPERTY_ID_PRESENT_INPUT_CURRENT 0x0057
/* Present input ripple voltage. */
#define MESH_PROPERTY_ID_PRESENT_INPUT_RIPPLE_VOLTAGE 0x0058
/* Present input voltage. */
#define MESH_PROPERTY_ID_PRESENT_INPUT_VOLTAGE 0x0059
/* Present luminous flux. */
#define MESH_PROPERTY_ID_PRESENT_LUMINOUS_FLUX 0x005A
/* Present outdoor ambient temperature. */
#define MESH_PROPERTY_ID_PRESENT_OUTDOOR_AMB_TEMPERATURE 0x005B
/* Present output current. */
#define MESH_PROPERTY_ID_PRESENT_OUTPUT_CURRENT 0x005C
/* Present output voltage. */
#define MESH_PROPERTY_ID_PRESENT_OUTPUT_VOLTAGE 0x005D
/* Present planckian distance. */
#define MESH_PROPERTY_ID_PRESENT_PLANCKIAN_DISTANCE 0x005E
/* Present relative output ripple voltage. */
#define MESH_PROPERTY_ID_PRESENT_REL_OUTPUT_RIPPLE_VOLTAGE 0x005F
/* Relative device energy use in a period of day. */
#define MESH_PROPERTY_ID_REL_DEV_ENERGY_USE_IN_A_PERIOD_OF_DAY 0x0060
/* Relative device runtime in a generic level range. */
#define MESH_PROPERTY_ID_REL_DEV_RUNTIME_IN_A_GENERIC_LEVEL_RANGE 0x0061
/* Relative exposure time in an illuminance range. */
#define MESH_PROPERTY_ID_REL_EXPOSURE_TIME_IN_AN_ILLUMINANCE_RANGE 0x0062
/* Relative runtime in a correlated color temperature range. */
#define MESH_PROPERTY_ID_REL_RUNTIME_IN_A_CORRELATED_COL_TEMPERATURE_RANGE 0x0063
/* Relative runtime in a device operating temperature range. */
#define MESH_PROPERTY_ID_REL_RUNTIME_IN_A_DEV_OP_TEMPERATURE_RANGE 0x0064
/* Relative runtime in an input current range. */
#define MESH_PROPERTY_ID_REL_RUNTIME_IN_AN_INPUT_CURRENT_RANGE 0x0065
/* Relative runtime in an input voltage range. */
#define MESH_PROPERTY_ID_REL_RUNTIME_IN_AN_INPUT_VOLTAGE_RANGE 0x0066
/* Short circuit event statistics. */
#define MESH_PROPERTY_ID_SHORT_CIRCUIT_EVT_STAT 0x0067
/* Time since motion sensed. */
#define MESH_PROPERTY_ID_TIME_SINCE_MOTION_SENSED 0x0068
/* Time since presence detected. */
#define MESH_PROPERTY_ID_TIME_SINCE_PRESENCE_DETECTED 0x0069
/* Total device energy use. */
#define MESH_PROPERTY_ID_TOT_DEV_ENERGY_USE 0x006A
/* Total device off on cycles. */
#define MESH_PROPERTY_ID_TOT_DEV_OFF_ON_CYCLES 0x006B
/* Total device power on cycles. */
#define MESH_PROPERTY_ID_TOT_DEV_POWER_ON_CYCLES 0x006C
/* Total device power on time. */
#define MESH_PROPERTY_ID_TOT_DEV_POWER_ON_TIME 0x006D
/* Total device runtime. */
#define MESH_PROPERTY_ID_TOT_DEV_RUNTIME 0x006E
/* Total light exposure time. */
#define MESH_PROPERTY_ID_TOT_LIGHT_EXPOSURE_TIME 0x006F
/* Total luminous energy. */
#define MESH_PROPERTY_ID_TOT_LUMINOUS_ENERGY 0x0070
/* Desired ambient temperature. */
#define MESH_PROPERTY_ID_DESIRED_AMB_TEMPERATURE 0x0071
/* Precise Total Device Energy Use. */
#define MESH_PROPERTY_ID_PRECISE_TOT_DEV_ENERGY_USE 0x0072
/* Power Factor. */
#define MESH_PROPERTY_ID_POWER_FACTOR 0x0073
/* Sensor Gain. */
#define MESH_PROPERTY_ID_SENSOR_GAIN 0x0074
/* Precise Present Ambient Temperature. */
#define MESH_PROPERTY_ID_PRECISE_PRESENT_AMB_TEMPERATURE 0x0075
/* Present Ambient Relative Humidity. */
#define MESH_PROPERTY_ID_PRESENT_AMB_REL_HUMIDITY 0x0076
/* Present Ambient Carbon Dioxide Concentration. */
#define MESH_PROPERTY_ID_PRESENT_AMB_CO2_CONCENTRATION 0x0077
/* Present Ambient Volatile Organic Compounds Concentration. */
#define MESH_PROPERTY_ID_PRESENT_AMB_VOC_CONCENTRATION 0x0078
/* Present Ambient Noise. */
#define MESH_PROPERTY_ID_PRESENT_AMB_NOISE 0x0079
/* Active Energy Loadside. */
#define MESH_PROPERTY_ID_ACTIVE_ENERGY_LOADSIDE 0x0080
/* Active Power Loadside. */
#define MESH_PROPERTY_ID_ACTIVE_POWER_LOADSIDE 0x0081
/* Air Pressure. */
#define MESH_PROPERTY_ID_AIR_PRESSURE 0x0082
/* Apparent Energy. */
#define MESH_PROPERTY_ID_APPARENT_ENERGY 0x0083
/* Apparent Power. */
#define MESH_PROPERTY_ID_APPARENT_POWER 0x0084
/* Apparent Wind Direction. */
#define MESH_PROPERTY_ID_APPARENT_WIND_DIRECTION 0x0085
/* Apparent Wind Speed. */
#define MESH_PROPERTY_ID_APPARENT_WIND_SPEED 0x0086
/* Dew Point. */
#define MESH_PROPERTY_ID_DEW_POINT 0x0087
/* External Supply Voltage. */
#define MESH_PROPERTY_ID_EXTERNAL_SUPPLY_VOLTAGE 0x0088
/* External Supply Voltage Frequency. */
#define MESH_PROPERTY_ID_EXTERNAL_SUPPLY_VOLTAGE_FREQUENCY 0x0089
/* Gust Factor. */
#define MESH_PROPERTY_ID_GUST_FACTOR 0x008A
/* Heat Index. */
#define MESH_PROPERTY_ID_HEAT_INDEX 0x008B
/* Light Distribution. */
#define MESH_PROPERTY_ID_LIGHT_DISTRIBUTION 0x008C
/* Light Source Current. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_CURRENT 0x008D
/* Light Source On Time Not Resettable. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_ON_TIME_NOT_RESETTABLE 0x008E
/* Light Source On Time Resettable. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_ON_TIME_RESETTABLE 0x008F
/* Light Source Open Circuit Statistics. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_OPEN_CIRCUIT_STATISTICS 0x0090
/* Light Source Overall Failures Statistics. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_OVERALL_FAILURES_STATISTICS 0x0091
/* Light Source Short Circuit Statistics. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_SHORT_CIRCUIT_STATISTICS 0x0092
/* Light Source Start Counter Resettable. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_START_COUNTER_RESETTABLE 0x0093
/* Light Source Temperature. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_TEMPERATURE 0x0094
/* Light Source Thermal Derating Statistics. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_THERMAL_DERATING_STATISTICS 0x0095
/* Light Source Thermal Shutdown Statistics. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_THERMAL_SHUTDOWN_STATISTICS 0x0096
/* Light Source Total Power On Cycles. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_TOTAL_POWER_ON_CYCLES 0x0097
/* Light Source Voltage. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_VOLTAGE 0x0098
/* Luminaire Color. */
#define MESH_PROPERTY_ID_LUMINAIRE_COLOR 0x0099
/* Luminaire Identification Number. */
#define MESH_PROPERTY_ID_LUMINAIRE_IDENTIFICATION_NUMBER 0x009A
/* Luminaire Manufacturer GTIN. */
#define MESH_PROPERTY_ID_LUMINAIRE_MANUFACTURER_GTIN 0x009B
/* Luminaire Nominal Input Power. */
#define MESH_PROPERTY_ID_LUMINAIRE_NOMINAL_INPUT_POWER 0x009C
/* Luminaire Nominal Maximum AC Mains Voltage. */
#define MESH_PROPERTY_ID_LUMINAIRE_NOMINAL_MAXIMUM_AC_MAINS_VOLTAGE 0x009D
/* Luminaire Nominal Minimum AC Mains Voltage. */
#define MESH_PROPERTY_ID_LUMINAIRE_NOMINAL_MINIMUM_AC_MAINS_VOLTAGE 0x009E
/* Luminaire Power At Minimum Dim Level. */
#define MESH_PROPERTY_ID_LUMINAIRE_POWER_AT_MINIMUM_DIM_LEVEL 0x009F
/* Luminaire Time Of Manufacture. */
#define MESH_PROPERTY_ID_LUMINAIRE_TIME_OF_MANUFACTURE 0x00A0
/* Magnetic Declination. */
#define MESH_PROPERTY_ID_MAGNETIC_DECLINATION 0x00A1
/* Magnetic Flux Density - 2D. */
#define MESH_PROPERTY_ID_MAGNETIC_FLUX_DENSITY_2D 0x00A2
/* Magnetic Flux Density - 3D. */
#define MESH_PROPERTY_ID_MAGNETIC_FLUX_DENSITY_3D 0x00A3
/* Nominal Light Output. */
#define MESH_PROPERTY_ID_NOMINAL_LIGHT_OUTPUT 0x00A4
/* Overall Failure Condition. */
#define MESH_PROPERTY_ID_OVERALL_FAILURE_CONDITION 0x00A5
/* Pollen Concentration. */
#define MESH_PROPERTY_ID_POLLEN_CONCENTRATION 0x00A6
/* Present Indoor Relative Humidity. */
#define MESH_PROPERTY_ID_PRESENT_INDOOR_RELATIVE_HUMIDITY 0x00A7
/* Present Outdoor Relative Humidity. */
#define MESH_PROPERTY_ID_PRESENT_OUTDOOR_RELATIVE_HUMIDITY 0x00A8
/* Pressure. */
#define MESH_PROPERTY_ID_PRESSURE 0x00A9
/* Rainfall. */
#define MESH_PROPERTY_ID_RAINFALL 0x00AA
/* Rated Median Useful Life Of Luminaire. */
#define MESH_PROPERTY_ID_RATED_MEDIAN_USEFUL_LIFE_OF_LUMINAIRE 0x00AB
/* Rated Median Useful Light Source Starts. */
#define MESH_PROPERTY_ID_RATED_MEDIAN_USEFUL_LIGHT_SOURCE_STARTS 0x00AC
/* Reference Temperature. */
#define MESH_PROPERTY_ID_REFERENCE_TEMPERATURE 0x00AD
/* Total Device Starts. */
#define MESH_PROPERTY_ID_TOTAL_DEVICE_STARTS 0x00AE
/* True Wind Direction. */
#define MESH_PROPERTY_ID_TRUE_WIND_DIRECTION 0x00AF
/* True Wind Speed. */
#define MESH_PROPERTY_ID_TRUE_WIND_SPEED 0x00B0
/* UV Index. */
#define MESH_PROPERTY_ID_UV_INDEX 0x00B1
/* Wind Chill. */
#define MESH_PROPERTY_ID_WIND_CHILL 0x00B2
/* Light Source Type. */
#define MESH_PROPERTY_ID_LIGHT_SOURCE_TYPE 0x00B3
/* Luminaire Identification String. */
#define MESH_PROPERTY_ID_LUMINAIRE_IDENTIFICATION_STRING 0x00B4
/* Output Power Limitation. */
#define MESH_PROPERTY_ID_OUTPUT_POWER_LIMITATION 0x00B5
/* Thermal Derating. */
#define MESH_PROPERTY_ID_THERMAL_DERATING 0x00B6
/* Output Current Percent. */
#define MESH_PROPERTY_ID_OUTPUT_CURRENT_PERCENT 0x00B7
/* Motion Threshold Steps. */
#define MESH_PROPERTY_ID_MOTION_THRESHOLD_STEPS 0x00B8

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_PROPERTIES_H_ */
