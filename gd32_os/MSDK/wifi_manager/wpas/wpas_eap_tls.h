/*
 * EAP peer: EAP-TLS/PEAP/TTLS/FAST common functions
 * Copyright (c) 2004-2009, 2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef EAP_TLS_COMMON_H
#define EAP_TLS_COMMON_H

struct tls_random {
    const uint8_t *client_random;
    size_t client_random_len;
    const uint8_t *server_random;
    size_t server_random_len;
};

enum tls_event {
    TLS_CERT_CHAIN_SUCCESS,
    TLS_CERT_CHAIN_FAILURE,
    TLS_PEER_CERTIFICATE,
    TLS_ALERT
};

/*
 * Note: These are used as identifier with external programs and as such, the
 * values must not be changed.
 */
enum tls_fail_reason {
    TLS_FAIL_UNSPECIFIED = 0,
    TLS_FAIL_UNTRUSTED = 1,
    TLS_FAIL_REVOKED = 2,
    TLS_FAIL_NOT_YET_VALID = 3,
    TLS_FAIL_EXPIRED = 4,
    TLS_FAIL_SUBJECT_MISMATCH = 5,
    TLS_FAIL_ALTSUBJECT_MISMATCH = 6,
    TLS_FAIL_BAD_CERTIFICATE = 7,
    TLS_FAIL_SERVER_CHAIN_PROBE = 8,
    TLS_FAIL_DOMAIN_SUFFIX_MISMATCH = 9,
    TLS_FAIL_DOMAIN_MISMATCH = 10,
    TLS_FAIL_INSUFFICIENT_KEY_LEN = 11,
    TLS_FAIL_DN_MISMATCH = 12,
};

#define TLS_MAX_ALT_SUBJECT 10

struct tls_cert_data {
    int depth;
    const char *subject;
    const struct wpabuf *cert;
    const uint8_t *hash;
    size_t hash_len;
    const char *altsubject[TLS_MAX_ALT_SUBJECT];
    int num_altsubject;
    const char *serial_num;
    int tod;
};

union tls_event_data {
    struct {
        int depth;
        const char *subject;
        enum tls_fail_reason reason;
        const char *reason_txt;
        const struct wpabuf *cert;
    } cert_fail;

    struct tls_cert_data peer_cert;

    struct {
        int is_local;
        const char *type;
        const char *description;
    } alert;
};

/**
 * struct eap_ssl_data - TLS data for EAP methods
 */
struct eap_ssl_data {
    /**
     * conn - TLS connection context data from tls_connection_init()
     */
    struct tls_connection *conn;

    /**
     * tls_out - TLS message to be sent out in fragments
     */
    struct wpabuf *tls_out;

    /**
     * tls_out_pos - The current position in the outgoing TLS message
     */
    size_t tls_out_pos;

    /**
     * tls_out_limit - Maximum fragment size for outgoing TLS messages
     */
    size_t tls_out_limit;

    /**
     * tls_in - Received TLS message buffer for re-assembly
     */
    struct wpabuf *tls_in;

    /**
     * tls_in_left - Number of remaining bytes in the incoming TLS message
     */
    size_t tls_in_left;

    /**
     * tls_in_total - Total number of bytes in the incoming TLS message
     */
    size_t tls_in_total;

    /**
     * phase2 - Whether this TLS connection is used in EAP phase 2 (tunnel)
     */
    int phase2;

    /**
     * include_tls_length - Whether the TLS length field is included even
     * if the TLS data is not fragmented
     */
    int include_tls_length;

    /**
     * eap - EAP state machine allocated with eap_peer_sm_init()
     */
    struct eap_sm *eap;

    /**
     * ssl_ctx - TLS library context to use for the connection
     */
    void *ssl_ctx;

    /**
     * eap_type - EAP method used in Phase 1
     * (EAP_TYPE_TLS/PEAP/TTLS/FAST/TEAP)
     */
    uint8_t eap_type;

