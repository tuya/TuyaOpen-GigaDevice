/*
 * EAPOL supplicant state machines
 * Copyright (c) 2004-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef WPAS_EAPOL_SUPP_SM_H
#define WPAS_EAPOL_SUPP_SM_H

#include "build_config.h"

#ifdef CONFIG_EAP_TLS
struct tls_cert_data;
#endif
typedef enum { Unauthorized, Authorized } PortStatus;
typedef enum { Auto, ForceUnauthorized, ForceAuthorized } PortControl;

/* Supplicant PAE state machine */
enum SUPP_PAE_state_t{
    SUPP_PAE_UNKNOWN = 0,
    SUPP_PAE_DISCONNECTED = 1,
    SUPP_PAE_LOGOFF = 2,
    SUPP_PAE_CONNECTING = 3,
    SUPP_PAE_AUTHENTICATING = 4,
    SUPP_PAE_AUTHENTICATED = 5,
    /* unused(6) */
    SUPP_PAE_HELD = 7,
    SUPP_PAE_RESTART = 8,
    SUPP_PAE_S_FORCE_AUTH = 9,
    SUPP_PAE_S_FORCE_UNAUTH = 10
} ; /* dot1xSuppPaeState */

/* Supplicant Backend state machine */
enum SUPP_BE_state_t{
    SUPP_BE_UNKNOWN     = 0,
    SUPP_BE_INITIALIZE  = 1,
    SUPP_BE_IDLE        = 2,
    SUPP_BE_REQUEST     = 3,
    SUPP_BE_RECEIVE     = 4,
    SUPP_BE_RESPONSE    = 5,
    SUPP_BE_FAIL        = 6,
    SUPP_BE_TIMEOUT     = 7,
    SUPP_BE_SUCCESS     = 8
} ; /* dot1xSuppBackendPaeState */

/* Key Receive state machine */
enum KEY_RX_state_t{
    KEY_RX_UNKNOWN = 0,
    KEY_RX_NO_KEY_RECEIVE, KEY_RX_KEY_RECEIVE
} ;

enum cb_status_t { EAPOL_CB_IN_PROGRESS = 0, EAPOL_CB_SUCCESS, EAPOL_CB_FAILURE } ;

typedef enum {
    WORK_TYPE_NONE,
    WORK_TYPE_WPS,
    WORK_TYPE_EAP_TLS,
    WORK_TYPE_UNKNOWN,
} WorkType;

struct eapol_sm {
    WorkType workType;

    /* Timers */
    unsigned int authWhile;
    unsigned int heldWhile;
    unsigned int startWhen;
    unsigned int idleWhile; /* for EAP state machine */
    int timer_tick_enabled;

    /* Global variables */
    bool eapFail;
    bool eapolEap;
    bool eapSuccess;
    bool initialize;
    bool keyDone;
    bool keyRun;
    PortControl portControl;
    bool portEnabled;
    PortStatus suppPortStatus;  /* dot1xSuppControlledPortStatus */
    bool portValid;
    bool suppAbort;
    bool suppFail;
    bool suppStart;
    bool suppSuccess;
    bool suppTimeout;

    /* Supplicant PAE state machine */
    enum SUPP_PAE_state_t SUPP_PAE_state; /* dot1xSuppPaeState */
    /* Variables */
    bool userLogoff;
    bool logoffSent;
    unsigned int startCount;
    bool eapRestart;
    PortControl sPortMode;
    /* Constants */
    unsigned int heldPeriod; /* dot1xSuppHeldPeriod */
    unsigned int startPeriod; /* dot1xSuppStartPeriod */
    unsigned int maxStart; /* dot1xSuppMaxStart */

    // /* Key Receive state machine */
    // enum KEY_RX_state_t KEY_RX_state;
    // /* Variables */
    // bool rxKey;

