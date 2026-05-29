/**
 * @file wifi_bridge.c
 * @brief SPI <-> WiFi L2 bridge for GD32VW553 NIC mode.
 *
 * Implements the three extern functions declared in mm_spi.c:
 *   send_to_wifi()         -- SPI RX -> WiFi TX (upstream, master->AP)
 *   wifi_trx_hook_init()   -- install netif->input intercept hook
 *   wifi_trx_hook_deinit() -- remove hook and restore original input
 *
 * Two operating modes controlled by g_bridge_mode:
 *   BRIDGE mode (g_bridge_mode != 0):
 *     WiFi RX frames are forwarded to SPI TX queue (master MCU is awake).
 *   SELF mode   (g_bridge_mode == 0):
 *     WiFi RX frames are delivered to GD32's own LwIP stack (master is asleep).
 *
 * Mode switching API (reserved — wire to SPI CMD when command IDs are defined):
 *   wifi_bridge_enter_sleep_mode() -- switch to SELF mode
 *   wifi_bridge_exit_sleep_mode()  -- switch back to BRIDGE mode
 *
 * Data flow:
 *   Upstream  (master -> AP):
 *     SPI DMA RX -> packet_from_spi() -> send_to_wifi()
 *       -> net_buf_tx_alloc() + memcpy + macif_tx_start() -> WiFi MAC
 *
 *   Downstream (AP -> master):
 *     WiFi driver -> wifi_bridge_input() (netif->input hook)
 *       -> [BRIDGE] packet_to_spi() -> SPI TX queue -> master MCU
 *       -> [SELF]   g_orig_input()  -> GD32 own LwIP
 */

#include "wifi_netif.h"   /* net_buf_tx_alloc, net_buf_tx_t            */
#include "lwip/netif.h"   /* struct netif, netif_input_fn, netif_is_up */
#include "lwip/pbuf.h"    /* pbuf_alloc, pbuf_free, pbuf_copy_partial  */
#include "lwip/err.h"     /* err_t, 0                             */
#include "app_cfg.h"
#include "wrapper_os.h"
#include "macif_api.h"    /* macif_tx_start                            */

extern void *vif_idx_to_net_if(uint8_t vif_idx);
extern int packet_to_spi(unsigned char *buf, unsigned int len);

/* STA VIF index is always 0 on GD32VW553 */
#define WIFI_BRIDGE_STA_VIF_IDX  0U

/* Hard cap matching SPI DMA buffer (TRX_BUF_SIZE - sizeof(mm_data_header) = 1588) */
#define WIFI_BRIDGE_MAX_ETH_LEN  1514U

/* Operating mode values for g_bridge_mode */
#define WIFI_BRIDGE_MODE_SELF   0  /* GD32 own LwIP only                */
#define WIFI_BRIDGE_MODE_BRIDGE 1  /* Forward to SPI only               */
#define WIFI_BRIDGE_MODE_TEST   2  /* Both SPI and LwIP paths active    */

/* When enabled, DHCP packets (UDP port 67/68) received in BRIDGE/TEST mode
 * are NOT forwarded to the master MCU via SPI; they are handled locally by
 * GD32's own LwIP stack so that IP address negotiation always succeeds.
 * Define to 0 to disable (all packets including DHCP go to the SPI path). */
#ifndef WIFI_BRIDGE_DHCP_LOCAL_HANDLE
#define WIFI_BRIDGE_DHCP_LOCAL_HANDLE  1
#endif

/* When enabled, EAPOL (802.1X) packets (EtherType 0x888E) received in
 * BRIDGE/TEST mode are NOT forwarded to the master MCU via SPI; they are
 * handled locally by GD32's own LwIP/security stack.
 * Define to 0 to disable. */
#ifndef WIFI_BRIDGE_EAPOL_LOCAL_HANDLE
#define WIFI_BRIDGE_EAPOL_LOCAL_HANDLE  1
#endif

static volatile int   g_bridge_mode = WIFI_BRIDGE_MODE_SELF;

/* Saved original netif->input pointer (tcpip_input) */
static netif_input_fn g_orig_input  = NULL;

/* STA network interface handle — set by wifi_trx_hook_init */
static struct netif  *g_sta_netif   = NULL;

/* -----------------------------------------------------------------------
 * wifi_bridge_rx_free_cb -- LwIP custom-pbuf free callback.
 *
 * Called by pbuf_free() when the LwIP stack releases a pbuf_custom that
 * was injected via wifi_bridge_inject_to_lwip().  The net_buf_rx_t header
 * and its trailing payload data are co-allocated in one sys_malloc call,
 * so freeing p (which == rx_buf, the start of the allocation) releases both.
 * ----------------------------------------------------------------------- */
