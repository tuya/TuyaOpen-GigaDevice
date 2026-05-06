
#include <stdbool.h>

#ifndef MESH_CFG_H_
#define MESH_CFG_H_

//#define CONFIG_BT_TESTING                                 true
#define CONFIG_BT_DEVICE_NAME                             ("GD-BLE-MESH")
#define CONFIG_MESH_CB_REGISTERED                           false

#define CONFIG_BT_EXT_ADV_MAX_ADV_SET                     4
#define CONFIG_BT_CONN                                    true

/// The module gathers statistics of received, relayed, and transmitted
/// frames. This helps to estimate the quality of the communication and
/// the sufficiency of configured advertiser instances.
#define CONFIG_BT_MESH_STATISTIC                          false

#define CONFIG_BT_MESH_PROXY                              true

#define CONFIG_BT_MESH_GATT_CLIENT                        true
#define CONFIG_BT_MESH_GATT_SERVER                        true
#define CONFIG_BT_MESH_PB_GATT_COMMON                     true
#define CONFIG_BT_MESH_PROV                               true

/* menuconfig BT_MESH_RELAY */
/// Support for acting as a Mesh Relay Node.
#define CONFIG_BT_MESH_RELAY                              true

/* menuconfig BT_MESH_PB_ADV */
/// Enable this option to allow the device to be provisioned over the advertising bearer.
#define CONFIG_BT_MESH_PB_ADV                                         true
#if (CONFIG_BT_MESH_PB_ADV && (!CONFIG_BT_MESH_PROV))
#error "CONFIG_BT_MESH_PB_ADV select CONFIG_BT_MESH_PROV error!"
#endif

/* menuconfig "Provisioning" */
/// Mesh Configuration Database
#define CONFIG_BT_MESH_CDB                                            true

/* menuconfig "Proxy" */
/// This option enables support for the Mesh GATT Proxy Service,
/// i.e. the ability to act as a proxy between a Mesh GATT Client
/// and a Mesh network.
#define CONFIG_BT_MESH_GATT_PROXY                                   true
#if (CONFIG_BT_MESH_GATT_PROXY && (!CONFIG_BT_MESH_GATT_SERVER || !CONFIG_BT_MESH_PROXY))
#error "BT_MESH_GATT_PROXY select BT_MESH_GATT_SERVER and BT_MESH_PROXY"
#endif

/* menuconfig "Low Power features" */
/// Enable this option to be able to act as a Low Power Node.
#define CONFIG_BT_MESH_LOW_POWER                                    true

/* menuconfig "Friend" */
/// Enable this option to be able to act as a Friend Node.
#define CONFIG_BT_MESH_FRIEND                                       true


#if (CONFIG_BT_MESH_PB_GATT_COMMON)
#define CONFIG_BT_MESH_PROXY_MSG_LEN                      66
#else
#define CONFIG_BT_MESH_PROXY_MSG_LEN                      33
#endif

#if (CONFIG_BT_CONN)
#define CONFIG_BT_MAX_CONN                                2
/// Maximum number of simultaneous Bluetooth connections that the Bluetooth
/// mesh stack can use.
#define CONFIG_BT_MESH_MAX_CONN                           CONFIG_BT_MAX_CONN
#endif    // CONFIG_BT_CONN

/// Size of bt mesh adv thread stack.
#define CONFIG_BT_MESH_ADV_STACK_SIZE                     1024
/// Priority of bt mesh adv thread.
#define CONFIG_BT_MESH_ADV_PRIO                           2

/* menu "BT_MESH_ADV_EXT" */
#if (CONFIG_BT_MESH_RELAY)
/// Maximum of simultaneous relay message support. Requires controller support
/// multiple advertising sets.
#define CONFIG_BT_MESH_RELAY_ADV_SETS                                 0     // range 0 ~ BT_EXT_ADV_MAX_ADV_SET
#else
#define CONFIG_BT_MESH_RELAY_ADV_SETS                                 0     // range 0 ~ BT_EXT_ADV_MAX_ADV_SET
#endif // CONFIG_BT_MESH_RELAY

#if (CONFIG_BT_MESH_RELAY_ADV_SETS > 0)
/// When this option is enabled, there is a message that needs to be
/// relayed, all relay advertising sets defined by
/// CONFIG_BT_MESH_RELAY_ADV_SETS are busy with relaying messages
/// and the main advertising set is not busy with sending local
/// messages, the stack will use the main advertising set to relay
/// the message. This maximizes the utilization efficiency of
/// advertising sets, which is helpful for the sending of dense
/// relays. With CONFIG_BT_MESH_RELAY_RETRANSMIT_COUNT value
/// greater than zero, this can noticeably delay transmission of
/// local messages. When Friend feature is enabled and the node is
/// in a friendship, this option can delay transmission of local
/// messages thus requiring bigger CONFIG_BT_MESH_FRIEND_RECV_WIN
/// value. This in turn will result in increase of the power
/// consumption of the Low Power node.
#define CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET              true
#else
#define CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET              false
#endif  // CONFIG_BT_MESH_RELAY_ADV_SETS > 0

/// Use a separate extended advertising set for GATT Server Advertising,
/// otherwise a shared advertising set will be used.
#if (CONFIG_BT_MESH_GATT_SERVER)
#define CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE                        true
#else
#define CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE                        false
#endif  // CONFIG_BT_MESH_GATT_SERVER

/// Number of advertising buffers available for sending local messages.
/// This should be chosen based on the number of local messages that the node
/// can send simultaneously.
#define CONFIG_BT_MESH_ADV_BUF_COUNT                                6   // range 1 ~ 256

/// This option forces the usage of the local identity address for
/// all advertising. This can be a help for debugging (analyzing
/// traces), however it should never be enabled for a production
/// build as it compromises the privacy of the device.
#define CONFIG_BT_MESH_DEBUG_USE_ID_ADDR                            false
/* endmenu # BT_MESH_ADV_EXT */

/* menu "Beacons" */
/// Controls whether the Secure network beacon feature is enabled by
/// default. Can be changed through runtime configuration.
#define CONFIG_BT_MESH_BEACON_ENABLED                               true

/// Enable support for private beacons.
#define CONFIG_BT_MESH_PRIV_BEACONS                                 true
/* endmenu # Beacons */

/*menuconfig "BT_MESH_ACCESS_DELAYABLE_MSG"*/
/// Enable following of the message transmitting recommendations, the Access layer
/// specification. The recommendations are optional.
/// However, they are strictly recommended if the device participates in the network with
/// intensive communication where the device receives a lot of requests that require responses.
#define CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG                         true


/* menu "Provisioning" */
#if (CONFIG_BT_MESH_CDB)
/// This option specifies how many nodes each network can at most
/// save in the configuration database.
#define CONFIG_BT_MESH_CDB_NODE_COUNT                                 8   // range 1 ~ 4096

/// This option specifies how many subnets that can at most be
/// saved in the configuration database.
#define CONFIG_BT_MESH_CDB_SUBNET_COUNT                               1   // range 1 ~ 4096

/// This option specifies how many application keys that can at most
/// be saved in the configuration database.
#define CONFIG_BT_MESH_CDB_APP_KEY_COUNT                              1   // range 1 ~ 4096
#endif  // CONFIG_BT_MESH_CDB

/// This option specifies the interval (in seconds) at which the
/// device sends unprovisioned beacon.
#define CONFIG_BT_MESH_UNPROV_BEACON_INT                              5  // range 1 ~ 10

#if (CONFIG_BT_MESH_PB_ADV)
/// Use relay advertising sets to send provisioning PDUs
#if (CONFIG_BT_MESH_RELAY_ADV_SETS > 0)
#define CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS                          true
#else
#define CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS                          false
#endif  // CONFIG_BT_MESH_RELAY_ADV_SETS > 0