    /**
     * tls_v13 - Whether TLS v1.3 or newer is used
     */
    int tls_v13;
};

enum {
    TLS_SET_PARAMS_ENGINE_PRV_BAD_PIN = -4,
    TLS_SET_PARAMS_ENGINE_PRV_VERIFY_FAILED = -3,
    TLS_SET_PARAMS_ENGINE_PRV_INIT_FAILED = -2
};

enum {
    TLS_CIPHER_NONE,
    TLS_CIPHER_RC4_SHA /* 0x0005 */,
    TLS_CIPHER_AES128_SHA /* 0x002f */,
    TLS_CIPHER_RSA_DHE_AES128_SHA /* 0x0031 */,
    TLS_CIPHER_ANON_DH_AES128_SHA /* 0x0034 */,
    TLS_CIPHER_RSA_DHE_AES256_SHA /* 0x0039 */,
    TLS_CIPHER_AES256_SHA /* 0x0035 */,
};

/* EAP TLS Flags */
#define EAP_TLS_FLAGS_LENGTH_INCLUDED 0x80
#define EAP_TLS_FLAGS_MORE_FRAGMENTS 0x40
#define EAP_TLS_FLAGS_START 0x20
#define EAP_TEAP_FLAGS_OUTER_TLV_LEN 0x10
#define EAP_TLS_VERSION_MASK 0x07

 /* could be up to 128 bytes, but only the first 64 bytes are used */
#define EAP_TLS_KEY_LEN 64

/* stub type used as a flag for UNAUTH-TLS */
#define EAP_UNAUTH_TLS_TYPE 255
#define EAP_WFA_UNAUTH_TLS_TYPE 254

int eap_peer_tls_register(void);

int eap_peer_tls_ssl_init(struct eap_sm *sm, struct eap_ssl_data *data,
                        struct eap_context *eap_ctx, uint8_t eap_type);
void eap_peer_tls_ssl_deinit(struct eap_sm *sm, struct eap_ssl_data *data);
uint8_t * eap_peer_tls_derive_key(struct eap_sm *sm, struct eap_ssl_data *data,
                                const char *label, const uint8_t *context,
                                size_t context_len, size_t len);
uint8_t * eap_peer_tls_derive_session_id(struct eap_sm *sm,
                                struct eap_ssl_data *data, uint8_t eap_type,
                                size_t *len);
int eap_peer_tls_process_helper(struct eap_sm *sm, struct eap_ssl_data *data,
                                enum eap_type eap_type, int peap_version,
                                uint8_t id, const struct wpabuf *in_data,
                                struct wpabuf **out_data);
struct wpabuf * eap_peer_tls_build_ack(uint8_t id, enum eap_type eap_type,
                                int peap_version);
int eap_peer_tls_reauth_init(struct eap_sm *sm, struct eap_ssl_data *data);
int eap_peer_tls_status(struct eap_sm *sm, struct eap_ssl_data *data,
                        char *buf, size_t buflen, int verbose);
const uint8_t * eap_peer_tls_process_init(struct eap_sm *sm,
                                struct eap_ssl_data *data,
                                enum eap_type eap_type,
                                struct eap_method_ret *ret,
                                const struct wpabuf *reqData,
                                size_t *len, uint8_t *flags);
void eap_peer_tls_reset_input(struct eap_ssl_data *data);
void eap_peer_tls_reset_output(struct eap_ssl_data *data);
int eap_peer_tls_decrypt(struct eap_sm *sm, struct eap_ssl_data *data,
                        const struct wpabuf *in_data,
                        struct wpabuf **in_decrypted);
int eap_peer_tls_encrypt(struct eap_sm *sm, struct eap_ssl_data *data,
                        enum eap_type eap_type, int peap_version, uint8_t id,
                        const struct wpabuf *in_data,
                        struct wpabuf **out_data);

#endif /* EAP_TLS_COMMON_H */