    /* Supplicant Backend state machine */
    enum SUPP_BE_state_t SUPP_BE_state; /* dot1xSuppBackendPaeState */
    /* Variables */
    bool eapNoResp;
    bool eapReq;
    bool eapResp;
    /* Constants */
    unsigned int authPeriod; /* dot1xSuppAuthPeriod */

#if 0
    /* Statistics */
    unsigned int dot1xSuppEapolFramesRx;
    unsigned int dot1xSuppEapolFramesTx;
    unsigned int dot1xSuppEapolStartFramesTx;
    unsigned int dot1xSuppEapolLogoffFramesTx;
    unsigned int dot1xSuppEapolRespFramesTx;
    unsigned int dot1xSuppEapolReqIdFramesRx;
    unsigned int dot1xSuppEapolReqFramesRx;
    unsigned int dot1xSuppInvalidEapolFramesRx;
    unsigned int dot1xSuppEapLengthErrorFramesRx;
    unsigned int dot1xSuppLastEapolFrameVersion;
    unsigned char dot1xSuppLastEapolFrameSource[6];
#endif
    /* Miscellaneous variables (not defined in IEEE 802.1X-2004) */
    bool changed;
    struct eap_sm *eap;
    bool initial_req;
    // uint8_t *last_rx_key;
    // size_t last_rx_key_len;
    struct wpabuf *eapReqData;  /* for EAP */
    bool altAccept;             /* for EAP */
    bool altReject;             /* for EAP */
    bool eapTriggerStart;
    bool replay_counter_valid;
    uint8_t last_replay_counter[16];

    enum cb_status_t cb_status;
    bool cached_pmk;

    bool unicast_key_received, broadcast_key_received;

    bool force_authorized_update;

};

enum eapol_supp_result {
    EAPOL_SUPP_RESULT_FAILURE,
    EAPOL_SUPP_RESULT_SUCCESS,
    EAPOL_SUPP_RESULT_EXPECTED_FAILURE
};

#ifdef IEEE8021X_EAPOL
struct eapol_sm *eapol_sm_init(int vif_idx, WorkType work_type);
void eapol_sm_deinit(struct eapol_sm *sm);
void eapol_sm_step(struct eapol_sm *sm);
void eapol_enable_timer_tick(struct eapol_sm *sm);
int eapol_sm_get_status(struct eapol_sm *sm, char *buf, size_t buflen,
            int verbose);
int eapol_sm_get_mib(struct eapol_sm *sm, char *buf, size_t buflen);
void eapol_sm_configure(struct eapol_sm *sm, int heldPeriod, int authPeriod,
            int startPeriod, int maxStart);
void eapol_sm_notify_tx_eapol_key(struct eapol_sm *sm);
void eapol_sm_notify_portEnabled(struct eapol_sm *sm, bool enabled);
void eapol_sm_notify_portValid(struct eapol_sm *sm, bool valid);
void eapol_sm_notify_eap_success(struct eapol_sm *sm, bool success);
void eapol_sm_notify_eap_fail(struct eapol_sm *sm, bool fail);
int eapol_sm_get_key(struct eapol_sm *sm, uint8_t *key, size_t len);
const uint8_t * eapol_sm_get_session_id(struct eapol_sm *sm, size_t *len);
void eapol_sm_notify_logoff(struct eapol_sm *sm, bool logoff);
void eapol_sm_notify_cached(struct eapol_sm *sm);
void eapol_sm_notify_pmkid_attempt(struct eapol_sm *sm);
void eapol_sm_register_scard_ctx(struct eapol_sm *sm, void *ctx);
void eapol_sm_notify_portControl(struct eapol_sm *sm, PortControl portControl);
void eapol_sm_notify_ctrl_attached(struct eapol_sm *sm);
void eapol_sm_notify_ctrl_response(struct eapol_sm *sm);
void eapol_sm_request_reauth(struct eapol_sm *sm);
void eapol_sm_notify_lower_layer_success(struct eapol_sm *sm, int in_eapol_sm);
void eapol_sm_invalidate_cached_session(struct eapol_sm *sm);
const char * eapol_sm_get_method_name(struct eapol_sm *sm);
// void eapol_sm_set_ext_pw_ctx(struct eapol_sm *sm,
//                  struct ext_password_data *ext);
int eapol_sm_failed(struct eapol_sm *sm);
void eapol_sm_erp_flush(struct eapol_sm *sm);
struct wpabuf * eapol_sm_build_erp_reauth_start(struct eapol_sm *sm);
void eapol_sm_process_erp_finish(struct eapol_sm *sm, const uint8_t *buf,
                 size_t len);