/// Controls the number of retransmissions of original Link Open and Transaction PDU,
/// in addition to the first transmission.
#if (CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS)
#define CONFIG_BT_MESH_PB_ADV_TRANS_PDU_RETRANSMIT_COUNT              7     // if CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS
#else
#define CONFIG_BT_MESH_PB_ADV_TRANS_PDU_RETRANSMIT_COUNT              0     // range 0 ~ 7
#endif  // CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS

/// Controls the number of retransmissions of original Link Ack and Transaction Acknowledgment PDU,
/// in addition to the first transmission.
#define CONFIG_BT_MESH_PB_ADV_TRANS_ACK_RETRANSMIT_COUNT              2     // range 0 ~ 7

/// Controls the number of retransmissions of original Link Close,
/// in addition to the first transmission.
#if (CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS)
#define CONFIG_BT_MESH_PB_ADV_LINK_CLOSE_RETRANSMIT_COUNT             7     // if CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS
#else
#define CONFIG_BT_MESH_PB_ADV_LINK_CLOSE_RETRANSMIT_COUNT             2     // range 0 ~ 7
#endif  // CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS

/// Timeout value of retransmit provisioning PDUs.
#define CONFIG_BT_MESH_PB_ADV_RETRANS_TIMEOUT                         500   // range 100 ~ 800
#endif  // CONFIG_BT_MESH_PB_ADV

#if (CONFIG_BT_CONN)
/// Enable this option to allow the device to be provisioned over GATT.
#define CONFIG_BT_MESH_PB_GATT                                        true

#if (CONFIG_BT_MESH_PB_GATT && (!CONFIG_BT_MESH_GATT_SERVER || !CONFIG_BT_MESH_PROV || !CONFIG_BT_MESH_PB_GATT_COMMON))
#error "BT_MESH_PB_GATT select BT_MESH_GATT_SERVER, BT_MESH_PROV, BT_MESH_PB_GATT_COMMON error!"
#endif // (CONFIG_BT_MESH_PB_GATT && (!CONFIG_BT_MESH_GATT_SERVER || !CONFIG_BT_MESH_PROV || !CONFIG_BT_MESH_PB_GATT_COMMON))

#if (CONFIG_BT_MESH_PB_GATT)
/// This option includes GAP device name in scan response when the PB-GATT is enabled.
#define CONFIG_BT_MESH_PB_GATT_USE_DEVICE_NAME                        true
#endif

/// Enable this option to allow the provisioner provisioning the
/// device over GATT.
#define CONFIG_BT_MESH_PB_GATT_CLIENT                                 true
#if (CONFIG_BT_MESH_PB_GATT_CLIENT && (!CONFIG_BT_MESH_GATT_CLIENT || !CONFIG_BT_MESH_PROV || !CONFIG_BT_MESH_PB_GATT_COMMON))
#error "BT_MESH_PB_GATT_CLIENT select BT_MESH_GATT_CLIENT, BT_MESH_PROV, BT_MESH_PB_GATT_COMMON error!"
#endif // (CONFIG_BT_MESH_PB_GATT_CLIENT && (!CONFIG_BT_MESH_GATT_CLIENT || !CONFIG_BT_MESH_PROV || !CONFIG_BT_MESH_PB_GATT_COMMON))

#endif  // CONFIG_BT_CONN

#if (CONFIG_BT_MESH_PB_ADV || CONFIG_BT_MESH_PB_GATT)
///  Enable this option to allow the device to be provisioned into a mesh network
#define CONFIG_BT_MESH_PROVISIONEE                                    true
#endif  // CONFIG_BT_MESH_PB_ADV || CONFIG_BT_MESH_PB_GATT

/// Enable this option if public key is to be exchanged via Out of Band (OOB) technology.
#if (CONFIG_BT_MESH_PROVISIONEE)
#define CONFIG_BT_MESH_PROV_OOB_PUBLIC_KEY                             true
#else
#define CONFIG_BT_MESH_PROV_OOB_PUBLIC_KEY                             false
#endif

#if (CONFIG_BT_MESH_PROV)
/// Enable this option to support CMAC AES128 for OOB authentication.
#define CONFIG_BT_MESH_ECDH_P256_CMAC_AES128_AES_CCM                  true
/// Enable this option to support HMAC SHA256 for OOB authentication.
#define CONFIG_BT_MESH_ECDH_P256_HMAC_SHA256_AES_CCM                  true
#endif  // CONFIG_BT_MESH_PROV

/// OOB authentication mandates to use HMAC SHA256
#if (CONFIG_BT_MESH_ECDH_P256_HMAC_SHA256_AES_CCM)
#define CONFIG_BT_MESH_OOB_AUTH_REQUIRED                              false
#endif  // CONFIG_BT_MESH_ECDH_P256_HMAC_SHA256_AES_CCM

/// Enable this option to have support for provisioning remote devices.
#if (CONFIG_BT_MESH_CDB && CONFIG_BT_MESH_PROV && (CONFIG_BT_MESH_PB_ADV || CONFIG_BT_MESH_PB_GATT_CLIENT))
#define CONFIG_BT_MESH_PROVISIONER                                    true
#else
#define CONFIG_BT_MESH_PROVISIONER                                    false
#endif    // (CONFIG_BT_MESH_CDB && CONFIG_BT_MESH_PROV && (CONFIG_BT_MESH_PB_ADV || CONFIG_BT_MESH_PB_GATT_CLIENT))
/* endmenu # Provisioning */


/* menu "Network layer" */
/// The number of buffers allocated for the network loopback mechanism.
/// Loopback is used when the device sends messages to itself.
#define CONFIG_BT_MESH_LOOPBACK_BUFS                      3

/// Controls the initial number of retransmissions of original messages,
///	in addition to the first transmission. Can be changed through runtime configuration.
#define CONFIG_BT_MESH_NETWORK_TRANSMIT_COUNT             2           // range 0 ~ 7

/// Controls the initial interval between retransmissions of original
///	messages, in milliseconds. Can be changed through runtime configuration.
#define CONFIG_BT_MESH_NETWORK_TRANSMIT_INTERVAL          20          // range 10 ~ 330

/// Number of messages that are cached by the node to avoid acting on the
///	recently seen duplicate messages. This option is similar to
///	the replay protection list, but has a different purpose. Network message
///	cache helps prevent unnecessary decryption operations. This also prevents
///	unnecessary relaying and helps in getting rid of relay loops. Setting
///	this value to a very low number can cause unnecessary network traffic.
///	Setting this value to a very large number can impact the processing time
/// for each received network PDU and increases RAM footprint proportionately.
#define CONFIG_BT_MESH_MSG_CACHE_SIZE                     32          // range 2 ~ 65535

/* menuconfig BT_MESH_RELAY */
#if CONFIG_BT_MESH_RELAY
/// Controls whether the Relay feature is enabled by default when the
///	device boots up for the first time or unprovisioned. Can be changed
///	at runtime using bt_mesh_relay_set() function.
#define CONFIG_BT_MESH_RELAY_ENABLED                      1

/// Controls the initial number of retransmissions of relayed messages, in
/// addition to the first transmission. Can be changed through runtime configuration.
#define CONFIG_BT_MESH_RELAY_RETRANSMIT_COUNT             2           // range 0 ~ 7

/// Controls the initial interval between retransmissions of relayed
/// messages, in milliseconds. Can be changed through runtime configuration.
#define CONFIG_BT_MESH_RELAY_RETRANSMIT_INTERVAL          20          // range 10 ~ 330

