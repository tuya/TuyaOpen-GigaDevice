/** @file
 *  @brief Bluetooth subsystem core APIs.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef INCLUDE_MESH_BLUETOOTH_BLUETOOTH_H_
#define INCLUDE_MESH_BLUETOOTH_BLUETOOTH_H_

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "sys/mesh_atomic.h"
#include "mesh_util.h"
#include "net/buf.h"
#include "bluetooth/bt_crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convenience macro for specifying the default identity. This helps
 * make the code more readable, especially when only one identity is
 * supported.
 */
#define BT_ID_DEFAULT 0

/** Length in bytes of a standard Bluetooth address */
#define BT_ADDR_SIZE 6

/** Bluetooth Device Address */
typedef struct {
	uint8_t  val[BT_ADDR_SIZE];
} bt_addr_t;
/**/

/** Length in bytes of an LE Bluetooth address. Not packed, so no sizeof() */
#define BT_ADDR_LE_SIZE 7

/** Bluetooth LE Device Address */
typedef struct {
	uint8_t      type;
	bt_addr_t a;
} bt_addr_le_t;


/** @brief Copy Bluetooth device address.
 *
 *  @param dst Bluetooth device address destination buffer.
 *  @param src Bluetooth device address source buffer.
 */
static inline void bt_addr_copy(bt_addr_t *dst, const bt_addr_t *src)
{
	memcpy(dst, src, sizeof(*dst));
}


/** @brief Compare Bluetooth LE device addresses.
 *
 *  @param a First Bluetooth LE device address to compare
 *  @param b Second Bluetooth LE device address to compare
 *
 *  @return negative value if @a a < @a b, 0 if @a a == @a b, else positive
 *
 *  @sa bt_addr_le_eq
 */
static inline int bt_addr_le_cmp(const bt_addr_le_t *a, const bt_addr_le_t *b)
{
	return memcmp(a, b, sizeof(*a));
}

#define BT_ADDR_LE_PUBLIC       0x00
#define BT_ADDR_LE_RANDOM       0x01
#define BT_ADDR_LE_PUBLIC_ID    0x02
#define BT_ADDR_LE_RANDOM_ID    0x03
#define BT_ADDR_LE_UNRESOLVED   0xFE /* Resolvable Private Address
				      * (Controller unable to resolve)
				      */
#define BT_ADDR_LE_ANONYMOUS    0xFF /* No address provided
				      * (anonymous advertisement)
				      */

/**
 *  @brief Recommended length of user string buffer for Bluetooth address
 *
 *  @details The recommended length guarantee the output of address
 *  conversion will not lose valuable information about address being
 *  processed.
 */
#define BT_ADDR_STR_LEN 18

/**
 *  @brief Recommended length of user string buffer for Bluetooth LE address
 *
 *  @details The recommended length guarantee the output of address
 *  conversion will not lose valuable information about address being
 *  processed.
 */
#define BT_ADDR_LE_STR_LEN 30

/** @brief Converts binary Bluetooth address to string.
 *
 *  @param addr Address of buffer containing binary Bluetooth address.
 *  @param str Address of user buffer with enough room to store formatted
 *  string containing binary address.
 *  @param len Length of data to be copied to user string buffer. Refer to
 *  BT_ADDR_STR_LEN about recommended value.
 *
 *  @return Number of successfully formatted bytes from binary address.
 */
static inline int bt_addr_to_str(const bt_addr_t *addr, char *str, size_t len)
{
	return snprintf(str, len, "%02X:%02X:%02X:%02X:%02X:%02X",
			addr->val[5], addr->val[4], addr->val[3],
			addr->val[2], addr->val[1], addr->val[0]);
}

/** @brief Converts binary LE Bluetooth address to string.
 *
 *  @param addr Address of buffer containing binary LE Bluetooth address.
 *  @param str Address of user buffer with enough room to store
 *  formatted string containing binary LE address.
 *  @param len Length of data to be copied to user string buffer. Refer to
 *  BT_ADDR_LE_STR_LEN about recommended value.
 *
 *  @return Number of successfully formatted bytes from binary address.
 */
static inline int bt_addr_le_to_str(const bt_addr_le_t *addr, char *str,
				    size_t len)
{
	char type[10];

	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		strcpy(type, "public");
		break;
	case BT_ADDR_LE_RANDOM:
		strcpy(type, "random");
		break;
	case BT_ADDR_LE_PUBLIC_ID:
		strcpy(type, "public-id");
		break;
	case BT_ADDR_LE_RANDOM_ID:
		strcpy(type, "random-id");
		break;
	default:
		snprintf(type, sizeof(type), "0x%02x", addr->type);
		break;
	}

	return snprintf(str, len, "%02X:%02X:%02X:%02X:%02X:%02X (%s)",
			addr->a.val[5], addr->a.val[4], addr->a.val[3],
			addr->a.val[2], addr->a.val[1], addr->a.val[0], type);
}


#define BT_HCI_ADV_IND                          0x00
#define BT_HCI_ADV_DIRECT_IND                   0x01
#define BT_HCI_ADV_SCAN_IND                     0x02
#define BT_HCI_ADV_NONCONN_IND                  0x03
#define BT_HCI_ADV_DIRECT_IND_LOW_DUTY          0x04
#define BT_HCI_ADV_SCAN_RSP                     0x04


/**
 * @name EIR/AD data type definitions
 * @{
 */
