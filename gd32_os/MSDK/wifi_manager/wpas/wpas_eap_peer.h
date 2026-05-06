/*
 * EAP peer state machine functions (RFC 4137)
 * Copyright (c) 2004-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef WPAS_EAP_PEER_H
#define WPAS_EAP_PEER_H

struct eap_method_type {
    int vendor;
    uint32_t method;
};

/*==============================================================*/
/* eap_peer/eap_i.h */
/*==============================================================*/
#define NO_EAP_METHOD_ERROR (-1)

/* RFC 4137 - EAP Peer state machine */

typedef enum {
    DECISION_FAIL, DECISION_COND_SUCC, DECISION_UNCOND_SUCC
} EapDecision;

typedef enum {
    METHOD_NONE, METHOD_INIT, METHOD_CONT, METHOD_MAY_CONT, METHOD_DONE
} EapMethodState;

/**
 * struct eap_method_ret - EAP return values from struct eap_method::process()
 *
 * These structure contains OUT variables for the interface between peer state
 * machine and methods (RFC 4137, Sect. 4.2). eapRespData will be returned as
 * the return value of struct eap_method::process() so it is not included in
 * this structure.
 */
struct eap_method_ret {
    /**
     * ignore - Whether method decided to drop the current packed (OUT)
     */
    bool ignore;

    /**
     * methodState - Method-specific state (IN/OUT)
     */
    EapMethodState methodState;

    /**
     * decision - Authentication decision (OUT)
     */
    EapDecision decision;

    /**
     * allowNotifications - Whether method allows notifications (OUT)
     */
    bool allowNotifications;
};

/**
 * struct eap_method - EAP method interface
 * This structure defines the EAP method interface. Each method will need to
 * register its own EAP type, EAP name, and set of function pointers for method
 * specific operations. This interface is based on section 4.4 of RFC 4137.
 */
struct eap_method {
    /**
     * vendor - EAP Vendor-ID (EAP_VENDOR_*) (0 = IETF)
     */
    int vendor;

    /**
     * method - EAP type number (EAP_TYPE_*)
     */
    enum eap_type method;

    /**
     * name - Name of the method (e.g., "TLS")
     */
    const char *name;

    /**
     * init - Initialize an EAP method
     * @sm: Pointer to EAP state machine
     * Returns: Pointer to allocated private data, or %NULL on failure
     *
     * This function is used to initialize the EAP method explicitly
     * instead of using METHOD_INIT state as specific in RFC 4137. The
     * method is expected to initialize it method-specific state and return
     * a pointer that will be used as the priv argument to other calls.
     */
    void * (*init)(struct eap_sm *sm);

    /**
     * deinit - Deinitialize an EAP method
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     *
     * Deinitialize the EAP method and free any allocated private data.
     */
    void (*deinit)(struct eap_sm *sm, void *priv);

    /**
     * process - Process an EAP request
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     * @ret: Return values from EAP request validation and processing
     * @reqData: EAP request to be processed (eapReqData)
     * Returns: Pointer to allocated EAP response packet (eapRespData)
     *
     * This function is a combination of m.check(), m.process(), and
     * m.buildResp() procedures defined in section 4.4 of RFC 4137 In other
     * words, this function validates the incoming request, processes it,
     * and build a response packet. m.check() and m.process() return values
     * are returned through struct eap_method_ret *ret variable. Caller is
     * responsible for freeing the returned EAP response packet.
     */
    struct wpabuf * (*process)(struct eap_sm *sm, void *priv,
                                struct eap_method_ret *ret,
                                const struct wpabuf *reqData);

    /**
     * isKeyAvailable - Find out whether EAP method has keying material
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     * Returns: %true if key material (eapKeyData) is available
     */
    bool (*isKeyAvailable)(struct eap_sm *sm, void *priv);

    /**
     * getKey - Get EAP method specific keying material (eapKeyData)
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     * @len: Pointer to variable to store key length (eapKeyDataLen)
     * Returns: Keying material (eapKeyData) or %NULL if not available
     *
     * This function can be used to get the keying material from the EAP
     * method. The key may already be stored in the method-specific private
     * data or this function may derive the key.
     */
    uint8_t * (*getKey)(struct eap_sm *sm, void *priv, size_t *len);