/// Number of advertising buffers available for messages to be relayed.
/// High number of advertising buffers increases the reliability of the
/// mesh network. Low number of advertising buffers reduces the message
/// latency on the Relay Node, but at the same time increases the amount
/// of packet drops. When considering the message latency, also consider
/// the values of BT_MESH_RELAY_RETRANSMIT_COUNT and BT_MESH_RELAY_RETRANSMIT_INTERVAL.
/// A higher number of BT_MESH_RELAY_ADV_SETS allows the increase in the number of buffers
///	while maintaining the latency.
#define CONFIG_BT_MESH_RELAY_BUF_COUNT                     32         // range 1 ~ 256
#endif // CONFIG_BT_MESH_RELAY

/* endmenu # Network layer */

/* menu "Transport layer" */
/// Maximum number of simultaneous outgoing multi-segment and/or reliable messages.

///	Note that: Since Mesh Segmentation/reassembling is a mandatory
///	feature of specification, set to zero will not allow send any
///	Mesh Segment message.
#define CONFIG_BT_MESH_TX_SEG_MSG_COUNT                     3         // range 0 ~ 255

/// Maximum number of simultaneous incoming multi-segment and/or reliable messages.

/// Note that: Since Mesh Segmentation/reassemblying is a mandatory
///	feature of specification, set to zero will not allow receive any Mesh Segment message.
#define CONFIG_BT_MESH_RX_SEG_MSG_COUNT                     3         // range 0 ~ 255

/// The incoming and outgoing segmented messages allocate their
///	segments from the same pool. Each segment is a 12 byte block,
///	and may only be used by one message at the time.

///	Outgoing messages will allocate their segments at the start of the
///	transmission, and release them one by one as soon as they have been
///	acknowledged by the receiver. Incoming messages allocate all their
///	segments at the start of the transaction, and won't release them until
///	the message is fully received.
#define CONFIG_BT_MESH_SEG_BUFS                             64        // range BT_MESH_RX_SEG_MAX 16384 if BT_MESH_RX_SEG_MAX > BT_MESH_TX_SEG_MAX
                                                                      // range BT_MESH_TX_SEG_MAX 16384

#if (CONFIG_BT_MESH_RX_SEG_MSG_COUNT > 0)
/// Maximum number of segments supported for incoming messages.
/// This value should typically be fine-tuned based on what
/// models the local node supports, i.e. what's the largest
/// message payload that the node needs to be able to receive.
/// This value affects memory and call stack consumption, which
/// is why the default is lower than the maximum that the
/// specification would allow (32 segments).

/// The maximum incoming SDU size is 12 times this number (out of
/// which 4 or 8 bytes is used for the Transport Layer MIC). For
/// example, 5 segments means the maximum SDU size is 60 bytes,
/// which leaves 56 bytes for application layer data using a
/// 4-byte MIC and 52 bytes using an 8-byte MIC.
#define CONFIG_BT_MESH_RX_SEG_MAX                           20         // range 1 ~ 32
#endif // CONFIG_BT_MESH_RX_SEG_MSG_COUNT

#if (CONFIG_BT_MESH_TX_SEG_MSG_COUNT > 0)
/// Maximum number of segments supported for outgoing messages.
/// This value should typically be fine-tuned based on what
/// models the local node supports, i.e. what's the largest
/// message payload that the node needs to be able to send.
/// This value affects memory consumption, which is why the
/// default is lower than the maximum that the specification would allow (32 segments).

/// The maximum outgoing SDU size is 12 times this number (out of
/// which 4 or 8 bytes is used for the Transport Layer MIC). For
/// example, 5 segments means the maximum SDU size is 60 bytes,
/// which leaves 56 bytes for application layer data using a
/// 4-byte MIC and 52 bytes using an 8-byte MIC.
#define CONFIG_BT_MESH_TX_SEG_MAX                           6         // range 1 ~ 32
#endif // CONFIG_BT_MESH_TX_SEG_MSG_COUNT

/// This value controls the interval between sending two consecutive
/// segments in a segmented message. The interval is measured in
/// milliseconds and calculated using the following formula:
/// (CONFIG_BT_MESH_SAR_TX_SEG_INT_STEP + 1) * 10 ms.
#define CONFIG_BT_MESH_SAR_TX_SEG_INT_STEP                  0x05      // range 0x00 ~ 0x0F

/// This value controls the maximum number of retransmissions of a
/// segmented message to a unicast address before giving up the transfer.
#define CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_COUNT         0x02      // range 0x00 ~ 0x0F

/// This value defines the maximum number of retransmissions of a
///	segmented message to a unicast address that the stack will send if no
/// acknowledgment was received during timeout, or if an
/// acknowledgment with already confirmed segments was received.
#define CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_WITHOUT_PROG_COUNT      0x02      // range 0x00 ~ 0x0F

/// This value controls the interval step used for delaying the
/// retransmissions of unacknowledged segments of a segmented message to
/// a unicast address. The interval step is measured in milliseconds and
/// calculated using the following formula:
/// (CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_INT_STEP + 1) * 25 ms.
#define CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_INT_STEP                0x07      // range 0x00 ~ 0x0F

/// This value controls the interval increment used for delaying the
/// retransmissions of unacknowledged segments of a segmented message to
/// a unicast address. The increment is measured in milliseconds and
/// calculated using the following formula:
/// (CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_INT_INC + 1) * 25 ms.
#define CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_INT_INC                 0x01      // range 0x00 ~ 0x0F

/// This value controls the total number of retransmissions of a segmented
///	message to a multicast address.
#define CONFIG_BT_MESH_SAR_TX_MULTICAST_RETRANS_COUNT                 0x02      // range 0x00 ~ 0x0F

/// This value controls the interval between retransmissions of all
/// segments in a segmented message to a multicast address. The
/// interval is measured in milliseconds and calculated using the
/// following formula:
/// (CONFIG_BT_MESH_SAR_TX_MULTICAST_RETRANS_INT + 1) * 25 ms.
#define CONFIG_BT_MESH_SAR_TX_MULTICAST_RETRANS_INT                   0x09      // range 0x00 ~ 0x0F

/// This value defines a threshold in number of segments of a segmented
/// message for acknowledgment retransmissions. When the number of
/// segments of a segmented message is above this threshold, the stack
/// will additionally retransmit every acknowledgment message the
/// number of times given by value of CONFIG_BT_MESH_SAR_RX_ACK_RETRANS_COUNT.
#define CONFIG_BT_MESH_SAR_RX_SEG_THRESHOLD                           0x03      // range 0x00 ~ 0x1F

/// This value controls the delay increment of an interval used for
/// delaying the transmission of an acknowledgment message after
/// receiving a new segment. The increment is measured in segments
/// and calculated using the following formula:
/// CONFIG_BT_MESH_SAR_RX_ACK_DELAY_INC + 1.5.
#define CONFIG_BT_MESH_SAR_RX_ACK_DELAY_INC                           0x01      // range 0x00 ~ 0x07

/// This value defines the segments reception interval step used for
/// delaying the transmission of an acknowledgment message after
/// receiving a new segmnet. The interval is measured in milliseconds
/// and calculated using the following formula:
/// (CONFIG_BT_MESH_SAR_RX_SEG_INT_STEP + 1) * 10 ms
#define CONFIG_BT_MESH_SAR_RX_SEG_INT_STEP                            0x05      // range 0x00 ~ 0x0F

/// This value defines the time since the last successfully received
/// segment before giving up the reception of a segmented message.
#define CONFIG_BT_MESH_SAR_RX_DISCARD_TIMEOUT                         0x01      // range 0x00 ~ 0x0F

/// This value defines the total number of retranmissions of an
/// acknowledgment message that the stack will additionally send when the
/// size of segments in a segmented message is above the
/// CONFIG_BT_MESH_SAR_RX_SEG_THRESHOLD value.
#define CONFIG_BT_MESH_SAR_RX_ACK_RETRANS_COUNT                       0x00      // range 0x00 ~ 0x03

/// Controls the default TTL value for outgoing messages. Can be changed
/// through runtime configuration.
#define CONFIG_BT_MESH_DEFAULT_TTL                                    0x07      // range 0 ~ 128