#define BT_DATA_FLAGS                   0x01 /**< AD flags */
#define BT_DATA_UUID16_SOME             0x02 /**< 16-bit UUID, more available */
#define BT_DATA_UUID16_ALL              0x03 /**< 16-bit UUID, all listed */
#define BT_DATA_UUID32_SOME             0x04 /**< 32-bit UUID, more available */
#define BT_DATA_UUID32_ALL              0x05 /**< 32-bit UUID, all listed */
#define BT_DATA_UUID128_SOME            0x06 /**< 128-bit UUID, more available */
#define BT_DATA_UUID128_ALL             0x07 /**< 128-bit UUID, all listed */
#define BT_DATA_NAME_SHORTENED          0x08 /**< Shortened name */
#define BT_DATA_NAME_COMPLETE           0x09 /**< Complete name */
#define BT_DATA_TX_POWER                0x0a /**< Tx Power */
#define BT_DATA_SM_TK_VALUE             0x10 /**< Security Manager TK Value */
#define BT_DATA_SM_OOB_FLAGS            0x11 /**< Security Manager OOB Flags */
#define BT_DATA_PERIPHERAL_INT_RANGE    0x12 /**< Peripheral Connection Interval Range */
#define BT_DATA_SOLICIT16               0x14 /**< Solicit UUIDs, 16-bit */
#define BT_DATA_SOLICIT128              0x15 /**< Solicit UUIDs, 128-bit */
#define BT_DATA_SVC_DATA16              0x16 /**< Service data, 16-bit UUID */
#define BT_DATA_PUB_TARGET_ADDR         0x17 /**< Public Target Address */
#define BT_DATA_RAND_TARGET_ADDR        0x18 /**< Random Target Address */
#define BT_DATA_GAP_APPEARANCE          0x19 /**< GAP appearance */
#define BT_DATA_ADV_INT                 0x1a /**< Advertising Interval */
#define BT_DATA_LE_BT_DEVICE_ADDRESS    0x1b /**< LE Bluetooth Device Address */
#define BT_DATA_LE_ROLE                 0x1c /**< LE Role */
#define BT_DATA_SIMPLE_PAIRING_HASH     0x1d /**< Simple Pairing Hash C256 */
#define BT_DATA_SIMPLE_PAIRING_RAND     0x1e /**< Simple Pairing Randomizer R256 */
#define BT_DATA_SOLICIT32               0x1f /**< Solicit UUIDs, 32-bit */
#define BT_DATA_SVC_DATA32              0x20 /**< Service data, 32-bit UUID */
#define BT_DATA_SVC_DATA128             0x21 /**< Service data, 128-bit UUID */
#define BT_DATA_LE_SC_CONFIRM_VALUE     0x22 /**< LE SC Confirmation Value */
#define BT_DATA_LE_SC_RANDOM_VALUE      0x23 /**< LE SC Random Value */
#define BT_DATA_URI                     0x24 /**< URI */
#define BT_DATA_INDOOR_POS              0x25 /**< Indoor Positioning */
#define BT_DATA_TRANS_DISCOVER_DATA     0x26 /**< Transport Discovery Data */
#define BT_DATA_LE_SUPPORTED_FEATURES   0x27 /**< LE Supported Features */
#define BT_DATA_CHANNEL_MAP_UPDATE_IND  0x28 /**< Channel Map Update Indication */
#define BT_DATA_MESH_PROV               0x29 /**< Mesh Provisioning PDU */
#define BT_DATA_MESH_MESSAGE            0x2a /**< Mesh Networking PDU */
#define BT_DATA_MESH_BEACON             0x2b /**< Mesh Beacon */
#define BT_DATA_BIG_INFO                0x2c /**< BIGInfo */
#define BT_DATA_BROADCAST_CODE          0x2d /**< Broadcast Code */
#define BT_DATA_CSIS_RSI                0x2e /**< CSIS Random Set ID type */
#define BT_DATA_ADV_INT_LONG            0x2f /**< Advertising Interval long */
#define BT_DATA_BROADCAST_NAME          0x30 /**< Broadcast Name */
#define BT_DATA_ENCRYPTED_AD_DATA       0x31 /**< Encrypted Advertising Data */
#define BT_DATA_3D_INFO                 0x3D /**< 3D Information Data */

#define BT_DATA_MANUFACTURER_DATA       0xff /**< Manufacturer Specific Data */

#define BT_LE_AD_LIMITED                0x01 /**< Limited Discoverable */
#define BT_LE_AD_GENERAL                0x02 /**< General Discoverable */
#define BT_LE_AD_NO_BREDR               0x04 /**< BR/EDR not supported */