    /**
     * get_status - Get EAP method status
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     * @buf: Buffer for status information
     * @buflen: Maximum buffer length
     * @verbose: Whether to include verbose status information
     * Returns: Number of bytes written to buf
     *
     * Query EAP method for status information. This function fills in a
     * text area with current status information from the EAP method. If
     * the buffer (buf) is not large enough, status information will be
     * truncated to fit the buffer.
     */
    int (*get_status)(struct eap_sm *sm, void *priv, char *buf,
              size_t buflen, int verbose);

    /**
     * has_reauth_data - Whether method is ready for fast reauthentication
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     * Returns: %true or %false based on whether fast reauthentication is
     * possible
     *
     * This function is an optional handler that only EAP methods
     * supporting fast re-authentication need to implement.
     */
    bool (*has_reauth_data)(struct eap_sm *sm, void *priv);

    /**
     * deinit_for_reauth - Release data that is not needed for fast re-auth
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     *
     * This function is an optional handler that only EAP methods
     * supporting fast re-authentication need to implement. This is called
     * when authentication has been completed and EAP state machine is
     * requesting that enough state information is maintained for fast
     * re-authentication
     */
    void (*deinit_for_reauth)(struct eap_sm *sm, void *priv);

    /**
     * init_for_reauth - Prepare for start of fast re-authentication
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     *
     * This function is an optional handler that only EAP methods
     * supporting fast re-authentication need to implement. This is called
     * when EAP authentication is started and EAP state machine is
     * requesting fast re-authentication to be used.
     */
    void * (*init_for_reauth)(struct eap_sm *sm, void *priv);

    /**
     * get_identity - Get method specific identity for re-authentication
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     * @len: Length of the returned identity
     * Returns: Pointer to the method specific identity or %NULL if default
     * identity is to be used
     *
     * This function is an optional handler that only EAP methods
     * that use method specific identity need to implement.
     */
    const uint8_t * (*get_identity)(struct eap_sm *sm, void *priv, size_t *len);

    /**
     * get_error_code - Get the latest EAP method error code
     * @priv: Pointer to private EAP method data from eap_method::init()
     * Returns: An int for the EAP method specific error code if exists or
     * NO_EAP_METHOD_ERROR otherwise.
     *
     * This method is an optional handler that only EAP methods that need to
     * report their error code need to implement.
     */
    int (*get_error_code)(void *priv);

    /**
     * free - Free EAP method data
     * @method: Pointer to the method data registered with
     * eap_peer_method_register().
     *
     * This function will be called when the EAP method is being
     * unregistered. If the EAP method allocated resources during
     * registration (e.g., allocated struct eap_method), they should be
     * freed in this function. No other method functions will be called
     * after this call. If this function is not defined (i.e., function
     * pointer is %NULL), a default handler is used to release the method
     * data with free(method). This is suitable for most cases.
     */
    void (*free)(struct eap_method *method);

#define EAP_PEER_METHOD_INTERFACE_VERSION 1
    /**
     * version - Version of the EAP peer method interface
     *
     * The EAP peer method implementation should set this variable to
     * EAP_PEER_METHOD_INTERFACE_VERSION. This is used to verify that the
     * EAP method is using supported API version when using dynamically
     * loadable EAP methods.
     */
    int version;

    /**
     * next - Pointer to the next EAP method
     *
     * This variable is used internally in the EAP method registration code
     * to create a linked list of registered EAP methods.
     */
    struct eap_method *next;

#ifdef CONFIG_DYNAMIC_EAP_METHODS
    /**
     * dl_handle - Handle for the dynamic library
     *
     * This variable is used internally in the EAP method registration code
     * to store a handle for the dynamic library. If the method is linked
     * in statically, this is %NULL.
     */
    void *dl_handle;
#endif /* CONFIG_DYNAMIC_EAP_METHODS */