/// This options specifies the maximum capacity of the replay
/// protection list. This option is similar to the network message
/// cache size, but has a different purpose.
#define CONFIG_BT_MESH_CRPL                                           10        // range 2 ~ 65535

/* endmenu # Transport layer */

/* menu "Access layer" */
/// This option allows the application to directly access
/// Bluetooth access layer messages without the need to
/// instantiate Bluetooth Mesh models.

/// Please note that Bluetooth Mesh stack stores authentication sequence from
/// any message that passed over network cache if this option is enabled.
/// It happens despite the device does not have necessary application keys or
/// there are unknown model operation codes. It causes the situation when device
/// will update the replay protection cache for messages those cannot be handled
/// in the stack. For example, spamming broadcast messages those stack cannot handle
/// might wear out persistent memory.
#define CONFIG_BT_MESH_ACCESS_LAYER_MSG                             true

/// This option forces vendor model to use messages for the corresponding CID field.
#define CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE                      true

/// Enable support for the model extension concept, allowing the Access
/// layer to know about mesh model relationships.
#define CONFIG_BT_MESH_MODEL_EXTENSIONS                             true

/// Enable support for Composition Data Page 1.
#define CONFIG_BT_MESH_COMP_PAGE_1                                  true

#if (CONFIG_BT_MESH_COMP_PAGE_1)
/// This option specifies how many models relations can be saved.
/// Equals to the number of `bt_mesh_model_extend` and `bt_mesh_model_correspond` calls.
/// This information is used to construct Composition Data Page 1.

// range 0 ~ 255
#if (CONFIG_BT_MESH_MODEL_EXTENSIONS == false)
#define CONFIG_BT_MESH_MODEL_EXTENSION_LIST_SIZE                    0
#else
#define CONFIG_BT_MESH_MODEL_EXTENSION_LIST_SIZE                    10
#endif  // CONFIG_BT_MESH_MODEL_EXTENSIONS
#endif  // CONFIG_BT_MESH_COMP_PAGE_1

/// Enable support for Composition Data Page 2.
#define CONFIG_BT_MESH_COMP_PAGE_2                                  true

/// Stack allocated buffer used to temporarily hold Composition
/// Data Pages during flash operations. Should reflect the size
/// of the largest Composition Data Page present in the application.
/// Note that this buffer should still be large enough to restore previously stored
/// pages after a performed device firmware update.
#define CONFIG_BT_MESH_COMP_PST_BUF_SIZE                            100

#if (CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG)
/// Controls whether the delayable message feature is enabled by default in
/// the message context of the opcode notifications. This allows the server part of any
/// model to not bother about additional context configuration to enable the delayable message.
/// Note that if this is disabled then all foundation models stop using the delayable message
/// functionality.
#define CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CTX_ENABLED             true

/// The maximum number of messages the Access layer can manage to delay
/// at the same time. The number of messages can be handled only if the Access layer
/// has a sufficient amount of memory to store the model payload for them.
#define CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_COUNT                   4

/// Size of memory that Access layer uses to split model message to. It allocates
/// a sufficient number of these chunks from the pool to store the full model payload.
#define CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE              10

/// The maximum number of available chunks the Access layer allocates to store model payload.
/// It is recommended to keep chunk size equal to the reasonable small value to prevent
/// unnecessary memory wasting.
#define CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_COUNT             40
#endif // CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG

/// When enabled, the periodic publications are randomly delayed by 20 to 50ms. Publications
/// triggered at the start of the stack or by the bt_mesh_model_publish() call are delayed by
/// 20 to 500ms. This option reduces the probability of collisions when multiple nodes publish
/// at the same time.
#define CONFIG_BT_MESH_DELAYABLE_PUBLICATION                        true
/* endmenu # Access layer */

/* menu "Models" */
/// Enable support for the configuration client model.
#define CONFIG_BT_MESH_CFG_CLI                                      true
/// Enable support for the health client model.
#define CONFIG_BT_MESH_HEALTH_CLI                                   true

/*menuconfig "BT_MESH_BLOB_SRV"*/
/// Enable the Binary Large Object (BLOB) Transfer Server model.
#define CONFIG_BT_MESH_BLOB_SRV                                     true
/*menuconfig BT_MESH_BLOB_CLI*/
/// Enable the Binary Large Object (BLOB) Transfer Client model.
#define CONFIG_BT_MESH_BLOB_CLI                                     true
#if (CONFIG_BT_MESH_MODEL_EXTENSIONS && CONFIG_BT_MESH_BLOB_SRV)
/// Enable the Firmware Update Server model.
#define CONFIG_BT_MESH_DFU_SRV                                      true
#endif
#if (CONFIG_BT_MESH_MODEL_EXTENSIONS && CONFIG_BT_MESH_BLOB_CLI)
/// Enable the Firmware Update Client model.
#define CONFIG_BT_MESH_DFU_CLI                                      true
#if (CONFIG_BT_MESH_DFU_CLI)
/*menuconfig BT_MESH_DFU_SLOTS*/
/// Enable the DFU image slot manager, for managing firmware distribution slots
/// for the Firmware Update Client model.
#define CONFIG_BT_MESH_DFU_SLOTS                                    true
#endif
#endif

#if (CONFIG_BT_MESH_BLOB_SRV && CONFIG_BT_MESH_DFU_CLI)
/*menuconfig BT_MESH_DFD_SRV*/
/// Enable the Firmware Distribution Server model.
#define CONFIG_BT_MESH_DFD_SRV                                      true
#endif  // CONFIG_BT_MESH_BLOB_SRV && CONFIG_BT_MESH_DFU_CLI

/// The Remote Provisioning Server is the proxy for a provisioning
/// process, allowing provisioners to tunnel their provisioning
/// messages through the mesh to the Remote Provisioning Server, which
/// communicates directly with the unprovisioned node.
#define CONFIG_BT_MESH_RPR_SRV                                      true

#if (CONFIG_BT_MESH_PROVISIONER)
/// The Remote Provisioning Client is instantiated on the provisioner
///	node, and allows provisioning of new devices through the mesh network
///	by tunnelling provisioning messages to a Remote Provisioning Server.
#define CONFIG_BT_MESH_RPR_CLI                                      true
#else
#define CONFIG_BT_MESH_RPR_CLI                                      false
#endif  // CONFIG_BT_MESH_PROVISIONER

#define CONFIG_BT_MESH_SAR_CFG                                      true

/// Enable support for the SAR configuration server model.
#define CONFIG_BT_MESH_SAR_CFG_SRV                                  true

/// Enable support for the SAR configuration client model.
#define CONFIG_BT_MESH_SAR_CFG_CLI                                  true

#if ((CONFIG_BT_MESH_SAR_CFG_SRV || CONFIG_BT_MESH_SAR_CFG_CLI) && !CONFIG_BT_MESH_SAR_CFG)
#error "CONFIG_BT_MESH_SAR_CFG_SRV or CONFIG_BT_MESH_SAR_CFG_CLI select BT_MESH_SAR_CFG"
#endif  // ((CONFIG_BT_MESH_SAR_CFG_SRV || CONFIG_BT_MESH_SAR_CFG_CLI) && !CONFIG_BT_MESH_SAR_CFG)

#define CONFIG_BT_MESH_OP_AGG                                       true

/// Enable support for the Opcode Aggregator Server model.
#define CONFIG_BT_MESH_OP_AGG_SRV                                   true

/// Enable support for the Opcode Aggregator Client model.
#define CONFIG_BT_MESH_OP_AGG_CLI                                   true

/// On-Demand Private Proxy Client allows to configure and check the state
/// of On-Demand Private Proxy Servers. The state determines if the peers will
/// advertise the Private Network Identity type after receiving a Solicitation PDU.
#define CONFIG_BT_MESH_OD_PRIV_PROXY_CLI                            true