enum {
	/* Advertising set has been created in the host. */
	BT_ADV_CREATED,
	/* Advertising parameters has been set in the controller.
	 * This implies that the advertising set has been created in the
	 * controller.
	 */
	BT_ADV_PARAMS_SET,
	/* Advertising data has been set in the controller. */
	BT_ADV_DATA_SET,
	/* Advertising random address pending to be set in the controller. */
	BT_ADV_RANDOM_ADDR_PENDING,
	/* The private random address of the advertiser is valid for this cycle
	 * of the RPA timeout.
	 */
	BT_ADV_RPA_VALID,
	/* The private random address of the advertiser is being updated. */
	BT_ADV_RPA_UPDATE,
	/* The advertiser set is limited by a timeout, or number of advertising
	 * events, or both.
	 */
	BT_ADV_LIMITED,
	/* Advertiser set is currently advertising in the controller. */
	BT_ADV_ENABLED,
	/* Advertiser should include name in advertising data */
	BT_ADV_INCLUDE_NAME_AD,
	/* Advertiser should include name in scan response data */
	BT_ADV_INCLUDE_NAME_SD,
	/* Advertiser set is connectable */
	BT_ADV_CONNECTABLE,
	/* Advertiser set is scannable */
	BT_ADV_SCANNABLE,
	/* Advertiser set is using extended advertising */
	BT_ADV_EXT_ADV,
	/* Advertiser set has disabled the use of private addresses and is using
	 * the identity address instead.
	 */
	BT_ADV_USE_IDENTITY,
	/* Advertiser has been configured to keep advertising after a connection
	 * has been established as long as there are connections available.
	 */
	BT_ADV_PERSIST,
	/* Advertiser has been temporarily disabled. */
	BT_ADV_PAUSED,
	/* Periodic Advertising has been enabled in the controller. */
	BT_PER_ADV_ENABLED,
	/* Periodic Advertising parameters has been set in the controller. */
	BT_PER_ADV_PARAMS_SET,
	/* Periodic Advertising to include AdvDataInfo (ADI) */
	BT_PER_ADV_INCLUDE_ADI,
	/* Constant Tone Extension parameters for Periodic Advertising
	 * has been set in the controller.
	 */
	BT_PER_ADV_CTE_PARAMS_SET,
	/* Constant Tone Extension for Periodic Advertising has been enabled
	 * in the controller.
	 */
	BT_PER_ADV_CTE_ENABLED,

	BT_ADV_NUM_FLAGS,
};

/** Advertising PDU types */
enum {
	/** Scannable and connectable advertising. */
	BT_GAP_ADV_TYPE_ADV_IND               = 0x00,
	/** Directed connectable advertising. */
	BT_GAP_ADV_TYPE_ADV_DIRECT_IND        = 0x01,
	/** Non-connectable and scannable advertising. */
	BT_GAP_ADV_TYPE_ADV_SCAN_IND          = 0x02,
	/** Non-connectable and non-scannable advertising. */
	BT_GAP_ADV_TYPE_ADV_NONCONN_IND       = 0x03,
	/** Additional advertising data requested by an active scanner. */
	BT_GAP_ADV_TYPE_SCAN_RSP              = 0x04,
	/** Extended advertising, see advertising properties. */
	BT_GAP_ADV_TYPE_EXT_ADV               = 0x05,
};

/** Advertising PDU properties */
enum {
	/** Connectable advertising. */
	BT_GAP_ADV_PROP_CONNECTABLE           = BIT(0),
	/** Scannable advertising. */
	BT_GAP_ADV_PROP_SCANNABLE             = BIT(1),
	/** Directed advertising. */
	BT_GAP_ADV_PROP_DIRECTED              = BIT(2),
	/** Additional advertising data requested by an active scanner. */
	BT_GAP_ADV_PROP_SCAN_RESPONSE         = BIT(3),
	/** Extended advertising. */
	BT_GAP_ADV_PROP_EXT_ADV               = BIT(4),
};

/** Advertising options */
enum {
	/** Convenience value when no options are specified. */
	BT_LE_ADV_OPT_NONE = 0,

	/**
	 * @brief Advertise as connectable.
	 *
	 * Advertise as connectable. If not connectable then the type of
	 * advertising is determined by providing scan response data.
	 * The advertiser address is determined by the type of advertising
	 * and/or enabling privacy @kconfig{CONFIG_BT_PRIVACY}.
	 */
	BT_LE_ADV_OPT_CONNECTABLE = BIT(0),

	/**
	 * @brief Advertise one time.
	 *
	 * Don't try to resume connectable advertising after a connection.
	 * This option is only meaningful when used together with
	 * BT_LE_ADV_OPT_CONNECTABLE. If set the advertising will be stopped
	 * when bt_le_adv_stop() is called or when an incoming (peripheral)
	 * connection happens. If this option is not set the stack will
	 * take care of keeping advertising enabled even as connections
	 * occur.
	 * If Advertising directed or the advertiser was started with
	 * @ref bt_le_ext_adv_start then this behavior is the default behavior
	 * and this flag has no effect.
	 */
	BT_LE_ADV_OPT_ONE_TIME = BIT(1),

	/**
	 * @brief Advertise using identity address.
	 *
	 * Advertise using the identity address as the advertiser address.
	 * @warning This will compromise the privacy of the device, so care
	 *          must be taken when using this option.
	 * @note The address used for advertising will not be the same as
	 *        returned by @ref bt_le_oob_get_local, instead @ref bt_id_get
	 *        should be used to get the LE address.
	 */
	BT_LE_ADV_OPT_USE_IDENTITY = BIT(2),

	/**
	 * @deprecated This option will be removed in the near future, see
	 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
	 *
	 * @brief Advertise using GAP device name.
	 *
	 * Include the GAP device name automatically when advertising.
	 * By default the GAP device name is put at the end of the scan
	 * response data.
	 * When advertising using @ref BT_LE_ADV_OPT_EXT_ADV and not
	 * @ref BT_LE_ADV_OPT_SCANNABLE then it will be put at the end of the
	 * advertising data.
	 * If the GAP device name does not fit into advertising data it will be
	 * converted to a shortened name if possible.
	 * @ref BT_LE_ADV_OPT_FORCE_NAME_IN_AD can be used to force the device
	 * name to appear in the advertising data of an advert with scan
	 * response data.
	 *
	 * The application can set the device name itself by including the
	 * following in the advertising data.
	 * @code
	 * BT_DATA(BT_DATA_NAME_COMPLETE, name, sizeof(name) - 1)
	 * @endcode
	 */
	BT_LE_ADV_OPT_USE_NAME = BIT(3),