    /**
     * get_emsk - Get EAP method specific keying extended material (EMSK)
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     * @len: Pointer to a variable to store EMSK length
     * Returns: EMSK or %NULL if not available
     *
     * This function can be used to get the extended keying material from
     * the EAP method. The key may already be stored in the method-specific
     * private data or this function may derive the key.
     */
    uint8_t * (*get_emsk)(struct eap_sm *sm, void *priv, size_t *len);

    /**
     * getSessionId - Get EAP method specific Session-Id
     * @sm: Pointer to EAP state machine
     * @priv: Pointer to private EAP method data from eap_method::init()
     * @len: Pointer to a variable to store Session-Id length
     * Returns: Session-Id or %NULL if not available
     *
     * This function can be used to get the Session-Id from the EAP method.
     * The Session-Id may already be stored in the method-specific private
     * data or this function may derive the Session-Id.
     */
    uint8_t * (*getSessionId)(struct eap_sm *sm, void *priv, size_t *len);
};

struct eap_erp_key {
    struct list_head list;
    size_t rRK_len;
    size_t rIK_len;
    uint8_t rRK[ERP_MAX_KEY_LEN];
    uint8_t rIK[ERP_MAX_KEY_LEN];
    uint32_t next_seq;
    char keyname_nai[];
};

enum EAP_state_t {
    EAP_INITIALIZE, EAP_DISABLED, EAP_IDLE, EAP_RECEIVED,
    EAP_GET_METHOD, EAP_METHOD, EAP_SEND_RESPONSE, EAP_DISCARD,
    EAP_IDENTITY, EAP_NOTIFICATION, EAP_RETRANSMIT, EAP_SUCCESS,
    EAP_FAILURE
} ;

/**
 * struct eap_sm - EAP state machine data
 */
struct eap_sm {
    enum EAP_state_t EAP_state;
    /* Long-term local variables */
    enum eap_type selectedMethod;
    EapMethodState methodState;
    int lastId;
    struct wpabuf *lastRespData;
    EapDecision decision;
    /* Short-term local variables */
    bool rxReq;
    bool rxSuccess;
    bool rxFailure;
    int reqId;
    enum eap_type reqMethod;
    int reqVendor;
    uint32_t reqVendorMethod;
    bool ignore;
    /* Constants */
    int ClientTimeout;

    /* Miscellaneous variables */
    bool allowNotifications; /* peer state machine <-> methods */
    struct wpabuf *eapRespData; /* peer to lower layer */
    bool eapKeyAvailable; /* peer to lower layer */
    uint8_t *eapKeyData; /* peer to lower layer */
    size_t eapKeyDataLen; /* peer to lower layer */
    uint8_t *eapSessionId; /* peer to lower layer */
    size_t eapSessionIdLen; /* peer to lower layer */
    const struct eap_method *m; /* selected EAP method */
    /* not defined in RFC 4137 */
    bool changed;
    int vif_idx;
    void *eapol_sm;
    // const struct eapol_callbacks *eapol_cb;
    void *eap_method_priv;
    int init_phase2;
    int fast_reauth;
    bool reauthInit; /* send EAP-Identity/Re-auth */
    uint32_t erp_seq;

    bool rxResp /* LEAP only */;
    bool leap_done;
    bool peap_done;
    uint8_t req_sha1[20]; /* SHA1() of the current EAP packet */
    uint8_t last_sha1[20]; /* SHA1() of the previously received EAP packet; used
               * in duplicate request detection. */

    //void *msg_ctx;
    void *scard_ctx;
#ifdef CONFIG_EAP_TLS
    void *ssl_ctx;
    void *ssl_ctx2;
#endif

    uint32_t workaround;

    /* Optional challenges generated in Phase 1 (EAP-FAST) */
    uint8_t *peer_challenge, *auth_challenge;

    int num_rounds;
    int num_rounds_short;
    int force_disabled;

    struct wps_context *wps_ctx;
    struct eap_context *eap_ctx;
    //struct wps_config *wps_cfg;

    int prev_failure;
//    struct eap_peer_config *last_config;

    int external_sim;