#if ((CONFIG_BT_MESH_OP_AGG_SRV || CONFIG_BT_MESH_OP_AGG_CLI) && !CONFIG_BT_MESH_OP_AGG)
#error "CONFIG_BT_MESH_OP_AGG_SRV or CONFIG_BT_MESH_OP_AGG_CLI select CONFIG_BT_MESH_OP_AGG"
#endif  // ((CONFIG_BT_MESH_OP_AGG_SRV || CONFIG_BT_MESH_OP_AGG_CLI) && !CONFIG_BT_MESH_OP_AGG)

/// Enable support for the Large Composition Data Server model.
#define CONFIG_BT_MESH_LARGE_COMP_DATA_SRV                          true

/// Enable support for the Large Composition Data Client model.
#define CONFIG_BT_MESH_LARGE_COMP_DATA_CLI                          true

#if (CONFIG_BT_MESH_PRIV_BEACONS)
/// Enable support for the Private Beacon Server model.
#define CONFIG_BT_MESH_PRIV_BEACON_SRV                              true

/// Enable support for the Private Beacon Client model.
#define CONFIG_BT_MESH_PRIV_BEACON_CLI                              true
#endif  // CONFIG_BT_MESH_PRIV_BEACONS

#if (CONFIG_BT_MESH_PRIV_BEACON_SRV)
/// The On-Demand Private Proxy Server is used to support configuration of
/// advertising with Private Network Identity type of a node.
/// When enabled, the Solicitation PDU RPL Configuration Server model is also enabled.
#define CONFIG_BT_MESH_OD_PRIV_PROXY_SRV                            true
#else
#define CONFIG_BT_MESH_OD_PRIV_PROXY_SRV                            false
#endif  //  CONFIG_BT_MESH_PRIV_BEACON_SRV

/// The Solicitation PDU RPL Configuration Client is used to support the
/// functionality of removing addresses from the solicitation replay
/// protection list (SRPL) of a node that supports the Solicitation
/// PDU RPL Configuration Server model.
#define CONFIG_BT_MESH_SOL_PDU_RPL_CLI                              true

#if (CONFIG_BT_MESH_CFG_CLI)
/// This timeout controls how long config client waits for a response
/// message to arrive. This value can be changed at runtime using
/// @ref bt_mesh_cfg_cli_timeout_set.
#define CONFIG_BT_MESH_CFG_CLI_TIMEOUT                              5000
#endif  // CONFIG_BT_MESH_CFG_CLI

#if (CONFIG_BT_MESH_HEALTH_CLI)
/// This timeout controls how long health client waits for a response
/// message to arrive. This value can be changed at runtime using
/// @ref bt_mesh_health_cli_timeout_set.
#define CONFIG_BT_MESH_HEALTH_CLI_TIMEOUT                           2000
#endif  // CONFIG_BT_MESH_HEALTH_CLI

/// Align chunk size to max segmented message size
#define CONFIG_BT_MESH_ALIGN_CHUNK_SIZE_TO_MAX_SEGMENT              true

#if (CONFIG_BT_MESH_BLOB_SRV)
/// In Pull mode (Pull BLOB Transfer Mode), the BLOB Transfer Server
/// requests a fixed number of chunks from the Client. Use this option to
/// control the chunk count in the request. If the BLOB Transfer Server
/// is instantiated on a Low Power node, the pull request count will be
/// trimmed to not overflow the Friend queue.
#define CONFIG_BT_MESH_BLOB_SRV_PULL_REQ_COUNT                      4         // range 1 ~ 16

/// The maximum object size a BLOB Transfer Server can receive.
#define CONFIG_BT_MESH_BLOB_SIZE_MAX                                524288    // range 1 ~ 3257617792

/// Minimum acceptable block size in a BLOB transfer. The transfer block
/// size will be some number that is a power of two, and is between block
/// size min and block size max. If no such number exists, a compile
/// time warning will be issued.
#define CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MIN                          4096      // range 64 ~ 1048576 # 2^6 - 2^20

/// Maximum acceptable block size in a BLOB transfer. The transfer block
/// size will be some number that is a power of two, and is between block
/// size min and block size max. If no such number exists, a compile
/// time warning will be issued.
#define CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX                          4096      // range BT_MESH_BLOB_BLOCK_SIZE_MIN 1048576

/// The timer value that Pull BLOB Transfer Server uses to report missed chunks.
#define CONFIG_BT_MESH_BLOB_REPORT_TIMEOUT                          10        // range 1 ~ 31

#if (!CONFIG_BT_MESH_ALIGN_CHUNK_SIZE_TO_MAX_SEGMENT)
/// Set the chunk size for the BLOB Server.
/// The actual maximum chunk size depends on how many segments are
/// possible and will be clamped to the max possible if set above.
/// see also: BT_MESH_RX_SEG_MAX,
/// and the maximum SDU a node can receive.
#define CONFIG_BT_MESH_RX_BLOB_CHUNK_SIZE                           8         // range 8 ~ 377
#endif // !CONFIG_BT_MESH_ALIGN_CHUNK_SIZE_TO_MAX_SEGMENT
#endif // CONFIG_BT_MESH_BLOB_SRV

#if (CONFIG_BT_MESH_BLOB_CLI)
/// Controls the number of times the client will attempt to resend missing
///	chunks to the BLOB receivers for every block.
#define CONFIG_BT_MESH_BLOB_CLI_BLOCK_RETRIES                       5

#if (!CONFIG_BT_MESH_ALIGN_CHUNK_SIZE_TO_MAX_SEGMENT)
/// Set the chunk size for the BLOB Client.
/// The actual maximum chunk size depends on how many segments are
/// possible and will be clamped to the max possible if set above.
/// see also: BT_MESH_TX_SEG_MAX,
/// and the maximum SDU a node can receive.
#define CONFIG_BT_MESH_TX_BLOB_CHUNK_SIZE                           8         // range 1 ~ 377
#endif // !CONFIG_BT_MESH_ALIGN_CHUNK_SIZE_TO_MAX_SEGMENT
/// Set the interval in milliseconds in which chunks are sent during the BLOB transfer.
/// Note: Without a delay between each sent chunk, the network might become too busy with the
/// BLOB transfer for other communications to succeed.
/// Note: Timing restrictions, like the timeout base, should be considered or changed
/// accordingly when setting this interval. Otherwise, the interval might be too big for the
/// timeout settings and cause timeouts.
#define CONFIG_BT_MESH_TX_BLOB_CHUNK_SEND_INTERVAL                         0         // range 0 ~ 2147483647
#endif  // CONFIG_BT_MESH_BLOB_CLI

/// A BLOB transfer contains several blocks. Each block is made up of
/// several chunks. This option controls the maximum chunk count per
/// block.
#define CONFIG_BT_MESH_BLOB_CHUNK_COUNT_MAX                         256       // range 1 ~ 2992

/* menu "Firmware Update model configuration" */
/// This value defines the maximum length of an image's firmware ID.
#define CONFIG_BT_MESH_DFU_FWID_MAXLEN                              16        // range 0 ~ 106

/// This value defines the maximum length of an image's metadata.
#define CONFIG_BT_MESH_DFU_METADATA_MAXLEN                          32        // range 18 ~ 255 if BT_MESH_DFU_METADATA
                                                                              // range 0 ~ 255
/// This option adds a set of functions that can be used to encode and decode a firmware
/// metadata using the format defined in the Bluetooth Mesh DFU subsystem.
#define CONFIG_BT_MESH_DFU_METADATA                                 true

/// This value defines the maximum length of an image's URI, not including
/// a string terminator.
#define CONFIG_BT_MESH_DFU_URI_MAXLEN                               32        // range 0 ~ 255

/* endmenu # Firmware Update model configuration */