	/**
	 * @brief Low duty cycle directed advertising.
	 *
	 * Use low duty directed advertising mode, otherwise high duty mode
	 * will be used.
	 */
	BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY = BIT(4),

	/**
	 * @brief Directed advertising to privacy-enabled peer.
	 *
	 * Enable use of Resolvable Private Address (RPA) as the target address
	 * in directed advertisements.
	 * This is required if the remote device is privacy-enabled and
	 * supports address resolution of the target address in directed
	 * advertisement.
	 * It is the responsibility of the application to check that the remote
	 * device supports address resolution of directed advertisements by
	 * reading its Central Address Resolution characteristic.
	 */
	BT_LE_ADV_OPT_DIR_ADDR_RPA = BIT(5),

	/** Use filter accept list to filter devices that can request scan
	 *  response data.
	 */
	BT_LE_ADV_OPT_FILTER_SCAN_REQ = BIT(6),

	/** Use filter accept list to filter devices that can connect. */
	BT_LE_ADV_OPT_FILTER_CONN = BIT(7),

	/** Notify the application when a scan response data has been sent to an
	 *  active scanner.
	 */
	BT_LE_ADV_OPT_NOTIFY_SCAN_REQ = BIT(8),

	/**
	 * @brief Support scan response data.
	 *
	 * When used together with @ref BT_LE_ADV_OPT_EXT_ADV then this option
	 * cannot be used together with the @ref BT_LE_ADV_OPT_CONNECTABLE
	 * option.
	 * When used together with @ref BT_LE_ADV_OPT_EXT_ADV then scan
	 * response data must be set.
	 */
	BT_LE_ADV_OPT_SCANNABLE = BIT(9),

	/**
	 * @brief Advertise with extended advertising.
	 *
	 * This options enables extended advertising in the advertising set.
	 * In extended advertising the advertising set will send a small header
	 * packet on the three primary advertising channels. This small header
	 * points to the advertising data packet that will be sent on one of
	 * the 37 secondary advertising channels.
	 * The advertiser will send primary advertising on LE 1M PHY, and
	 * secondary advertising on LE 2M PHY.
	 * Connections will be established on LE 2M PHY.
	 *
	 * Without this option the advertiser will send advertising data on the
	 * three primary advertising channels.
	 *
	 * @note Enabling this option requires extended advertising support in
	 *       the peer devices scanning for advertisement packets.
	 *
	 * @note This cannot be used with bt_le_adv_start().
	 */
	BT_LE_ADV_OPT_EXT_ADV = BIT(10),

	/**
	 * @brief Disable use of LE 2M PHY on the secondary advertising
	 * channel.
	 *
	 * Disabling the use of LE 2M PHY could be necessary if scanners don't
	 * support the LE 2M PHY.
	 * The advertiser will send primary advertising on LE 1M PHY, and
	 * secondary advertising on LE 1M PHY.
	 * Connections will be established on LE 1M PHY.
	 *
	 * @note Cannot be set if BT_LE_ADV_OPT_CODED is set.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV.
	 */
	BT_LE_ADV_OPT_NO_2M = BIT(11),

	/**
	 * @brief Advertise on the LE Coded PHY (Long Range).
	 *
	 * The advertiser will send both primary and secondary advertising
	 * on the LE Coded PHY. This gives the advertiser increased range with
	 * the trade-off of lower data rate and higher power consumption.
	 * Connections will be established on LE Coded PHY.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 */
	BT_LE_ADV_OPT_CODED = BIT(12),

	/**
	 * @brief Advertise without a device address (identity or RPA).
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 */
	BT_LE_ADV_OPT_ANONYMOUS = BIT(13),

	/**
	 * @brief Advertise with transmit power.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 */
	BT_LE_ADV_OPT_USE_TX_POWER = BIT(14),

	/** Disable advertising on channel index 37. */
	BT_LE_ADV_OPT_DISABLE_CHAN_37 = BIT(15),

	/** Disable advertising on channel index 38. */
	BT_LE_ADV_OPT_DISABLE_CHAN_38 = BIT(16),

	/** Disable advertising on channel index 39. */
	BT_LE_ADV_OPT_DISABLE_CHAN_39 = BIT(17),

	/**
	 * @deprecated This option will be removed in the near future, see
	 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
	 *
	 * @brief Put GAP device name into advert data
	 *
	 * Will place the GAP device name into the advertising data rather than
	 * the scan response data.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_USE_NAME
	 */
	BT_LE_ADV_OPT_FORCE_NAME_IN_AD = BIT(18),

	/**
	 * @brief Advertise using a Non-Resolvable Private Address.
	 *
	 * A new NRPA is set when updating the advertising parameters.
	 *
	 * This is an advanced feature; most users will want to enable
	 * @kconfig{CONFIG_BT_EXT_ADV} instead.
	 *
	 * @note Not implemented when @kconfig{CONFIG_BT_PRIVACY}.
	 *
	 * @note Mutually exclusive with BT_LE_ADV_OPT_USE_IDENTITY.
	 */
	BT_LE_ADV_OPT_USE_NRPA = BIT(19),
};

/**
 * @name Defined GAP timers
 * @{
 */