static void wifi_bridge_rx_free_cb(struct pbuf *p)
{
    sys_mfree(p);
}

/* -----------------------------------------------------------------------
 * wifi_bridge_inject_to_lwip -- inject a raw Ethernet frame into GD32 LwIP.
 *
 * Allocates a pbuf_custom (net_buf_rx_t) with co-located payload, sets the
 * free callback, builds a PBUF_REF pbuf, and calls g_orig_input DIRECTLY
 * (bypassing the bridge hook) to prevent re-entrant forwarding loops.
 * Used in TEST mode upstream path: SPI RX -> WiFi MAC AND GD32 LwIP.
 * ----------------------------------------------------------------------- */
static void wifi_bridge_inject_to_lwip(const unsigned char *buf, uint16_t len)
{
    net_buf_rx_t *rx_buf;
    uint8_t      *rx_data;
    struct pbuf  *p;

    if (g_orig_input == NULL || g_sta_netif == NULL) {
        return;
    }

    /* Co-allocate: net_buf_rx_t header followed immediately by payload copy */
    rx_buf = (net_buf_rx_t *)sys_malloc(sizeof(net_buf_rx_t) + len);
    if (rx_buf == NULL) {
        printf("[WB][TEST] inject_to_lwip: malloc(%u) failed", len);
        return;
    }
    rx_data = (uint8_t *)(rx_buf + 1);
    sys_memcpy(rx_data, buf, len);

    rx_buf->custom_free_function = wifi_bridge_rx_free_cb;
    p = pbuf_alloced_custom(PBUF_RAW, len, PBUF_REF, rx_buf, rx_data, len);
    if (p == NULL) {
        sys_mfree(rx_buf);
        printf("[WB][TEST] inject_to_lwip: pbuf_alloced_custom failed");
        return;
    }
    /* Call g_orig_input (tcpip_input) directly to bypass the bridge hook */
    if (g_orig_input(p, g_sta_netif) != 0) {
        pbuf_free(p); /* triggers wifi_bridge_rx_free_cb -> sys_mfree */
    }
}

/* -----------------------------------------------------------------------
 * wifi_bridge_forward_to_spi -- linearise pbuf and post to SPI TX queue.
 *
 * Single-segment fast path uses p->payload directly (packet_to_spi copies
 * internally).  Multi-segment chains are linearised on the heap first.
 * Returns 1 on success, 0 if the frame was dropped.
 * ----------------------------------------------------------------------- */