    uint32_t expected_failure:1;
    uint32_t ext_cert_check:1;
    uint32_t waiting_ext_cert_check:1;
    uint32_t use_machine_cred:1;

};

const uint8_t * eap_get_config_identity(struct eap_sm *sm, size_t *len);
const uint8_t * eap_get_config_password(struct eap_sm *sm, size_t *len);
const uint8_t * eap_get_config_password2(struct eap_sm *sm, size_t *len, int *hash);
const uint8_t * eap_get_config_new_password(struct eap_sm *sm, size_t *len);
const uint8_t * eap_get_config_otp(struct eap_sm *sm, size_t *len);
void eap_clear_config_otp(struct eap_sm *sm);
const char * eap_get_config_phase1(struct eap_sm *sm);
const char * eap_get_config_phase2(struct eap_sm *sm);
int eap_get_config_fragment_size(struct eap_sm *sm);
struct eap_peer_config * eap_get_config(struct eap_sm *sm);
#ifdef USE_BLOB
void eap_set_config_blob(struct eap_sm *sm, struct wpa_config_blob *blob);
const struct wpa_config_blob *
    eap_get_config_blob(struct eap_sm *sm, const char *name);
#endif
// void eap_notify_pending(struct eap_sm *sm);

int eap_peer_method_register(struct eap_method *method);
struct eap_method * eap_peer_method_alloc(int version, int vendor,
                                            enum eap_type method,
                                            const char *name);

/*==============================================================*/
/* eap_peer/eap.h */
/*==============================================================*/
struct eap_sm;
#ifdef USE_BLOB
struct wpa_config_blob;
#endif
struct wpabuf;
struct tls_cert_data;

#ifdef IEEE8021X_EAPOL

/**
 * enum eapol_bool_var - EAPOL boolean state variables for EAP state machine
 *
 * These variables are used in the interface between EAP peer state machine and
 * lower layer. These are defined in RFC 4137, Sect. 4.1. Lower layer code is
 * expected to maintain these variables and register a callback functions for
 * EAP state machine to get and set the variables.
 */
enum eapol_bool_var {
    /**
     * EAPOL_eapSuccess - EAP SUCCESS state reached
     *
     * EAP state machine reads and writes this value.
     */
    EAPOL_eapSuccess,

    /**
     * EAPOL_eapRestart - Lower layer request to restart authentication
     *
     * Set to true in lower layer, false in EAP state machine.
     */
    EAPOL_eapRestart,

    /**
     * EAPOL_eapFail - EAP FAILURE state reached
     *
     * EAP state machine writes this value.
     */
    EAPOL_eapFail,

    /**
     * EAPOL_eapResp - Response to send
     *
     * Set to true in EAP state machine, false in lower layer.
     */
    EAPOL_eapResp,

    /**
     * EAPOL_eapNoResp - Request has been process; no response to send
     *
     * Set to true in EAP state machine, false in lower layer.
     */
    EAPOL_eapNoResp,

    /**
     * EAPOL_eapReq - EAP request available from lower layer
     *
     * Set to true in lower layer, false in EAP state machine.
     */
    EAPOL_eapReq,

    /**
     * EAPOL_portEnabled - Lower layer is ready for communication
     *
     * EAP state machines reads this value.
     */
    EAPOL_portEnabled,

    /**
     * EAPOL_altAccept - Alternate indication of success (RFC3748)
     *
     * EAP state machines reads this value.
     */
    EAPOL_altAccept,

    /**
     * EAPOL_altReject - Alternate indication of failure (RFC3748)
     *
     * EAP state machines reads this value.
     */
    EAPOL_altReject,

    /**
     * EAPOL_eapTriggerStart - EAP-based trigger to send EAPOL-Start
     *
     * EAP state machine writes this value.
     */
    EAPOL_eapTriggerStart
};

/**
 * enum eapol_int_var - EAPOL integer state variables for EAP state machine
 *
 * These variables are used in the interface between EAP peer state machine and
 * lower layer. These are defined in RFC 4137, Sect. 4.1. Lower layer code is
 * expected to maintain these variables and register a callback functions for
 * EAP state machine to get and set the variables.
 */