#define BT_GAP_SCAN_FAST_INTERVAL_MIN           0x0030  /* 30 ms    */
#define BT_GAP_SCAN_FAST_INTERVAL               0x0060  /* 60 ms    */
#define BT_GAP_SCAN_FAST_WINDOW                 0x0030  /* 30 ms    */
#define BT_GAP_SCAN_SLOW_INTERVAL_1             0x0800  /* 1.28 s   */
#define BT_GAP_SCAN_SLOW_WINDOW_1               0x0012  /* 11.25 ms */
#define BT_GAP_SCAN_SLOW_INTERVAL_2             0x1000  /* 2.56 s   */
#define BT_GAP_SCAN_SLOW_WINDOW_2               0x0012  /* 11.25 ms */
#define BT_GAP_ADV_FAST_INT_MIN_1               0x0030  /* 30 ms    */
#define BT_GAP_ADV_FAST_INT_MAX_1               0x0060  /* 60 ms    */
#define BT_GAP_ADV_FAST_INT_MIN_2               0x00a0  /* 100 ms   */
#define BT_GAP_ADV_FAST_INT_MAX_2               0x00f0  /* 150 ms   */
#define BT_GAP_ADV_SLOW_INT_MIN                 0x0640  /* 1 s      */
#define BT_GAP_ADV_SLOW_INT_MAX                 0x0780  /* 1.2 s    */
#define BT_GAP_PER_ADV_FAST_INT_MIN_1           0x0018  /* 30 ms    */
#define BT_GAP_PER_ADV_FAST_INT_MAX_1           0x0030  /* 60 ms    */
#define BT_GAP_PER_ADV_FAST_INT_MIN_2           0x0050  /* 100 ms   */
#define BT_GAP_PER_ADV_FAST_INT_MAX_2           0x0078  /* 150 ms   */
#define BT_GAP_PER_ADV_SLOW_INT_MIN             0x0320  /* 1 s      */
#define BT_GAP_PER_ADV_SLOW_INT_MAX             0x03C0  /* 1.2 s    */
#define BT_GAP_INIT_CONN_INT_MIN                0x0018  /* 30 ms    */
#define BT_GAP_INIT_CONN_INT_MAX                0x0028  /* 50 ms    */

struct bt_le_ext_adv;

/** Connection Type */
enum __packed bt_conn_type {
	/** LE Connection Type */
	BT_CONN_TYPE_LE = BIT(0),
	/** BR/EDR Connection Type */
	BT_CONN_TYPE_BR = BIT(1),
	/** SCO Connection Type */
	BT_CONN_TYPE_SCO = BIT(2),
	/** ISO Connection Type */
	BT_CONN_TYPE_ISO = BIT(3),
	/** All Connection Type */
	BT_CONN_TYPE_ALL = BT_CONN_TYPE_LE | BT_CONN_TYPE_BR |
			   BT_CONN_TYPE_SCO | BT_CONN_TYPE_ISO,
};

/** HCI Error Codes, BT Core Spec v5.4 [Vol 1, Part F]. */
#define BT_HCI_ERR_SUCCESS                      0x00
#define BT_HCI_ERR_UNKNOWN_CMD                  0x01
#define BT_HCI_ERR_UNKNOWN_CONN_ID              0x02
#define BT_HCI_ERR_HW_FAILURE                   0x03
#define BT_HCI_ERR_PAGE_TIMEOUT                 0x04
#define BT_HCI_ERR_AUTH_FAIL                    0x05
#define BT_HCI_ERR_PIN_OR_KEY_MISSING           0x06
#define BT_HCI_ERR_MEM_CAPACITY_EXCEEDED        0x07
#define BT_HCI_ERR_CONN_TIMEOUT                 0x08
#define BT_HCI_ERR_CONN_LIMIT_EXCEEDED          0x09
#define BT_HCI_ERR_SYNC_CONN_LIMIT_EXCEEDED     0x0a
#define BT_HCI_ERR_CONN_ALREADY_EXISTS          0x0b
#define BT_HCI_ERR_CMD_DISALLOWED               0x0c
#define BT_HCI_ERR_INSUFFICIENT_RESOURCES       0x0d
#define BT_HCI_ERR_INSUFFICIENT_SECURITY        0x0e
#define BT_HCI_ERR_BD_ADDR_UNACCEPTABLE         0x0f
#define BT_HCI_ERR_CONN_ACCEPT_TIMEOUT          0x10
#define BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL     0x11
#define BT_HCI_ERR_INVALID_PARAM                0x12
#define BT_HCI_ERR_REMOTE_USER_TERM_CONN        0x13
#define BT_HCI_ERR_REMOTE_LOW_RESOURCES         0x14
#define BT_HCI_ERR_REMOTE_POWER_OFF             0x15
#define BT_HCI_ERR_LOCALHOST_TERM_CONN          0x16
#define BT_HCI_ERR_REPEATED_ATTEMPTS            0x17
#define BT_HCI_ERR_PAIRING_NOT_ALLOWED          0x18
#define BT_HCI_ERR_UNKNOWN_LMP_PDU              0x19
#define BT_HCI_ERR_UNSUPP_REMOTE_FEATURE        0x1a
#define BT_HCI_ERR_SCO_OFFSET_REJECTED          0x1b
#define BT_HCI_ERR_SCO_INTERVAL_REJECTED        0x1c
#define BT_HCI_ERR_SCO_AIR_MODE_REJECTED        0x1d
#define BT_HCI_ERR_INVALID_LL_PARAM             0x1e
#define BT_HCI_ERR_UNSPECIFIED                  0x1f
#define BT_HCI_ERR_UNSUPP_LL_PARAM_VAL          0x20
#define BT_HCI_ERR_ROLE_CHANGE_NOT_ALLOWED      0x21
#define BT_HCI_ERR_LL_RESP_TIMEOUT              0x22
#define BT_HCI_ERR_LL_PROC_COLLISION            0x23
#define BT_HCI_ERR_LMP_PDU_NOT_ALLOWED          0x24
#define BT_HCI_ERR_ENC_MODE_NOT_ACCEPTABLE      0x25
#define BT_HCI_ERR_LINK_KEY_CANNOT_BE_CHANGED   0x26
#define BT_HCI_ERR_REQUESTED_QOS_NOT_SUPPORTED  0x27
#define BT_HCI_ERR_INSTANT_PASSED               0x28
#define BT_HCI_ERR_PAIRING_NOT_SUPPORTED        0x29
#define BT_HCI_ERR_DIFF_TRANS_COLLISION         0x2a
#define BT_HCI_ERR_QOS_UNACCEPTABLE_PARAM       0x2c
#define BT_HCI_ERR_QOS_REJECTED                 0x2d
#define BT_HCI_ERR_CHAN_ASSESS_NOT_SUPPORTED    0x2e
#define BT_HCI_ERR_INSUFF_SECURITY              0x2f
#define BT_HCI_ERR_PARAM_OUT_OF_MANDATORY_RANGE 0x30
#define BT_HCI_ERR_ROLE_SWITCH_PENDING          0x32
#define BT_HCI_ERR_RESERVED_SLOT_VIOLATION      0x34
#define BT_HCI_ERR_ROLE_SWITCH_FAILED           0x35
#define BT_HCI_ERR_EXT_INQ_RESP_TOO_LARGE       0x36
#define BT_HCI_ERR_SIMPLE_PAIR_NOT_SUPP_BY_HOST 0x37
#define BT_HCI_ERR_HOST_BUSY_PAIRING            0x38
#define BT_HCI_ERR_CONN_REJECTED_DUE_TO_NO_CHAN 0x39
#define BT_HCI_ERR_CONTROLLER_BUSY              0x3a
#define BT_HCI_ERR_UNACCEPT_CONN_PARAM          0x3b
#define BT_HCI_ERR_ADV_TIMEOUT                  0x3c
#define BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL         0x3d
#define BT_HCI_ERR_CONN_FAIL_TO_ESTAB           0x3e
#define BT_HCI_ERR_MAC_CONN_FAILED              0x3f
#define BT_HCI_ERR_CLOCK_ADJUST_REJECTED        0x40
#define BT_HCI_ERR_SUBMAP_NOT_DEFINED           0x41
#define BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER       0x42
#define BT_HCI_ERR_LIMIT_REACHED                0x43
#define BT_HCI_ERR_OP_CANCELLED_BY_HOST         0x44
#define BT_HCI_ERR_PACKET_TOO_LONG              0x45
#define BT_HCI_ERR_TOO_LATE                     0x46
#define BT_HCI_ERR_TOO_EARLY                    0x47