#if (CONFIG_BT_MESH_DFU_SLOTS)
/// This value defines the number of firmware slots the DFU image slot manager
/// can keep simultaneously.
#define CONFIG_BT_MESH_DFU_SLOT_CNT                                 3         // range 1 ~ 32767
#endif

#if (CONFIG_BT_MESH_DFD_SRV)
/// This value defines the largest DFU image a single slot can store.
#define CONFIG_BT_MESH_DFD_SRV_SLOT_MAX_SIZE                        CONFIG_BT_MESH_BLOB_SIZE_MAX      // range 0 ~ BT_MESH_BLOB_SIZE_MAX
/// This value defines the total storage space dedicated to storing DFU
///	images on the Firmware Distribution Server.
#define CONFIG_BT_MESH_DFD_SRV_SLOT_SPACE                           CONFIG_BT_MESH_DFD_SRV_SLOT_MAX_SIZE  // range 0 ~ 4294967295
/// This value defines the maximum number of Target nodes the Firmware
/// Distribution Server can target simultaneously.
#define CONFIG_BT_MESH_DFD_SRV_TARGETS_MAX                          8         // range 1 ~ 65535
/// This enables support for OOB upload of firmware images for
///	distribution. This makes several callbacks and use of the init
///	macro BT_MESH_DFD_SRV_INIT_OOB mandatory. See the API documentation
///	for bt_mesh_dfd_srv_cb for details about the mandatory callbacks.
#define CONFIG_BT_MESH_DFD_SRV_OOB_UPLOAD                           true
#endif    // CONFIG_BT_MESH_DFD_SRV

/* menu "Remote Provisioning configuration" */
/// During extended scanning, the Remote Provisioning Server can include
/// a set of AD types in the scan reports, collected from the
/// unprovisioned device's advertisement data. This option controls
/// the highest number of AD types a single server can scan for, and a
/// Client can request.
#define CONFIG_BT_MESH_RPR_AD_TYPES_MAX                             8   // range 1 ~ 16
/// Max number of unique unprovisioned devices a single Remote
/// Provisioning Server can hold.
#define CONFIG_BT_MESH_RPR_SRV_SCANNED_ITEMS_MAX                    4   // range 4 ~ 255
/// Buffer size for the additional advertisement data reported during extended scanning.
#define CONFIG_BT_MESH_RPR_SRV_AD_DATA_MAX                          31  // range 3 ~ 255
/* endmenu # Remote Provisioning configuration */

#if (CONFIG_BT_MESH_OP_AGG_CLI)
/// This timeout controls how long Opcodes Aggregator Client waits
///	for a response message to arrive. This value can be changed at
/// runtime using @ref bt_mesh_op_agg_cli_timeout_set.
#define CONFIG_BT_MESH_OP_AGG_CLI_TIMEOUT                           10000
#endif  // CONFIG_BT_MESH_OP_AGG_CLI

#if (CONFIG_BT_MESH_LARGE_COMP_DATA_SRV)
/// This value is the combined total metadata length for
/// all models on the device.
#define CONFIG_BT_MESH_MODELS_METADATA_PAGE_LEN                     150
#endif  //  CONFIG_BT_MESH_LARGE_COMP_DATA_SRV

#if (CONFIG_BT_MESH_OD_PRIV_PROXY_CLI)
/// This timeout controls how long the On-Demand Private Proxy Client waits
/// for a response message to arrive. This value can be changed at runtime
/// using @ref bt_mesh_od_priv_proxy_cli_timeout_set.
#define CONFIG_BT_MESH_OD_PRIV_PROXY_CLI_TIMEOUT                    5000
#endif // CONFIG_BT_MESH_OD_PRIV_PROXY_CLI

#if (CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
/// Size of SRPL(solicitation replay protection list). The list is used to determine if a received Solicitation PDU
///	is valid. It is valid when the SSRC field value of the received Solicitation PDU
///	is stored in the SRPL and the SSEQ field value is bigger than the corresponding
///	stored SSEQ value, or if the SSRC was not stored in the RPL and the SRPL still has
///	space for new entries.
#define CONFIG_BT_MESH_PROXY_SRPL_SIZE                              10    // range 1 ~ 255
#endif  // CONFIG_BT_MESH_OD_PRIV_PROXY_SRV

#if (CONFIG_BT_MESH_SOL_PDU_RPL_CLI)
/// This timeout controls how long Solicitation PDU RPL Configuration Client waits
/// for a response message to arrive. This value can be changed at runtime
/// using @ref bt_mesh_sol_pdu_rpl_cli_timeout_set.
#define CONFIG_BT_MESH_SOL_PDU_RPL_CLI_TIMEOUT                      5000
#endif

#if (CONFIG_BT_MESH_MODEL_EXTENSIONS)
/// The Bridge Configuration Server model is used to support the configuration
/// of the subnet bridge functionality of a node.
#define CONFIG_BT_MESH_BRG_CFG_SRV                                  true
#if (CONFIG_BT_MESH_BRG_CFG_SRV)
/// The maximum number of entries in the bridging table.
#define CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX                          16    // range 16 ~ 255
#endif // CONFIG_BT_MESH_BRG_CFG_SRV
#endif // CONFIG_BT_MESH_MODEL_EXTENSIONS

/// The Bridge Configuration Client is used to support the functionality of a
/// node that can configure the subnet bridge functionality of another node.
#define CONFIG_BT_MESH_BRG_CFG_CLI                                  true
#if (CONFIG_BT_MESH_BRG_CFG_CLI)
/// This timeout controls how long the bridge configuration client waits for a
/// response message to arrive. This value can be changed at runtime using
/// @ref bt_mesh_brg_cfg_cli_timeout_set.
#define CONFIG_BT_MESH_BRG_CFG_CLI_TIMEOUT                          5000
#endif  // CONFIG_BT_MESH_BRG_CFG_CLI
/* endmenu # Models */

/* menu "Proxy" */
/// This option enables support for the Mesh GATT Proxy Client,
/// i.e. the ability to act as a proxy between a Mesh GATT Service
/// and a Mesh network.
#define CONFIG_BT_MESH_PROXY_CLIENT                                 true

#if (CONFIG_BT_MESH_PROXY_CLIENT && !CONFIG_BT_MESH_GATT_CLIENT)
#error "CONFIG_BT_MESH_PROXY_CLIENT and !CONFIG_BT_MESH_GATT_CLIENT"
#endif  //(CONFIG_BT_MESH_PROXY_CLIENT && !CONFIG_BT_MESH_GATT_CLIENT)

/// This option enables support for sending Solicitation PDUs.
#define CONFIG_BT_MESH_PROXY_SOLICITATION                           true

#if (CONFIG_BT_MESH_OD_PRIV_PROXY_SRV || CONFIG_BT_MESH_PROXY_SOLICITATION)
#define CONFIG_BT_MESH_SOLICITATION                                 true    // auto select
#endif    // CONFIG_BT_MESH_OD_PRIV_PROXY_SRV || CONFIG_BT_MESH_PROXY_SOLICITATION

#if (CONFIG_BT_MESH_GATT_PROXY)
/// Controls whether the GATT Proxy feature is enabled by default.
/// Can be changed through runtime configuration.
#define CONFIG_BT_MESH_GATT_PROXY_ENABLED                           true

/// This option determines for how long the local node advertises
/// using Node Identity. The given value is in seconds. The
/// specification limits this to 60 seconds, and implies that to
/// be the appropriate value as well, so just leaving this as the
/// default is the safest option.
#define CONFIG_BT_MESH_NODE_ID_TIMEOUT                              60    // range 1 ~ 60

/// This option includes GAP device name in scan response when
/// the GATT Proxy feature is enabled.
#define CONFIG_BT_MESH_PROXY_USE_DEVICE_NAME                        true