enum eapol_int_var {
    /**
     * EAPOL_idleWhile - Outside time for EAP peer timeout
     *
     * This integer variable is used to provide an outside timer that the
     * external (to EAP state machine) code must decrement by one every
     * second until the value reaches zero. This is used in the same way as
     * EAPOL state machine timers. EAP state machine reads and writes this
     * value.
     */
    EAPOL_idleWhile
};

struct eap_sm * eap_peer_sm_init(void *eapol_sm,
                    struct eap_context *eap_ctx,
                    struct wps_context *wps_ctx);
void eap_peer_sm_deinit(struct eap_sm *sm);
int eap_peer_sm_step(struct eap_sm *sm);
void eap_sm_abort(struct eap_sm *sm);
int eap_sm_get_status(struct eap_sm *sm, char *buf, size_t buflen,
              int verbose);
const char * eap_sm_get_method_name(struct eap_sm *sm);
struct wpabuf * eap_sm_buildIdentity(struct eap_sm *sm, int id, int encrypted);
void eap_sm_request_identity(struct eap_sm *sm);
void eap_sm_request_password(struct eap_sm *sm);
void eap_sm_request_new_password(struct eap_sm *sm);
void eap_sm_request_pin(struct eap_sm *sm);
void eap_sm_request_otp(struct eap_sm *sm, const char *msg, size_t msg_len);
void eap_sm_request_passphrase(struct eap_sm *sm);
void eap_sm_request_sim(struct eap_sm *sm, const char *req);
void eap_sm_notify_ctrl_attached(struct eap_sm *sm);
uint32_t eap_get_phase2_type(const char *name, int *vendor);
struct eap_method_type * eap_get_phase2_types(struct eap_peer_config *config,
                          size_t *count);
void eap_set_fast_reauth(struct eap_sm *sm, int enabled);
void eap_set_workaround(struct eap_sm *sm, unsigned int workaround);
void eap_set_force_disabled(struct eap_sm *sm, int disabled);
// void eap_set_external_sim(struct eap_sm *sm, int external_sim);
int eap_key_available(struct eap_sm *sm);
void eap_notify_success(struct eap_sm *sm);
void eap_notify_lower_layer_success(struct eap_sm *sm);
const uint8_t * eap_get_eapSessionId(struct eap_sm *sm, size_t *len);
const uint8_t * eap_get_eapKeyData(struct eap_sm *sm, size_t *len);
struct wpabuf * eap_get_eapRespData(struct eap_sm *sm);
// void eap_register_scard_ctx(struct eap_sm *sm, void *ctx);
void eap_invalidate_cached_session(struct eap_sm *sm);

#ifdef CONFIG_EXT_PASSWORD
struct ext_password_data;
void eap_sm_set_ext_pw_ctx(struct eap_sm *sm, struct ext_password_data *ext);
#endif
// void eap_set_anon_id(struct eap_sm *sm, const uint8_t *id, size_t len);
// int eap_peer_was_failure_expected(struct eap_sm *sm);
void eap_peer_erp_free_keys(struct eap_sm *sm);
struct wpabuf * eap_peer_build_erp_reauth_start(struct eap_sm *sm, uint8_t eap_id);
void eap_peer_finish(struct eap_sm *sm, const struct eap_hdr *hdr, size_t len);
int eap_peer_get_erp_info(struct eap_sm *sm, struct eap_peer_config *config,
              const uint8_t **username, size_t *username_len,
              const uint8_t **realm, size_t *realm_len, uint16_t *erp_seq_num,
              const uint8_t **rrk, size_t *rrk_len);
int eap_peer_update_erp_next_seq_num(struct eap_sm *sm, uint16_t seq_num);
void eap_peer_erp_init(struct eap_sm *sm, uint8_t *ext_session_id,
               size_t ext_session_id_len, uint8_t *ext_emsk,
               size_t ext_emsk_len);
void eap_peer_unregister_methods(void);

#endif /* IEEE8021X_EAPOL */

#endif /* WPAS_EAP_PEER_H */