struct bt_le_ext_adv_sent_info {
	/** The number of advertising events completed. */
	uint8_t num_sent;
};

struct bt_le_ext_adv_connected_info {
	/** Connection object of the new connection */
	struct bt_conn *conn;
};

struct bt_le_ext_adv_scanned_info {
	/** Active scanner LE address and type */
	bt_addr_le_t *addr;
};

struct bt_le_ext_adv_start_param {
	/**
	 * @brief Advertiser timeout (N * 10 ms).
	 *
	 * Application will be notified by the advertiser sent callback.
	 * Set to zero for no timeout.
	 *
	 * When using high duty cycle directed connectable advertising then
	 * this parameters must be set to a non-zero value less than or equal
	 * to the maximum of @ref BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT.
	 *
	 * If privacy @kconfig{CONFIG_BT_PRIVACY} is enabled then the timeout
	 * must be less than @kconfig{CONFIG_BT_RPA_TIMEOUT}.
	 */
	uint16_t timeout;
	/**
	 * @brief Number of advertising events.
	 *
	 * Application will be notified by the advertiser sent callback.
	 * Set to zero for no limit.
	 */
	uint8_t  num_events;
};

struct bt_le_ext_adv_cb {
	/**
	 * @brief The advertising set has finished sending adv data.
	 *
	 * This callback notifies the application that the advertising set has
	 * finished sending advertising data.
	 * The advertising set can either have been stopped by a timeout or
	 * because the specified number of advertising events has been reached.
	 *
	 * @param adv  The advertising set object.
	 * @param info Information about the sent event.
	 */
	void (*sent)(struct bt_le_ext_adv *adv,
		     struct bt_le_ext_adv_sent_info *info);

	/**
	 * @brief The advertising set has accepted a new connection.
	 *
	 * This callback notifies the application that the advertising set has
	 * accepted a new connection.
	 *
	 * @param adv  The advertising set object.
	 * @param info Information about the connected event.
	 */
	void (*connected)(struct bt_le_ext_adv *adv,
			  struct bt_le_ext_adv_connected_info *info);

	/**
	 * @brief The advertising set has sent scan response data.
	 *
	 * This callback notifies the application that the advertising set has
	 * has received a Scan Request packet, and has sent a Scan Response
	 * packet.
	 *
	 * @param adv  The advertising set object.
	 * @param addr Information about the scanned event.
	 */
	void (*scanned)(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_scanned_info *info);

#if defined(CONFIG_BT_PRIVACY)
	/**
	 * @brief The RPA validity of the advertising set has expired.
	 *
	 * This callback notifies the application that the RPA validity of
	 * the advertising set has expired. The user can use this callback
	 * to synchronize the advertising payload update with the RPA rotation.
	 *
	 * If rpa sharing is enabled and rpa expired cb of any adv-sets belonging
	 * to same adv id returns false, then adv-sets will continue with old rpa
	 * through out the rpa rotations.
	 *
	 * @param adv  The advertising set object.
	 *
	 * @return true to rotate the current RPA, or false to use it for the
	 *         next rotation period.
	 */
	bool (*rpa_expired)(struct bt_le_ext_adv *adv);
#endif /* defined(CONFIG_BT_PRIVACY) */

};