/// This option specifies how many Proxy Filter entries the local
/// node supports. This helps in reducing unwanted traffic getting sent to
/// the proxy client. This value is application specific and should be large
/// enough so that proxy client can communicate with several devices through
/// this proxy server node using the default accept list filter type.
#define CONFIG_BT_MESH_PROXY_FILTER_SIZE                            16    // range 1 ~ 32767
#endif    // CONFIG_BT_MESH_GATT_PROXY

#if (CONFIG_BT_MESH_PROXY_SOLICITATION)
/// How many times Solicitation PDU advertisements will be repeated. 0 means that there will be
/// 1 transmission without retransmissions.
#define CONFIG_BT_MESH_SOL_ADV_XMIT                                 2     // range 0 ~ 10
#endif  // CONFIG_BT_MESH_PROXY_SOLICITATION
/* endmenu # Proxy */

/* menu "IV Index & Sequence number" */
/// This option removes the 96 hour limit of the IV Update
/// Procedure and lets the state be changed at any time.
#define CONFIG_BT_MESH_IV_UPDATE_TEST                               true

/// This option specifies the sequence number value to start iv update.
#define CONFIG_BT_MESH_IV_UPDATE_SEQ_LIMIT                          0x800000    // range 0x000001 ~ 0xFFFFFE

/// When the IV Update state enters Normal operation or IV Update
/// in Progress, we need to keep track of how many hours has passed
/// in the state, since the specification requires us to remain in
/// the state at least for 96 hours (Update in Progress has an
/// additional upper limit of 144 hours).

/// In order to fulfill the above requirement, even if the node might
/// be powered off once in a while, we need to store persistently
/// how many hours the node has been in the state. This doesn't
/// necessarily need to happen every hour (thanks to the flexible
/// duration range). The exact cadence will depend a lot on the
/// ways that the node will be used and what kind of power source it
/// has.

/// Since there is no single optimal answer, this configuration
/// option allows specifying a divider, i.e. how many intervals
/// the 96 hour minimum gets split into. After each interval the
/// duration that the node has been in the current state gets
/// stored to flash. E.g. the default value of 4 means that the
/// state is saved every 24 hours (96 / 4).
#define CONFIG_BT_MESH_IVU_DIVIDER                                  4     // range 2 ~ 96
/* endmenu # IV Index & Sequence number */

#if (CONFIG_BT_MESH_LOW_POWER)
/// Perform the Friendship establishment using low power, with
/// the help of a reduced scan duty cycle. The downside of this
/// is that the node may miss out on messages intended for it
/// until it has successfully set up Friendship with a Friend
/// node.
#define CONFIG_BT_MESH_LPN_ESTABLISHMENT                            true

/// Automatically enable LPN functionality once provisioned and start
/// looking for Friend nodes. If this option is disabled LPN mode
/// needs to be manually enabled by calling bt_mesh_lpn_set(true).
#define CONFIG_BT_MESH_LPN_AUTO                                     true
#if (CONFIG_BT_MESH_LPN_AUTO)
/// Time in seconds from the last received message, that the node
/// will wait before starting to look for Friend nodes.
#define CONFIG_BT_MESH_LPN_AUTO_TIMEOUT                             15    // range 0 ~ 3600
#endif    // CONFIG_BT_MESH_LPN_AUTO

/// Time in seconds between Friend Requests, if a previous Friend
/// Request did not receive any acceptable Friend Offers.
#define CONFIG_BT_MESH_LPN_RETRY_TIMEOUT                            8     // range 1 ~ 3600

/// The contribution of the RSSI measured by the Friend node used
/// in Friend Offer Delay calculations. 0 = 1, 1 = 1.5, 2 = 2, 3 = 2.5.
#define CONFIG_BT_MESH_LPN_RSSI_FACTOR                              0     // range 0 ~ 3

/// The contribution of the supported Receive Window used in
/// Friend Offer Delay calculations. 0 = 1, 1 = 1.5, 2 = 2, 3 = 2.5.
#define CONFIG_BT_MESH_LPN_RECV_WIN_FACTOR                          0     // range 0 ~ 3

/// The MinQueueSizeLog field is defined as log_2(N), where N is
/// the minimum number of maximum size Lower Transport PDUs that
/// the Friend node can store in its Friend Queue. As an example,
/// MinQueueSizeLog value 1 gives N = 2, and value 7 gives N = 128.
#define CONFIG_BT_MESH_LPN_MIN_QUEUE_SIZE                           1     // range 1 ~ 7

/// The ReceiveDelay is the time between the Low Power node
/// sending a request and listening for a response. This delay
/// allows the Friend node time to prepare the response. The value
/// is in units of milliseconds. When BT_MESH_ADV_LEGACY is used,
/// the minimal value for the delay can not be less than 50ms due
/// to a limitation in the legacy advertiser implementation.
#define CONFIG_BT_MESH_LPN_RECV_DELAY                               100   // range 50 ~ 255 if BT_MESH_ADV_LEGACY
                                                                          // range 10 ~ 255

/// PollTimeout timer is used to measure time between two
/// consecutive requests sent by the Low Power node. If no
/// requests are received by the Friend node before the
/// PollTimeout timer expires, then the friendship is considered
/// terminated. The value is in units of 100 milliseconds, so e.g.
/// a value of 300 means 30 seconds.
#define CONFIG_BT_MESH_LPN_POLL_TIMEOUT                             300   // range 10 ~ 244735

/// The initial value of the PollTimeout timer when Friendship
/// gets established for the first time. After this the timeout
/// will gradually grow toward the actual PollTimeout, doubling
/// in value for each iteration. The value is in units of 100
/// milliseconds, so e.g. a value of 300 means 30 seconds.
#define CONFIG_BT_MESH_LPN_INIT_POLL_TIMEOUT                        CONFIG_BT_MESH_LPN_POLL_TIMEOUT   // range 10 ~ BT_MESH_LPN_POLL_TIMEOUT

/// Latency in milliseconds that it takes to enable scanning. This
/// is in practice how much time in advance before the Receive Window
/// that scanning is requested to be enabled.
#define CONFIG_BT_MESH_LPN_SCAN_LATENCY                             15    // range 0 ~ 50

/// Maximum number of groups that the LPN can subscribe to.
#define CONFIG_BT_MESH_LPN_GROUPS                                   8     // range 0 ~ 16384

/// Automatically subscribe all nodes address when friendship
/// established.
#define CONFIG_BT_MESH_LPN_SUB_ALL_NODES_ADDR                       true
#endif    // CONFIG_BT_MESH_LOW_POWER

#if (CONFIG_BT_MESH_FRIEND)
/// Controls whether the Friend feature is enabled by default when the
/// device boots up for the first time or unprovisioned. Can be changed
/// at runtime using bt_mesh_friend_set() function.
#define CONFIG_BT_MESH_FRIEND_ENABLED                               true

/// Receive Window in milliseconds supported by the Friend node.
#define CONFIG_BT_MESH_FRIEND_RECV_WIN                              255     // range 1 ~ 255

/// Minimum number of buffers available to be stored for each
/// local Friend Queue.
#define CONFIG_BT_MESH_FRIEND_QUEUE_SIZE                            16      // range 2 ~ 65536

/// Size of the Subscription List that can be supported by a
/// Friend node for a Low Power node.
#define CONFIG_BT_MESH_FRIEND_SUB_LIST_SIZE                         3       // range 0 ~ 1023

/// Number of Low Power Nodes the Friend can have a Friendship
/// with simultaneously.
#define CONFIG_BT_MESH_FRIEND_LPN_COUNT                             2       // range 1 ~ 1000

/// Number of incomplete segment lists that we track for each LPN
/// that we are Friends for. In other words, this determines how
/// many elements we can simultaneously be receiving segmented
/// messages from when the messages are going into the Friend queue.
#define CONFIG_BT_MESH_FRIEND_SEG_RX                                1       // range 1 ~ 1000