int eapol_sm_get_eap_proxy_imsi(void *ctx, int sim_num, char *imsi,
                size_t *len);
int eapol_sm_update_erp_next_seq_num(struct eapol_sm *sm, uint16_t next_seq_num);

#else /* IEEE8021X_EAPOL */
static inline struct eapol_sm *eapol_sm_init(int vif_idx, WorkType work_type)
{
    return (struct eapol_sm *) 1;
}
static inline void eapol_sm_deinit(struct eapol_sm *sm)
{
}
static inline void eapol_sm_step(struct eapol_sm *sm)
{
}
static inline int eapol_sm_get_status(struct eapol_sm *sm, char *buf,
                      size_t buflen, int verbose)
{
    return 0;
}
static inline int eapol_sm_get_mib(struct eapol_sm *sm, char *buf,
                   size_t buflen)
{
    return 0;
}
static inline void eapol_sm_configure(struct eapol_sm *sm, int heldPeriod,
                      int authPeriod, int startPeriod,
                      int maxStart)
{
}
static inline void eapol_sm_notify_tx_eapol_key(struct eapol_sm *sm)
{
}
static inline void eapol_sm_notify_portEnabled(struct eapol_sm *sm,
                           bool enabled)
{
}
static inline void eapol_sm_notify_portValid(struct eapol_sm *sm,
                         bool valid)
{
}
static inline void eapol_sm_notify_eap_success(struct eapol_sm *sm,
                           bool success)
{
}
static inline void eapol_sm_notify_eap_fail(struct eapol_sm *sm, bool fail)
{
}

static inline int eapol_sm_get_key(struct eapol_sm *sm, uint8_t *key, size_t len)
{
    return -1;
}
static inline const uint8_t *
eapol_sm_get_session_id(struct eapol_sm *sm, size_t *len)
{
    return NULL;
}
static inline void eapol_sm_notify_logoff(struct eapol_sm *sm, bool logoff)
{
}
static inline void eapol_sm_notify_cached(struct eapol_sm *sm)
{
}
static inline void eapol_sm_notify_pmkid_attempt(struct eapol_sm *sm)
{
}
#define eapol_sm_register_scard_ctx(sm, ctx) do { } while (0)
static inline void eapol_sm_notify_portControl(struct eapol_sm *sm,
                           PortControl portControl)
{
}
static inline void eapol_sm_notify_ctrl_attached(struct eapol_sm *sm)
{
}
static inline void eapol_sm_notify_ctrl_response(struct eapol_sm *sm)
{
}
static inline void eapol_sm_request_reauth(struct eapol_sm *sm)
{
}
static inline void eapol_sm_notify_lower_layer_success(struct eapol_sm *sm,
                               int in_eapol_sm)
{
}
static inline void eapol_sm_invalidate_cached_session(struct eapol_sm *sm)
{
}
static inline const char * eapol_sm_get_method_name(struct eapol_sm *sm)
{
    return NULL;
}
static inline int eapol_sm_failed(struct eapol_sm *sm)
{
    return 0;
}
static inline void eapol_sm_erp_flush(struct eapol_sm *sm)
{
}
static inline struct wpabuf *
eapol_sm_build_erp_reauth_start(struct eapol_sm *sm)
{
    return NULL;
}
static inline void eapol_sm_process_erp_finish(struct eapol_sm *sm,
                           const uint8_t *buf, size_t len)
{
}
static inline int eapol_sm_update_erp_next_seq_num(struct eapol_sm *sm,
                           uint16_t next_seq_num)
{
    return -1;
}
#endif /* IEEE8021X_EAPOL */

#endif /* WPAS_EAPOL_SUPP_SM_H */