struct bt_le_ext_adv {
	/* ID Address used for advertising */
	uint8_t                 id;

	/* Advertising handle */
	uint8_t                 handle;

	/* Current local Random Address */
	bt_addr_le_t            random_addr;

	/* Current target address */
	bt_addr_le_t            target_addr;

	//ATOMIC_DEFINE(flags, BT_ADV_NUM_FLAGS);
  atomic_t flags[ATOMIC_BITMAP_SIZE(BT_ADV_NUM_FLAGS)];

	const struct bt_le_ext_adv_cb *cb;

	/* TX Power in use by the controller */
	int8_t                    tx_power;

	struct k_work_delayable	lim_adv_timeout_work;

	/** The options used to set the parameters for this advertising set
	 * @ref bt_le_adv_param
	 */
	uint32_t options;
};

/** LE Advertising Parameters. */
struct bt_le_adv_param {
	/**
	 * @brief Local identity.
	 *
	 * @note When extended advertising @kconfig{CONFIG_BT_EXT_ADV} is not
	 *       enabled or not supported by the controller it is not possible
	 *       to scan and advertise simultaneously using two different
	 *       random addresses.
	 */
	uint8_t  id;

	/**
	 * @brief Advertising Set Identifier, valid range 0x00 - 0x0f.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 **/
	uint8_t  sid;

	/**
	 * @brief Secondary channel maximum skip count.
	 *
	 * Maximum advertising events the advertiser can skip before it must
	 * send advertising data on the secondary advertising channel.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 */
	uint8_t  secondary_max_skip;

	/** Bit-field of advertising options */
	uint32_t options;

	/** Minimum Advertising Interval (N * 0.625 milliseconds)
	 * Minimum Advertising Interval shall be less than or equal to the
	 * Maximum Advertising Interval. The Minimum Advertising Interval and
	 * Maximum Advertising Interval should not be the same value (as stated
	 * in Bluetooth Core Spec 5.2, section 7.8.5)
	 * Range: 0x0020 to 0x4000
	 */
	uint32_t interval_min;

	/** Maximum Advertising Interval (N * 0.625 milliseconds)
	 * Minimum Advertising Interval shall be less than or equal to the
	 * Maximum Advertising Interval. The Minimum Advertising Interval and
	 * Maximum Advertising Interval should not be the same value (as stated
	 * in Bluetooth Core Spec 5.2, section 7.8.5)
	 * Range: 0x0020 to 0x4000
	 */
	uint32_t interval_max;

	/**
	 * @brief Directed advertising to peer
	 *
	 * When this parameter is set the advertiser will send directed
	 * advertising to the remote device.
	 *
	 * The advertising type will either be high duty cycle, or low duty
	 * cycle if the BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY option is enabled.
	 * When using @ref BT_LE_ADV_OPT_EXT_ADV then only low duty cycle is
	 * allowed.
	 *
	 * In case of connectable high duty cycle if the connection could not
	 * be established within the timeout the connected() callback will be
	 * called with the status set to @ref BT_HCI_ERR_ADV_TIMEOUT.
	 */
	const bt_addr_le_t *peer;
};


/**
 * @brief Bluetooth data.
 *
 * Description of different data types that can be encoded into
 * advertising data. Used to form arrays that are passed to the
 * bt_le_adv_start() function.
 */
struct bt_data {
	uint8_t type;
	uint8_t data_len;
	const uint8_t *data;
};

/**
 * @brief Helper to declare elements of bt_data arrays
 *
 * This macro is mainly for creating an array of struct bt_data
 * elements which is then passed to e.g. @ref bt_le_adv_start().
 *
 * @param _type Type of advertising data field
 * @param _data Pointer to the data field payload
 * @param _data_len Number of bytes behind the _data pointer
 */
#define BT_DATA(_type, _data, _data_len) \
	{ \
		.type = (_type), \
		.data_len = (_data_len), \
		.data = (const uint8_t *)(_data), \
	}

/**
 * @brief Helper to declare elements of bt_data arrays
 *
 * This macro is mainly for creating an array of struct bt_data
 * elements which is then passed to e.g. @ref bt_le_adv_start().
 *
 * @param _type Type of advertising data field
 * @param _bytes Variable number of single-byte parameters
 */
#define BT_DATA_BYTES(_type, _bytes...) \
	BT_DATA(_type, ((uint8_t []) { _bytes }), \
		sizeof((uint8_t []) { _bytes }))

#define BT_HCI_LE_SCAN_PASSIVE                  0x00
#define BT_HCI_LE_SCAN_ACTIVE                   0x01


/** LE scan parameters */
struct bt_le_scan_param {
	/** Scan type (BT_LE_SCAN_TYPE_ACTIVE or BT_LE_SCAN_TYPE_PASSIVE) */
	uint8_t  type;

	/** Bit-field of scanning options. */
	uint32_t options;

	/** Scan interval (N * 0.625 ms) */
	uint16_t interval;

	/** Scan window (N * 0.625 ms) */
	uint16_t window;

	/**
	 * @brief Scan timeout (N * 10 ms)
	 *
	 * Application will be notified by the scan timeout callback.
	 * Set zero to disable timeout.
	 */
	uint16_t timeout;