static int wifi_bridge_forward_to_spi(struct pbuf *p)
{
    uint8_t  *flat;
    uint16_t  copied;

    if (p->next == NULL) {
        packet_to_spi((unsigned char *)p->payload, (unsigned int)p->tot_len);
        return 1;
    }

    flat = (uint8_t *)sys_malloc(p->tot_len);
    if (flat == NULL) {
        printf("[WB] forward_to_spi: malloc(%u) failed, dropping frame", p->tot_len);
        return 0;
    }
    copied = pbuf_copy_partial(p, flat, p->tot_len, 0U);
    if (copied > 0U) {
        packet_to_spi(flat, (unsigned int)copied);
    } else {
        printf("[WB] pbuf_copy_partial returned 0, dropping (tot_len=%u)", p->tot_len);
    }
    sys_mfree(flat);
    return (copied > 0U) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 * Local-only packet filter — table-driven, extensible.
 *
 * Rule descriptor:
 *   ethertype   : required Ethernet type field to match
 *   ip_proto    : 0  = EtherType match only (e.g. EAPOL, ARP)
 *                 17 = also match UDP src/dst port range below
 *   udp_port_lo : inclusive lower bound of UDP port range (ip_proto != 0)
 *   udp_port_hi : inclusive upper bound of UDP port range (ip_proto != 0)
 *
 * To add a new locally-handled protocol, append one line to g_local_rules[].
 * No other code needs to change.
 *
 * Examples:
 *   { 0x888EU, 0U,   0U,   0U   }  -- EAPOL  (802.1X)
 *   { 0x0806U, 0U,   0U,   0U   }  -- ARP
 *   { 0x0800U, 17U,  67U,  68U  }  -- DHCP   (IPv4/UDP 67-68)
 *   { 0x0800U, 17U, 123U, 123U  }  -- NTP    (IPv4/UDP 123)
 * ----------------------------------------------------------------------- */
typedef struct {
    uint16_t ethertype;    /* Ethernet type to match (required)              */
    uint8_t  ip_proto;     /* IP protocol; 0 = no IP/UDP check needed        */
    uint16_t udp_port_lo;  /* UDP port range lower bound (inclusive)         */
    uint16_t udp_port_hi;  /* UDP port range upper bound (inclusive)         */
} wb_local_rule_t;

static const wb_local_rule_t g_local_rules[] = {
#if WIFI_BRIDGE_EAPOL_LOCAL_HANDLE
    { 0x888EU, 0U,  0U,  0U  },  /* EAPOL (IEEE 802.1X)       */
#endif
#if WIFI_BRIDGE_DHCP_LOCAL_HANDLE
    { 0x0800U, 17U, 67U, 68U },  /* DHCP  (IPv4/UDP port 67-68) */
#endif
};

#define WB_LOCAL_RULES_COUNT  ((int)(sizeof(g_local_rules) / sizeof(g_local_rules[0])))

/* -----------------------------------------------------------------------
 * wifi_bridge_is_local_only -- returns non-zero if the packet matches any
 * entry in g_local_rules[] and should be handled locally by GD32's LwIP.
 *
 * Optimisations:
 *   - When WB_LOCAL_RULES_COUNT == 0 (all macros off) the compiler
 *     eliminates the entire body via dead-code removal.
 *   - Single-segment fast path: p->payload cast directly — no per-byte
 *     pbuf_get_at() overhead for the common contiguous WiFi RX buffer.
 *   - EtherType is read exactly once and shared across all rule iterations.
 * ----------------------------------------------------------------------- */
static int wifi_bridge_is_local_only(struct pbuf *p)
{
    const uint8_t  *hdr;
    uint16_t        ethertype;
    int             i;

    if (WB_LOCAL_RULES_COUNT == 0 || p->tot_len < 14U) {
        return 0;
    }

    /* Fast path: single contiguous segment — direct pointer access */
    hdr = (p->next == NULL) ? (const uint8_t *)p->payload : NULL;

    /* Read EtherType once; shared by every rule in the loop below */
    ethertype = (hdr != NULL)
                    ? (((uint16_t)hdr[12] << 8) | hdr[13])
                    : (((uint16_t)pbuf_get_at(p, 12U) << 8) |
                        (uint16_t)pbuf_get_at(p, 13U));

    for (i = 0; i < WB_LOCAL_RULES_COUNT; i++) {
        const wb_local_rule_t *r = &g_local_rules[i];

        if (ethertype != r->ethertype) {
            continue;
        }
        if (r->ip_proto == 0U) {
            return 1;  /* EtherType-only match (e.g. EAPOL) */
        }

        /* IPv4/UDP port range check */
        if (p->tot_len >= 42U) {
            uint8_t proto = (hdr != NULL) ? hdr[23] : pbuf_get_at(p, 23U);
            if (proto == r->ip_proto) {
                uint8_t  ihl    = ((hdr != NULL) ? hdr[14] : pbuf_get_at(p, 14U)) & 0x0FU;
                if (ihl >= 5U) {
                    uint16_t udp_off = 14U + (uint16_t)ihl * 4U;
                    if (p->tot_len >= (uint16_t)(udp_off + 8U)) {
                        uint16_t udp_src, udp_dst;
                        if (hdr != NULL) {
                            udp_src = ((uint16_t)hdr[udp_off]      << 8) | hdr[udp_off + 1U];
                            udp_dst = ((uint16_t)hdr[udp_off + 2U] << 8) | hdr[udp_off + 3U];
                        } else {
                            udp_src = ((uint16_t)pbuf_get_at(p, udp_off)      << 8) |
                                       (uint16_t)pbuf_get_at(p, udp_off + 1U);
                            udp_dst = ((uint16_t)pbuf_get_at(p, udp_off + 2U) << 8) |
                                       (uint16_t)pbuf_get_at(p, udp_off + 3U);
                        }
                        if ((udp_src >= r->udp_port_lo && udp_src <= r->udp_port_hi) ||
                            (udp_dst >= r->udp_port_lo && udp_dst <= r->udp_port_hi)) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * Internal: WiFi RX intercept hook.
 *
 * Replaces netif->input; called from the LwIP tcpip_thread context.
 * packet_to_spi() is non-blocking (queue post with timeout=0), safe here.
 *
 * BRIDGE mode:  pbuf consumed + freed here (custom_free_function reclaims
 *               the buffer to the WiFi driver).
 *               EXCEPTION: locally-handled packets (DHCP, EAPOL, ...) are
 *               never forwarded to the master MCU; they go to GD32's own
 *               LwIP stack for IP negotiation and 802.1X authentication.
 * TEST mode:    Local-only packets skip the SPI path and go to LwIP only.
 *               Non-local: SPI first (copy), then g_orig_input takes pbuf.
 * SELF mode:    pbuf passed directly to the original tcpip_input.
 * ----------------------------------------------------------------------- */
static err_t wifi_bridge_input(struct pbuf *p, struct netif *netif)
{
    if (p == NULL) {
        return 0;
    }

    if (g_bridge_mode == WIFI_BRIDGE_MODE_BRIDGE) {
        /* ----- BRIDGE mode: SPI only (matched local rules go to LwIP) ----- */
        if (wifi_bridge_is_local_only(p)) {
            if (g_orig_input != NULL) {
                return g_orig_input(p, netif);
            }
            pbuf_free(p);
            return 0;
        }
        wifi_bridge_forward_to_spi(p);
        pbuf_free(p); /* reclaims WiFi driver buffer via custom_free_function */
        return 0;
    }

    if (g_bridge_mode == WIFI_BRIDGE_MODE_TEST) {
        /* ----- TEST mode: matched local rules skip SPI; LwIP gets all ----- */
        if (!wifi_bridge_is_local_only(p)) {
            wifi_bridge_forward_to_spi(p);
        }
        if (g_orig_input != NULL) {
            return g_orig_input(p, netif); /* LwIP owns p, will call pbuf_free */
        }
        pbuf_free(p);
        return 0;
    }

    /* ----- SELF mode (default): deliver to GD32 own LwIP stack ----- */
    if (g_orig_input != NULL) {
        return g_orig_input(p, netif);
    }
    pbuf_free(p);
    return 0;
}

/* -----------------------------------------------------------------------
 * send_to_wifi() -- SPI RX -> WiFi TX
 *
 * buf/len describe a raw Ethernet frame:
 *   [ dst MAC (6) | src MAC (6) | EtherType (2) | payload ]
 * (the mm_data_header has already been stripped by packet_from_spi)
 *
 * Allocates a PBUF_RAW_TX buffer (includes PBUF_LINK_ENCAPSULATION_HLEN
 * headroom for WiFi driver use), copies the frame, and hands it to the
 * WiFi MAC via macif_tx_start, bypassing LwIP's IP layer entirely.
 *
 * Ownership rules:
 *   - On success: macif owns pbuf and frees it asynchronously after TX.
 *   - On failure: pbuf_free() is called here.  net_buf_tx_free() must NOT
 *     be used on this error path because the WiFi driver has not yet called
 *     net_buf_tx_info() (payload pointer has not been shifted).
 * ----------------------------------------------------------------------- */
int send_to_wifi(unsigned char *buf, unsigned int len)
{
    net_buf_tx_t *pbuf;

    if (buf == NULL || len == 0U || len > WIFI_BRIDGE_MAX_ETH_LEN) {
        printf("[WB] send_to_wifi: invalid args (buf=%p len=%u)\r\n", (void *)buf, len);
        return -1;
    }

    if (g_sta_netif == NULL || !netif_is_up(g_sta_netif)) {
        printf("[WB] send_to_wifi: STA netif not ready, dropping %u B\r\n", len);
        return -1;
    }

    pbuf = net_buf_tx_alloc((uint32_t)len);
    if (pbuf == NULL) {
        printf("[WB] send_to_wifi: net_buf_tx_alloc(%u) failed\r\n", len);
        return -1;
    }

    sys_memcpy(pbuf->payload, buf, len);

    if (macif_tx_start(g_sta_netif, pbuf, NULL, NULL) != 0) {
        /* macif rejected the frame before taking ownership — free it here */
        pbuf_free(pbuf);
        printf("[WB] send_to_wifi: macif_tx_start failed (len=%u)\r\n", len);
        return -1;
    }

    /* TEST mode: also inject a copy into GD32 own LwIP so both paths are
     * exercised simultaneously.  Calls g_orig_input directly (not via the
     * hook) to prevent re-entrant forwarding back to the SPI TX queue.   */
    if (g_bridge_mode == WIFI_BRIDGE_MODE_TEST) {
        wifi_bridge_inject_to_lwip(buf, (uint16_t)len);
    }

    return 0;
}

/* -----------------------------------------------------------------------
 * wifi_trx_hook_init() -- install bridge hook
 *
 * Must be called AFTER WiFi STA has associated and netif is up.
 * The master MCU guarantees this by sending the "hook_init" SPI command
 * only after receiving WiFi-connected confirmation from GD32.
 *
 * Safe to call multiple times; g_orig_input is preserved on re-init so
 * the double-hook scenario is avoided.
 * ----------------------------------------------------------------------- */
void wifi_trx_hook_init(void)
{
    g_sta_netif = (struct netif *)vif_idx_to_net_if(WIFI_BRIDGE_STA_VIF_IDX);
    if (g_sta_netif == NULL) {
        printf("[WB] wifi_trx_hook_init: VIF%u not found — WiFi not connected?\r\n",
               WIFI_BRIDGE_STA_VIF_IDX);
        return;
    }

    /* Preserve original only on the first install to survive re-init calls */
    if (g_orig_input == NULL) {
        g_orig_input = g_sta_netif->input;
    }

    g_sta_netif->input = wifi_bridge_input;
    g_bridge_mode = WIFI_BRIDGE_MODE_BRIDGE;

    printf("[WB] hook installed, BRIDGE mode ON (netif=%p)\r\n", (void *)g_sta_netif);
}

/* -----------------------------------------------------------------------
 * wifi_trx_hook_deinit() -- remove hook and restore original input
 * ----------------------------------------------------------------------- */
void wifi_trx_hook_deinit(void)
{
    if (g_sta_netif != NULL && g_orig_input != NULL) {
        g_sta_netif->input = g_orig_input;
    }

    g_bridge_mode = 0;
    g_orig_input  = NULL;
    g_sta_netif   = NULL;

    printf("[WB] hook removed, BRIDGE mode OFF\r\n");
}

/* -----------------------------------------------------------------------
 * wifi_bridge_enter_sleep_mode() -- switch to SELF mode
 *
 * Reserved interface.  Wire to ty_lp_proto_cs_data_parse() when the
 * "master enter sleep" SPI command ID is defined.
 * In SELF mode WiFi RX frames are processed by GD32's own LwIP stack;
 * the bridge hook remains installed so switching back is instant.
 * ----------------------------------------------------------------------- */
void wifi_bridge_enter_sleep_mode(void)
{
    g_bridge_mode = 0;
    printf("[WB] SELF mode ON (master is sleeping)\r\n");
}

void wifi_bridge_self_mode(void)
{
    g_bridge_mode = WIFI_BRIDGE_MODE_SELF;
    printf("[WB] SELF mode ON (master is sleeping)\r\n");
}

/* -----------------------------------------------------------------------
 * wifi_bridge_exit_sleep_mode() -- switch back to BRIDGE mode
 *
 * Reserved interface.  Wire to ty_lp_proto_cs_data_parse() when the
 * "master wake-up ACK" SPI command ID is defined.
 * ----------------------------------------------------------------------- */
void wifi_bridge_exit_sleep_mode(void)
{
    if (g_sta_netif != NULL) {
        g_bridge_mode = WIFI_BRIDGE_MODE_BRIDGE;
        printf("[WB] BRIDGE mode ON (master is awake)\r\n");
    } else {
        printf("[WB] exit_sleep: hook not installed, call wifi_trx_hook_init() first\r\n");
    }
}

/* -----------------------------------------------------------------------
 * wifi_bridge_enter_test_mode() -- activate both SPI and LwIP paths
 *
 * Downstream (WiFi -> GD32): frames are forwarded to the SPI TX queue
 *   (master MCU) AND delivered to GD32's own LwIP stack.
 * Upstream (SPI -> WiFi): frames are transmitted via WiFi MAC AND injected
 *   into GD32's LwIP as an RX frame via wifi_bridge_inject_to_lwip().
 *
 * The inject helper calls g_orig_input directly so the bridge hook is
 * bypassed — no re-entrant forwarding or packet duplication loops occur.
 *
 * Call wifi_trx_hook_init() before this function.
 * ----------------------------------------------------------------------- */
void wifi_bridge_enter_test_mode(void)
{
    if (g_sta_netif != NULL) {
        g_bridge_mode = WIFI_BRIDGE_MODE_TEST;
        printf("[WB] TEST mode ON (SPI + LwIP both active)\r\n");
    } else {
        printf("[WB] enter_test: hook not installed, call wifi_trx_hook_init() first\r\n");
    }
}