/// Latency in milliseconds between request for and start of Friend
/// advertising. Used to tune the ReceiveDelay, making Friend
/// start sending a message earlier thus compensating for the time between
/// pushing the message to the Bluetooth host and the actual advertising
/// start.
#define CONFIG_BT_MESH_FRIEND_ADV_LATENCY                           0       // range 0 ~ 4

/// Use a separate extended advertising set for Friend advertising,
/// otherwise a shared advertising set will be used. Using the separate
/// extended advertising set makes the Friend node send friendship
/// messages as close to the start of the ReceiveWindow as possible,
/// thus reducing the scanning time on the Low Power node.
#define CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE                      true
#else
#define CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE                      false
#endif    // CONFIG_BT_MESH_FRIEND

/* menu "Capabilities" */
/// This option specifies how many subnets a Mesh network can
/// participate in at the same time.
#define CONFIG_BT_MESH_SUBNET_COUNT                                 2       // range 1 ~ 4096

/// This option specifies how many application keys the device can
/// store per network.
#define CONFIG_BT_MESH_APP_KEY_COUNT                                2       // range 1 ~ 4096

/// This option specifies how many application keys each model can
/// at most be bound to.
#define CONFIG_BT_MESH_MODEL_KEY_COUNT                              2       // range 1 ~ 4096

/// This option specifies how many group addresses each model can
/// at most be subscribed to.
#define CONFIG_BT_MESH_MODEL_GROUP_COUNT                            2       // range 1 ~ 4096

/// This option specifies how many Label UUIDs can be stored.
#define CONFIG_BT_MESH_LABEL_COUNT                                  2       // range 0 ~ 4096
/* endmenu # Capabilities */

/// Persistent storage of RPL in Settings. If BT_SETTINGS is not
/// enabled this choise will provide a non-persistent implementation
/// variant of the RPL list.
#define CONFIG_BT_MESH_RPL_STORAGE_MODE_SETTINGS                      true

#define CONFIG_BT_SETTINGS                                            true

/* menu "Persistent storage" */

/// This value defines in seconds how soon any pending changes
/// are actually written into persistent storage (flash) after
/// a change occurs.
#define CONFIG_BT_MESH_STORE_TIMEOUT                                  3     // range 0 ~ 1000000

/// This value defines how often the local sequence number gets
/// updated in persistent storage (i.e. flash). E.g. a value of 100
/// means that the sequence number will be stored to flash on every
/// 100th increment. If the node sends messages very frequently a
/// higher value makes more sense, whereas if the node sends
/// infrequently a value as low as 0 (update storage for every
/// increment) can make sense. When the stack gets initialized it
/// will add this number to the last stored one, so that it starts
/// off with a value that's guaranteed to be larger than the last
/// one used before power off.
#if (CONFIG_BT_SETTINGS)
#define CONFIG_BT_MESH_SEQ_STORE_RATE                                 128   // range 0 ~ 1000000
#else
#define CONFIG_BT_MESH_SEQ_STORE_RATE                                 1
#endif  // CONFIG_BT_SETTINGS

#if (CONFIG_BT_MESH_RPL_STORAGE_MODE_SETTINGS && CONFIG_BT_SETTINGS)
/// This value defines time in seconds until unsaved RPL and SRPL entries
/// are written to the persistent storage. Setting this value
/// to a large number may lead to security vulnerabilities if a node
/// gets powered off before the timer is fired. When flash is used
/// as the persistent storage setting this value to a low number
/// may wear out flash sooner or later. However, if the RPL gets
/// updated infrequently a value as low as 0 (write immediately)
/// may make sense. Setting this value to -1 will disable this timer.
/// In this case, a user is responsible to store pending RPL entries
/// using @ref bt_mesh_rpl_pending_store. In the mean time, when
/// IV Index is updated, the outdated RPL entries will still be
/// stored by @ref BT_MESH_STORE_TIMEOUT. Finding the right balance
/// between this timeout and calling @ref bt_mesh_rpl_pending_store
/// may reduce a risk of security vulnerability and flash wear out.
/// Failure to store the RPL and becoming vulnerable after reboot
/// will cause the device to not perform the replay protection
/// required by the spec.
#define CONFIG_BT_MESH_RPL_STORE_TIMEOUT                              5   // range -1 ~ 1000000
#endif  // CONFIG_BT_MESH_RPL_STORAGE_MODE_SETTINGS && CONFIG_BT_SETTINGS

/// This option enables a separate cooperative thread which is used to
/// store Bluetooth Mesh configuration. When this option is disabled,
/// the stack's configuration is stored in the system workqueue. This
/// means that the system workqueue will be blocked for the time needed
/// to store the pending data. This time may significantly increase if
/// the flash driver does the erase operation. Enabling this option
/// allows Bluetooth Mesh not to block the system workqueue, and thus
/// process the incoming and outgoing messages while the flash driver
/// waits for the controller to allocate the time needed to write the
/// data and/or erase the required flash pages.
#define CONFIG_BT_MESH_SETTINGS_WORKQ                                 true

#if (CONFIG_BT_MESH_SETTINGS_WORKQ)
#define CONFIG_BT_MESH_SETTINGS_WORKQ_PRIO                            2

/// Size of the settings workqueue stack.
#define CONFIG_BT_MESH_SETTINGS_WORKQ_STACK_SIZE                      768
#endif  // CONFIG_BT_MESH_SETTINGS_WORKQ

/* endmenu # Persistent storage */

/* CONFIG MEHS LOG LEVEL */
enum {
    CONFIG_BT_MESH_LOG_LEVEL = 0,
    CONFIG_BT_MESH_ACCESS_LOG_LEVEL,
    CONFIG_BT_MESH_ADV_LOG_LEVEL,
    CONFIG_BT_MEHS_AES_CCM_LOG_LEVEL,
    CONFIG_BT_MESH_KEYS_LOG_LEVEL,
    CONFIG_BT_MESH_BEACON_LOG_LEVEL = 5,
    CONFIG_BT_MESH_MODEL_LOG_LEVEL,
    CONFIG_NET_BUF_LOG_LEVEL,
    CONFIG_BT_MESH_CDB_LOG_LEVEL,
    CONFIG_BT_MESH_CFG_LOG_LEVEL,
    CONFIG_BT_MESH_CRYPTO_LOG_LEVEL = 10,
    CONFIG_BT_MESH_DFU_LOG_LEVEL,
    CONFIG_BT_MESH_FRIEND_LOG_LEVEL,
    CONFIG_BT_MESH_PROXY_LOG_LEVEL,
    CONFIG_BT_MESH_TRANS_LOG_LEVEL,
    CONFIG_BT_MESH_LOW_POWER_LOG_LEVEL = 15,
    CONFIG_BT_MESH_NET_LOG_LEVEL,
    CONFIG_BT_MESH_PROV_LOG_LEVEL,
    CONFIG_BT_MESH_PROVISIONEE_LOG_LEVEL,
    CONFIG_BT_MESH_PROVISIONER_LOG_LEVEL,
    CONFIG_BT_MESH_RPL_LOG_LEVEL = 20,
    CONFIG_BT_MESH_SETTINGS_LOG_LEVEL,
    CONFIG_BT_MESH_SCAN_LOG_LEVEL,
    CONFIG_BT_MESH_KERNEL_LOG_LEVEL,
    CONFIG_BT_MESH_SOLICI_MODEL_LOG_LEVEL,
    CONFIG_BTTESTER_LOG_LEVEL = 25,
    CONFIG_BT_MESH_BRG_LOG_LEVEL,
    CONFIG_BT_MESH_MAX_LOG_LEVEL,
};

#define CONFIG_NET_BUF_LOG

/* endmenu # MESH LOG LEVEL */

#endif /* MESH_CFG_H_ */