	/**
	 * @brief Scan interval LE Coded PHY (N * 0.625 MS)
	 *
	 * Set zero to use same as LE 1M PHY scan interval.
	 */
	uint16_t interval_coded;

	/**
	 * @brief Scan window LE Coded PHY (N * 0.625 MS)
	 *
	 * Set zero to use same as LE 1M PHY scan window.
	 */
	uint16_t window_coded;
};

/** LE advertisement and scan response packet information */
struct bt_le_scan_recv_info {
	/**
	 * @brief Advertiser LE address and type.
	 *
	 * If advertiser is anonymous then this address will be
	 * @ref BT_ADDR_LE_ANY.
	 */
	const bt_addr_le_t *addr;

	/** Strength of advertiser signal. */
	int8_t rssi;

	/**
	 * @brief Advertising packet type.
	 *
	 * Uses the BT_GAP_ADV_TYPE_* value.
	 *
	 * May indicate that this is a scan response if the type is
	 * @ref BT_GAP_ADV_TYPE_SCAN_RSP.
	 */
	uint8_t adv_type;

	/**
	 * @brief Advertising packet properties bitfield.
	 *
	 * Uses the BT_GAP_ADV_PROP_* values.
	 * May indicate that this is a scan response if the value contains the
	 * @ref BT_GAP_ADV_PROP_SCAN_RESPONSE bit.
	 *
	 */
	uint16_t adv_props;

};

/** Listener context for (LE) scanning. */
struct bt_le_scan_cb {

	/**
	 * @brief Advertisement packet and scan response received callback.
	 *
	 * @param info Advertiser packet and scan response information.
	 * @param buf  Buffer containing advertiser data.
	 */
	void (*recv)(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf);

	/** @brief The scanner has stopped scanning after scan timeout. */
	void (*timeout)(void);

	sys_snode_t node;
};

/**
 * @typedef bt_le_scan_cb_t
 * @brief Callback type for reporting LE scan results.
 *
 * A function of this type is given to the bt_le_scan_start() function
 * and will be called for any discovered LE device.
 *
 * @param addr Advertiser LE address and type.
 * @param rssi Strength of advertiser signal.
 * @param adv_type Type of advertising response from advertiser.
 *                 Uses the BT_GAP_ADV_TYPE_* values.
 * @param buf Buffer containing advertiser data.
 */
typedef void bt_le_scan_cb_t(const bt_addr_le_t *addr, int8_t rssi,
			     uint8_t adv_type, struct net_buf_simple *buf);

/**
 * @brief Register scanner packet callbacks.
 *
 * Adds the callback structure to the list of callback structures that monitors
 * scanner activity.
 *
 * This callback will be called for all scanner activity, regardless of what
 * API was used to start the scanner.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_le_scan_cb_register(struct bt_le_scan_cb *cb);

/**
 * @brief Unregister scanner packet callbacks.
 *
 * Remove the callback structure from the list of scanner callbacks.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_le_scan_cb_unregister(struct bt_le_scan_cb *cb);

typedef void (*bt_gatt_complete_func_t) (uint8_t conn_idx, uint16_t status, void *user_data);


enum {
	BT_GATT_ITER_STOP = 0,
	BT_GATT_ITER_CONTINUE,
};

/** GATT attribute permission bit field values */
enum bt_gatt_perm {
	/** No operations supported, e.g. for notify-only */
	BT_GATT_PERM_NONE = 0,

	/** Attribute read permission. */
	BT_GATT_PERM_READ = BIT(0),

	/** Attribute write permission. */
	BT_GATT_PERM_WRITE = BIT(1),

	/** @brief Attribute read permission with encryption.
	 *
	 *  If set, requires encryption for read access.
	 */
	BT_GATT_PERM_READ_ENCRYPT = BIT(2),

	/** @brief Attribute write permission with encryption.
	 *
	 *  If set, requires encryption for write access.
	 */
	BT_GATT_PERM_WRITE_ENCRYPT = BIT(3),

	/** @brief Attribute read permission with authentication.
	 *
	 *  If set, requires encryption using authenticated link-key for read
	 *  access.
	 */
	BT_GATT_PERM_READ_AUTHEN = BIT(4),

	/** @brief Attribute write permission with authentication.
	 *
	 *  If set, requires encryption using authenticated link-key for write
	 *  access.
	 */
	BT_GATT_PERM_WRITE_AUTHEN = BIT(5),

	/** @brief Attribute prepare write permission.
	 *
	 *  If set, allows prepare writes with use of ``BT_GATT_WRITE_FLAG_PREPARE``
	 *  passed to write callback.
	 */
	BT_GATT_PERM_PREPARE_WRITE = BIT(6),

	/** @brief Attribute read permission with LE Secure Connection encryption.
	 *
	 *  If set, requires that LE Secure Connections is used for read access.
	 */
	BT_GATT_PERM_READ_LESC = BIT(7),

	/** @brief Attribute write permission with LE Secure Connection encryption.
	 *
	 *  If set, requires that LE Secure Connections is used for write access.
	 */
	BT_GATT_PERM_WRITE_LESC = BIT(8),
};

/** @brief GATT Attribute structure. */
struct bt_gatt_attr {
	/** Attribute handle */
	uint16_t handle;
	/** @brief Attribute permissions.
	 *
	 * Will be 0 if returned from ``bt_gatt_discover()``.
	 */
	uint16_t perm;
};


/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* INCLUDE_MESH_BLUETOOTH_BLUETOOTH_H_ */
