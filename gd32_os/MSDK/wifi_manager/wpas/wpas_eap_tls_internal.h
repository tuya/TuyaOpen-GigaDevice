#include "wpas_includes.h"
#include "stdarg.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))
#endif

/***************************   tlsv1_common start  ****************************************/
#define TLS_VERSION_1   0x0301 /* TLSv1 */
#define TLS_VERSION_1_1 0x0302 /* TLSv1.1 */
#define TLS_VERSION_1_2 0x0303 /* TLSv1.2 */
#ifdef CONFIG_TLSV12
#define TLS_VERSION TLS_VERSION_1_2
#else /* CONFIG_TLSV12 */
#ifdef CONFIG_TLSV11
#define TLS_VERSION TLS_VERSION_1_1
#else /* CONFIG_TLSV11 */
#define TLS_VERSION TLS_VERSION_1
#endif /* CONFIG_TLSV11 */
#endif /* CONFIG_TLSV12 */
#define TLS_RANDOM_LEN              32
#define TLS_PRE_MASTER_SECRET_LEN   48
#define TLS_MASTER_SECRET_LEN       48
#define TLS_SESSION_ID_MAX_LEN      32
#define TLS_VERIFY_DATA_LEN         12

/* HandshakeType */
enum {
    TLS_HANDSHAKE_TYPE_HELLO_REQUEST        = 0,
    TLS_HANDSHAKE_TYPE_CLIENT_HELLO         = 1,
    TLS_HANDSHAKE_TYPE_SERVER_HELLO         = 2,
    TLS_HANDSHAKE_TYPE_NEW_SESSION_TICKET   = 4 /* RFC 4507 */,
    TLS_HANDSHAKE_TYPE_CERTIFICATE          = 11,
    TLS_HANDSHAKE_TYPE_SERVER_KEY_EXCHANGE  = 12,
    TLS_HANDSHAKE_TYPE_CERTIFICATE_REQUEST  = 13,
    TLS_HANDSHAKE_TYPE_SERVER_HELLO_DONE    = 14,
    TLS_HANDSHAKE_TYPE_CERTIFICATE_VERIFY   = 15,
    TLS_HANDSHAKE_TYPE_CLIENT_KEY_EXCHANGE  = 16,
    TLS_HANDSHAKE_TYPE_FINISHED             = 20,
    TLS_HANDSHAKE_TYPE_CERTIFICATE_URL      = 21 /* RFC 4366 */,
    TLS_HANDSHAKE_TYPE_CERTIFICATE_STATUS   = 22 /* RFC 4366 */
};

/* CipherSuite */
#define TLS_NULL_WITH_NULL_NULL                 0x0000 /* RFC 2246 */
#define TLS_RSA_WITH_NULL_MD5                   0x0001 /* RFC 2246 */
#define TLS_RSA_WITH_NULL_SHA                   0x0002 /* RFC 2246 */
#define TLS_RSA_EXPORT_WITH_RC4_40_MD5          0x0003 /* RFC 2246 */
#define TLS_RSA_WITH_RC4_128_MD5                0x0004 /* RFC 2246 */
#define TLS_RSA_WITH_RC4_128_SHA                0x0005 /* RFC 2246 */
#define TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5      0x0006 /* RFC 2246 */
#define TLS_RSA_WITH_IDEA_CBC_SHA               0x0007 /* RFC 2246 */
#define TLS_RSA_EXPORT_WITH_DES40_CBC_SHA       0x0008 /* RFC 2246 */
#define TLS_RSA_WITH_DES_CBC_SHA                0x0009 /* RFC 2246 */
#define TLS_RSA_WITH_3DES_EDE_CBC_SHA           0x000A /* RFC 2246 */
#define TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA    0x000B /* RFC 2246 */
#define TLS_DH_DSS_WITH_DES_CBC_SHA             0x000C /* RFC 2246 */
#define TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA        0x000D /* RFC 2246 */
#define TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA    0x000E /* RFC 2246 */
#define TLS_DH_RSA_WITH_DES_CBC_SHA             0x000F /* RFC 2246 */
#define TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA        0x0010 /* RFC 2246 */
#define TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA   0x0011 /* RFC 2246 */
#define TLS_DHE_DSS_WITH_DES_CBC_SHA            0x0012 /* RFC 2246 */
#define TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA       0x0013 /* RFC 2246 */
#define TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA   0x0014 /* RFC 2246 */
#define TLS_DHE_RSA_WITH_DES_CBC_SHA            0x0015 /* RFC 2246 */
#define TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA       0x0016 /* RFC 2246 */
#define TLS_DH_anon_EXPORT_WITH_RC4_40_MD5      0x0017 /* RFC 2246 */
#define TLS_DH_anon_WITH_RC4_128_MD5            0x0018 /* RFC 2246 */
#define TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA   0x0019 /* RFC 2246 */
#define TLS_DH_anon_WITH_DES_CBC_SHA            0x001A /* RFC 2246 */
#define TLS_DH_anon_WITH_3DES_EDE_CBC_SHA       0x001B /* RFC 2246 */
#define TLS_RSA_WITH_AES_128_CBC_SHA            0x002F /* RFC 3268 */
#define TLS_DH_DSS_WITH_AES_128_CBC_SHA         0x0030 /* RFC 3268 */
#define TLS_DH_RSA_WITH_AES_128_CBC_SHA         0x0031 /* RFC 3268 */
#define TLS_DHE_DSS_WITH_AES_128_CBC_SHA        0x0032 /* RFC 3268 */
#define TLS_DHE_RSA_WITH_AES_128_CBC_SHA        0x0033 /* RFC 3268 */
#define TLS_DH_anon_WITH_AES_128_CBC_SHA        0x0034 /* RFC 3268 */
#define TLS_RSA_WITH_AES_256_CBC_SHA            0x0035 /* RFC 3268 */
#define TLS_DH_DSS_WITH_AES_256_CBC_SHA         0x0036 /* RFC 3268 */
#define TLS_DH_RSA_WITH_AES_256_CBC_SHA         0x0037 /* RFC 3268 */
#define TLS_DHE_DSS_WITH_AES_256_CBC_SHA        0x0038 /* RFC 3268 */
#define TLS_DHE_RSA_WITH_AES_256_CBC_SHA        0x0039 /* RFC 3268 */
#define TLS_DH_anon_WITH_AES_256_CBC_SHA        0x003A /* RFC 3268 */
#define TLS_RSA_WITH_NULL_SHA256                0x003B /* RFC 5246 */
#define TLS_RSA_WITH_AES_128_CBC_SHA256         0x003C /* RFC 5246 */
#define TLS_RSA_WITH_AES_256_CBC_SHA256         0x003D /* RFC 5246 */
#define TLS_DH_DSS_WITH_AES_128_CBC_SHA256      0x003E /* RFC 5246 */
#define TLS_DH_RSA_WITH_AES_128_CBC_SHA256      0x003F /* RFC 5246 */
#define TLS_DHE_DSS_WITH_AES_128_CBC_SHA256     0x0040 /* RFC 5246 */
#define TLS_DHE_RSA_WITH_AES_128_CBC_SHA256     0x0067 /* RFC 5246 */
#define TLS_DH_DSS_WITH_AES_256_CBC_SHA256      0x0068 /* RFC 5246 */
#define TLS_DH_RSA_WITH_AES_256_CBC_SHA256      0x0069 /* RFC 5246 */
#define TLS_DHE_DSS_WITH_AES_256_CBC_SHA256     0x006A /* RFC 5246 */
#define TLS_DHE_RSA_WITH_AES_256_CBC_SHA256     0x006B /* RFC 5246 */
#define TLS_DH_anon_WITH_AES_128_CBC_SHA256     0x006C /* RFC 5246 */
#define TLS_DH_anon_WITH_AES_256_CBC_SHA256     0x006D /* RFC 5246 */

/* HashAlgorithm */
enum {
    TLS_HASH_ALG_NONE = 0,
    TLS_HASH_ALG_MD5 = 1,
    TLS_HASH_ALG_SHA1 = 2,
    TLS_HASH_ALG_SHA224 = 3,
    TLS_HASH_ALG_SHA256 = 4,
    TLS_HASH_ALG_SHA384 = 5,
    TLS_HASH_ALG_SHA512 = 6
};

/* SignatureAlgorithm */
enum {
    TLS_SIGN_ALG_ANONYMOUS = 0,
    TLS_SIGN_ALG_RSA = 1,
    TLS_SIGN_ALG_DSA = 2,
    TLS_SIGN_ALG_ECDSA = 3,
};

/* CompressionMethod */
#define TLS_COMPRESSION_NULL    0

struct tls_verify_hash {
    struct crypto_hash *md5_client;
    struct crypto_hash *sha1_client;
    struct crypto_hash *sha256_client;
    struct crypto_hash *md5_server;
    struct crypto_hash *sha1_server;
    struct crypto_hash *sha256_server;
    struct crypto_hash *md5_cert;
    struct crypto_hash *sha1_cert;
    struct crypto_hash *sha256_cert;
};

/* TLS Extensions */
#define TLS_EXT_SERVER_NAME                 0 /* RFC 4366 */
#define TLS_EXT_MAX_FRAGMENT_LENGTH         1 /* RFC 4366 */
#define TLS_EXT_CLIENT_CERTIFICATE_URL      2 /* RFC 4366 */
#define TLS_EXT_TRUSTED_CA_KEYS             3 /* RFC 4366 */
#define TLS_EXT_TRUNCATED_HMAC              4 /* RFC 4366 */
#define TLS_EXT_STATUS_REQUEST              5 /* RFC 4366 */
#define TLS_EXT_SIGNATURE_ALGORITHMS        13 /* RFC 5246 */
#define TLS_EXT_STATUS_REQUEST_V2           17 /* RFC 6961 */
#define TLS_EXT_SESSION_TICKET              35 /* RFC 4507 */

#define TLS_EXT_PAC_OPAQUE      TLS_EXT_SESSION_TICKET /* EAP-FAST terminology */

typedef enum {
    TLS_CIPHER_STREAM,
    TLS_CIPHER_BLOCK
} tls_cipher_type;

typedef enum {
    TLS_KEY_X_NULL,
    TLS_KEY_X_RSA,
    TLS_KEY_X_RSA_EXPORT,
    TLS_KEY_X_DH_DSS_EXPORT,
    TLS_KEY_X_DH_DSS,
    TLS_KEY_X_DH_RSA_EXPORT,
    TLS_KEY_X_DH_RSA,
    TLS_KEY_X_DHE_DSS_EXPORT,
    TLS_KEY_X_DHE_DSS,
    TLS_KEY_X_DHE_RSA_EXPORT,
    TLS_KEY_X_DHE_RSA,
    TLS_KEY_X_DH_anon_EXPORT,
    TLS_KEY_X_DH_anon
} tls_key_exchange;

typedef enum {
    TLS_CIPHER_NULL,
    TLS_CIPHER_RC4_40,
    TLS_CIPHER_RC4_128,
    TLS_CIPHER_RC2_CBC_40,
    TLS_CIPHER_IDEA_CBC,
    TLS_CIPHER_DES40_CBC,
    TLS_CIPHER_DES_CBC,
    TLS_CIPHER_3DES_EDE_CBC,
    TLS_CIPHER_AES_128_CBC,
    TLS_CIPHER_AES_256_CBC
} tls_cipher;

typedef enum {
    TLS_HASH_NULL,
    TLS_HASH_MD5,
    TLS_HASH_SHA,
    TLS_HASH_SHA256
} tls_hash;

struct tls_cipher_suite {
    uint16_t suite;
    tls_key_exchange key_exchange;
    tls_cipher cipher;
    tls_hash hash;
};

struct tls_cipher_data {
    tls_cipher cipher;
    tls_cipher_type type;
    size_t key_material;
    size_t expanded_key_material;
    size_t block_size; /* also iv_size */
    enum crypto_cipher_alg alg;
};

int tls_verify_hash_init(struct tls_verify_hash *verify);
void tls_verify_hash_free(struct tls_verify_hash *verify);
int tls_prf(uint16_t ver, const uint8_t *secret, size_t secret_len, const char *label,
            const uint8_t *seed, size_t seed_len, uint8_t *out, size_t outlen);
const struct tls_cipher_data * tls_get_cipher_data(tls_cipher cipher);
const struct tls_cipher_suite * tls_get_cipher_suite(uint16_t suite);

/***************************   tlsv1_common end    **********************************/

/* AlertLevel */
#define TLS_ALERT_LEVEL_WARNING 1
#define TLS_ALERT_LEVEL_FATAL 2

/* AlertDescription */
#define TLS_ALERT_CLOSE_NOTIFY                      0
#define TLS_ALERT_UNEXPECTED_MESSAGE                10
#define TLS_ALERT_BAD_RECORD_MAC                    20
#define TLS_ALERT_DECRYPTION_FAILED                 21
#define TLS_ALERT_RECORD_OVERFLOW                   22
#define TLS_ALERT_DECOMPRESSION_FAILURE             30
#define TLS_ALERT_HANDSHAKE_FAILURE                 40
#define TLS_ALERT_BAD_CERTIFICATE                   42
#define TLS_ALERT_UNSUPPORTED_CERTIFICATE           43
#define TLS_ALERT_CERTIFICATE_REVOKED               44
#define TLS_ALERT_CERTIFICATE_EXPIRED               45
#define TLS_ALERT_CERTIFICATE_UNKNOWN               46
#define TLS_ALERT_ILLEGAL_PARAMETER                 47
#define TLS_ALERT_UNKNOWN_CA                        48
#define TLS_ALERT_ACCESS_DENIED                     49
#define TLS_ALERT_DECODE_ERROR                      50
#define TLS_ALERT_DECRYPT_ERROR                     51
#define TLS_ALERT_EXPORT_RESTRICTION                60
#define TLS_ALERT_PROTOCOL_VERSION                  70
#define TLS_ALERT_INSUFFICIENT_SECURITY             71
#define TLS_ALERT_INTERNAL_ERROR                    80
#define TLS_ALERT_USER_CANCELED                     90
#define TLS_ALERT_NO_RENEGOTIATION                  100
#define TLS_ALERT_UNSUPPORTED_EXTENSION             110 /* RFC 4366 */
#define TLS_ALERT_CERTIFICATE_UNOBTAINABLE          111 /* RFC 4366 */
#define TLS_ALERT_UNRECOGNIZED_NAME                 112 /* RFC 4366 */
#define TLS_ALERT_BAD_CERTIFICATE_STATUS_RESPONSE   113 /* RFC 4366 */
#define TLS_ALERT_BAD_CERTIFICATE_HASH_VALUE        114 /* RFC 4366 */

/* ChangeCipherSpec */
enum {
    TLS_CHANGE_CIPHER_SPEC = 1
};

struct tls_config {
    const char *opensc_engine_path;
    const char *pkcs11_engine_path;
    const char *pkcs11_module_path;
    int fips_mode;
    int cert_in_cb;
#ifdef USE_OPENSSL
    const char *openssl_ciphers;
#endif
    unsigned int tls_session_lifetime;
    unsigned int crl_reload_interval;
    unsigned int tls_flags;

    void (*event_cb)(void *ctx, enum tls_event ev,
                     union tls_event_data *data);
    void *cb_ctx;
};

#define TLS_CONN_ALLOW_SIGN_RSA_MD5     BIT(0)
#define TLS_CONN_DISABLE_TIME_CHECKS    BIT(1)
#define TLS_CONN_DISABLE_SESSION_TICKET BIT(2)
#define TLS_CONN_REQUEST_OCSP           BIT(3)
#define TLS_CONN_REQUIRE_OCSP           BIT(4)
#define TLS_CONN_DISABLE_TLSv1_1        BIT(5)
#define TLS_CONN_DISABLE_TLSv1_2        BIT(6)
#define TLS_CONN_EAP_FAST               BIT(7)
#define TLS_CONN_DISABLE_TLSv1_0        BIT(8)
#define TLS_CONN_EXT_CERT_CHECK         BIT(9)
#define TLS_CONN_REQUIRE_OCSP_ALL       BIT(10)
#define TLS_CONN_SUITEB                 BIT(11)
#define TLS_CONN_SUITEB_NO_ECDH         BIT(12)
#define TLS_CONN_DISABLE_TLSv1_3        BIT(13)
#define TLS_CONN_ENABLE_TLSv1_0         BIT(14)
#define TLS_CONN_ENABLE_TLSv1_1         BIT(15)
#define TLS_CONN_ENABLE_TLSv1_2         BIT(16)
#define TLS_CONN_TEAP_ANON_DH           BIT(17)

/**
 * struct tls_connection_params - Parameters for TLS connection
 * @ca_cert: File or reference name for CA X.509 certificate in PEM or DER
 * format
 * @ca_cert_blob: ca_cert as inlined data or %NULL if not used
 * @ca_cert_blob_len: ca_cert_blob length
 * @ca_path: Path to CA certificates (OpenSSL specific)
 * @subject_match: String to match in the subject of the peer certificate or
 * %NULL to allow all subjects
 * @altsubject_match: String to match in the alternative subject of the peer
 * certificate or %NULL to allow all alternative subjects
 * @suffix_match: Semicolon deliminated string of values to suffix match against
 * the dNSName or CN of the peer certificate or %NULL to allow all domain names.
 * This may allow subdomains and wildcard certificates. Each domain name label
 * must have a full case-insensitive match.
 * @domain_match: String to match in the dNSName or CN of the peer
 * certificate or %NULL to allow all domain names. This requires a full,
 * case-insensitive match.
 *
 * More than one match string can be provided by using semicolons to
 * separate the strings (e.g., example.org;example.com). When multiple
 * strings are specified, a match with any one of the values is
 * considered a sufficient match for the certificate, i.e., the
 * conditions are ORed together.
 * @client_cert: File or reference name for client X.509 certificate in PEM or
 * DER format
 * @client_cert_blob: client_cert as inlined data or %NULL if not used
 * @client_cert_blob_len: client_cert_blob length
 * @private_key: File or reference name for client private key in PEM or DER
 * format (traditional format (RSA PRIVATE KEY) or PKCS#8 (PRIVATE KEY)
 * @private_key_blob: private_key as inlined data or %NULL if not used
 * @private_key_blob_len: private_key_blob length
 * @private_key_passwd: Passphrase for decrypted private key, %NULL if no
 * passphrase is used.
 * @dh_file: File name for DH/DSA data in PEM format, or %NULL if not used
 * @dh_blob: dh_file as inlined data or %NULL if not used
 * @dh_blob_len: dh_blob length
 * @engine: 1 = use engine (e.g., a smartcard) for private key operations
 * (this is OpenSSL specific for now)
 * @engine_id: engine id string (this is OpenSSL specific for now)
 * @ppin: pointer to the pin variable in the configuration
 * (this is OpenSSL specific for now)
 * @key_id: the private key's id when using engine (this is OpenSSL
 * specific for now)
 * @cert_id: the certificate's id when using engine
 * @ca_cert_id: the CA certificate's id when using engine
 * @openssl_ciphers: OpenSSL cipher configuration
 * @openssl_ecdh_curves: OpenSSL ECDH curve configuration. %NULL for auto if
 *    supported, empty string to disable, or a colon-separated curve list.
 * @flags: Parameter options (TLS_CONN_*)
 * @ocsp_stapling_response: DER encoded file with cached OCSP stapling response
 *    or %NULL if OCSP is not enabled
 * @ocsp_stapling_response_multi: DER encoded file with cached OCSP stapling
 *    response list (OCSPResponseList for ocsp_multi in RFC 6961) or %NULL if
 *    ocsp_multi is not enabled
 * @check_cert_subject: Client certificate subject name matching string
 *
 * TLS connection parameters to be configured with tls_connection_set_params()
 * and tls_global_set_params().
 *
 * Certificates and private key can be configured either as a reference name
 * (file path or reference to certificate store) or by providing the same data
 * as a pointer to the data in memory. Only one option will be used for each
 * field.
 */
struct tls_connection_params {
    const char *ca_cert;
    const char *ca_path;
    const char *subject_match;
    const char *altsubject_match;
    const char *suffix_match;
    const char *domain_match;
    const char *client_cert;
    const char *client_cert2;
    const char *private_key;
    const char *private_key2;
    const char *private_key_passwd;
    const char *private_key_passwd2;
    const char *dh_file;
#ifdef USE_BLOB
    const uint8_t *ca_cert_blob;
    size_t ca_cert_blob_len;
    const uint8_t *client_cert_blob;
    size_t client_cert_blob_len;
    const uint8_t *private_key_blob;
    size_t private_key_blob_len;
    const uint8_t *dh_blob;
    size_t dh_blob_len;
#endif

#ifdef USE_OPENSSL
    /* OpenSSL specific variables */
    int engine;
    const char *engine_id;
    const char *pin;
    const char *key_id;
    const char *cert_id;
    const char *ca_cert_id;
    const char *openssl_ciphers;
    const char *openssl_ecdh_curves;
#endif

    unsigned int flags;
    const char *ocsp_stapling_response;
    const char *ocsp_stapling_response_multi;
    const char *check_cert_subject;
};

void * tls_init(const struct tls_config *conf);

void tls_deinit(void *ssl_ctx);

/**
 * tls_connection_init - Initialize a new TLS connection
 * @tls_ctx: TLS context data from tls_init()
 * Returns: Connection context data, conn for other function calls
 */
struct tls_connection * tls_connection_init(void *tls_ctx);

/**
 * tls_connection_deinit - Free TLS connection data
 * @tls_ctx: TLS context data from tls_init()
 * @conn: Connection context data from tls_connection_init()
 *
 * Release all resources allocated for TLS connection.
 */
void tls_connection_deinit(void *tls_ctx, struct tls_connection *conn);
/**
 * tls_connection_set_params - Set TLS connection parameters
 * @tls_ctx: TLS context data from tls_init()
 * @conn: Connection context data from tls_connection_init()
 * @params: Connection parameters
 * Returns: 0 on success, -1 on failure,
 * TLS_SET_PARAMS_ENGINE_PRV_INIT_FAILED (-2) on error causing PKCS#11 engine
 * failure, or
 * TLS_SET_PARAMS_ENGINE_PRV_VERIFY_FAILED (-3) on failure to verify the
 * PKCS#11 engine private key, or
 * TLS_SET_PARAMS_ENGINE_PRV_BAD_PIN (-4) on PIN error causing PKCS#11 engine
 * failure.
 */
int tls_connection_set_params(void *tls_ctx, struct tls_connection *conn,
                            const struct tls_connection_params *params);
/**
 * tls_connection_resumed - Was session resumption used
 * @tls_ctx: TLS context data from tls_init()
 * @conn: Connection context data from tls_connection_init()
 * Returns: 1 if current session used session resumption, 0 if not
 */
int tls_connection_resumed(void *tls_ctx, struct tls_connection *conn);
/**
 * tls_get_cipher - Get current cipher name
 * @tls_ctx: TLS context data from tls_init()
 * @conn: Connection context data from tls_connection_init()
 * @buf: Buffer for the cipher name
 * @buflen: buf size
 * Returns: 0 on success, -1 on failure
 *
 * Get the name of the currently used cipher.
 */
int tls_get_cipher(void *tls_ctx, struct tls_connection *conn, char *buf, size_t buflen);

int tls_connection_established(void *tls_ctx, struct tls_connection *conn);

struct wpabuf * tls_connection_handshake(void *tls_ctx,
                     struct tls_connection *conn,
                     const struct wpabuf *in_data,
                     struct wpabuf **appl_data);

/**
 * tls_connection_shutdown - Shutdown TLS connection
 * @tls_ctx: TLS context data from tls_init()
 * @conn: Connection context data from tls_connection_init()
 * Returns: 0 on success, -1 on failure
 *
 * Shutdown current TLS connection without releasing all resources. New
 * connection can be started by using the same conn without having to call
 * tls_connection_init() or setting certificates etc. again. The new
 * connection should try to use session resumption.
 */
int tls_connection_shutdown(void *tls_ctx, struct tls_connection *conn);

/**
 * tls_connection_get_failed - Get connection failure status
 * @tls_ctx: TLS context data from tls_init()
 * @conn: Connection context data from tls_connection_init()
 *
 * Returns >0 if connection has failed, 0 if not.
 */
int tls_connection_get_failed(void *tls_ctx, struct tls_connection *conn);

int tls_connection_get_random(void *tls_ctx, struct tls_connection *conn, struct tls_random *data);

int tls_connection_export_key(void *tls_ctx, struct tls_connection *conn,
                  const char *label, const uint8_t *context,
                  size_t context_len, uint8_t *out, size_t out_len);

/**
 * tls_get_version - Get the current TLS version number
 * @tls_ctx: TLS context data from tls_init()
 * @conn: Connection context data from tls_connection_init()
 * @buf: Buffer for returning the TLS version number
 * @buflen: buf size
 * Returns: 0 on success, -1 on failure
 *
 * Get the currently used TLS version number.
 */
int __must_check tls_get_version(void *tls_ctx, struct tls_connection *conn,
                                char *buf, size_t buflen);

/**
 * tls_get_errors - Process pending errors
 * @tls_ctx: TLS context data from tls_init()
 * Returns: Number of found error, 0 if no errors detected.
 *
 * Process all pending TLS errors.
 */
int tls_get_errors(void *tls_ctx);

typedef int (*tls_session_ticket_cb)
    (void *ctx, const uint8_t *ticket, size_t len, const uint8_t *client_random,
    const uint8_t *server_random, uint8_t *master_secret);
/***************************   pkcs5 start  ****************************************/

struct pkcs5_params {
    enum pkcs5_alg {
        PKCS5_ALG_UNKNOWN,
        PKCS5_ALG_MD5_DES_CBC,
        PKCS5_ALG_PBES2,
        PKCS5_ALG_SHA1_3DES_CBC,
    } alg;
    uint8_t salt[64];
    size_t salt_len;
    unsigned int iter_count;
    enum pbes2_enc_alg {
        PBES2_ENC_ALG_UNKNOWN,
        PBES2_ENC_ALG_DES_EDE3_CBC,
    } enc_alg;
#ifdef CONFIG_GDWIFI
    enum pbkdf2_digest_alg {
        PBKDF2_DIG_ALG_UNKNOWN,
        PBKDF2_DIG_ALG_MD5,
        PBKDF2_DIG_ALG_SHA1,
        PBKDF2_DIG_ALG_SHA224,
        PBKDF2_DIG_ALG_SHA256,
        PBKDF2_DIG_ALG_SHA384,
        PBKDF2_DIG_ALG_SHA512,
    } dig_alg;
#endif /* CONFIG_GDWIFI */
    uint8_t iv[8];
    size_t iv_len;
};

/***************************   pkcs5 end    ****************************************/

/***************************   crypto_internal start  ****************************************/
struct crypto_public_key;
struct crypto_private_key;

struct crypto_hash {
    enum crypto_hash_alg alg;
    union {
        struct MD5Context md5;
        struct SHA1Context sha1;
#ifdef CONFIG_SHA256
        struct sha256_state sha256;
#endif /* CONFIG_SHA256 */
#ifdef CONFIG_INTERNAL_SHA384
        struct sha384_state sha384;
#endif /* CONFIG_INTERNAL_SHA384 */
#ifdef CONFIG_INTERNAL_SHA512
        struct sha512_state sha512;
#endif /* CONFIG_INTERNAL_SHA512 */
    } u;
    uint8_t key[64];
    size_t key_len;
};
/***************************   crypto_internal end    ****************************************/

/***************************   tlsv1_client_ocsp start ************************************/

enum tls_ocsp_result {
    TLS_OCSP_NO_RESPONSE, TLS_OCSP_INVALID, TLS_OCSP_GOOD, TLS_OCSP_REVOKED
};
/***************************   tlsv1_client_ocsp end   ************************************/

/***************************   tlsv1_record  start ****************************************/

#define TLS_MAX_WRITE_MAC_SECRET_LEN    32
#define TLS_MAX_WRITE_KEY_LEN           32
#define TLS_MAX_IV_LEN                  16
#define TLS_MAX_KEY_BLOCK_LEN           (2 * (TLS_MAX_WRITE_MAC_SECRET_LEN + \
                                        TLS_MAX_WRITE_KEY_LEN + TLS_MAX_IV_LEN)) // 160

#define TLS_SEQ_NUM_LEN         8
#define TLS_RECORD_HEADER_LEN   5

enum {
    TLS_CONTENT_TYPE_CHANGE_CIPHER_SPEC = 20,
    TLS_CONTENT_TYPE_ALERT              = 21,
    TLS_CONTENT_TYPE_HANDSHAKE          = 22,
    TLS_CONTENT_TYPE_APPLICATION_DATA   = 23
};

struct tlsv1_record_layer {
    uint16_t tls_version;

    uint8_t write_mac_secret[TLS_MAX_WRITE_MAC_SECRET_LEN];
    uint8_t read_mac_secret[TLS_MAX_WRITE_MAC_SECRET_LEN];
    uint8_t write_key[TLS_MAX_WRITE_KEY_LEN];
    uint8_t read_key[TLS_MAX_WRITE_KEY_LEN];
    uint8_t write_iv[TLS_MAX_IV_LEN];
    uint8_t read_iv[TLS_MAX_IV_LEN];

    size_t hash_size;
    size_t key_material_len;
    size_t iv_size; /* also block_size */

    enum crypto_hash_alg hash_alg;
    enum crypto_cipher_alg cipher_alg;

    uint8_t write_seq_num[TLS_SEQ_NUM_LEN];
    uint8_t read_seq_num[TLS_SEQ_NUM_LEN];

    uint16_t cipher_suite;
    uint16_t write_cipher_suite;
    uint16_t read_cipher_suite;

    struct crypto_cipher *write_cbc;
    struct crypto_cipher *read_cbc;
};
int tlsv1_record_send(struct tlsv1_record_layer *rl, uint8_t content_type, uint8_t *buf,
                        size_t buf_size, const uint8_t *payload, size_t payload_len,
                        size_t *out_len);
int tlsv1_record_set_cipher_suite(struct tlsv1_record_layer *rl,
                                    uint16_t cipher_suite);
int tlsv1_record_receive(struct tlsv1_record_layer *rl,
                const uint8_t *in_data, size_t in_len,
                uint8_t *out_data, size_t *out_len, uint8_t *alert);
int tlsv1_record_change_write_cipher(struct tlsv1_record_layer *rl);
int tlsv1_record_change_read_cipher(struct tlsv1_record_layer *rl);

/***************************   tlsv1_record  end   ****************************************/

/***************************   tlsv1_client start  ****************************************/

typedef int (*tlsv1_client_session_ticket_cb)(void *ctx, const uint8_t *ticket,     \
            size_t len, const uint8_t *client_random, const uint8_t *server_random, \
            uint8_t *master_secret);

struct tlsv1_client {
    enum {
        CLIENT_HELLO, SERVER_HELLO, SERVER_CERTIFICATE,
        SERVER_KEY_EXCHANGE, SERVER_CERTIFICATE_REQUEST,
        SERVER_HELLO_DONE, CLIENT_KEY_EXCHANGE, CHANGE_CIPHER_SPEC,
        SERVER_CHANGE_CIPHER_SPEC, SERVER_FINISHED, ACK_FINISHED,
        ESTABLISHED, FAILED
    } state;

    struct tlsv1_record_layer rl;

    uint8_t session_id[TLS_SESSION_ID_MAX_LEN];
    size_t session_id_len;
    uint8_t client_random[TLS_RANDOM_LEN];
    uint8_t server_random[TLS_RANDOM_LEN];
    uint8_t master_secret[TLS_MASTER_SECRET_LEN];

    uint8_t alert_level;
    uint8_t alert_description;

    unsigned int flags; /* TLS_CONN_* bitfield */

    unsigned int certificate_requested:1;
    unsigned int session_resumed:1;
    unsigned int session_ticket_included:1;
    unsigned int use_session_ticket:1;
    unsigned int cert_in_cb:1;
    unsigned int ocsp_resp_received:1;

    struct crypto_public_key *server_rsa_key;

    struct tls_verify_hash verify;

#define MAX_CIPHER_COUNT 30
    uint16_t cipher_suites[MAX_CIPHER_COUNT];
    size_t num_cipher_suites;

    uint16_t prev_cipher_suite;

    uint8_t *client_hello_ext;
    size_t client_hello_ext_len;

    /* The prime modulus used for Diffie-Hellman */
    uint8_t *dh_p;
    size_t dh_p_len;
    /* The generator used for Diffie-Hellman */
    uint8_t *dh_g;
    size_t dh_g_len;
    /* The server's Diffie-Hellman public value */
    uint8_t *dh_ys;
    size_t dh_ys_len;

    struct tlsv1_credentials *cred;

    tlsv1_client_session_ticket_cb session_ticket_cb;
    void *session_ticket_cb_ctx;

    struct wpabuf *partial_input;

    void (*event_cb)(void *ctx, enum tls_event ev,
             union tls_event_data *data);
    void *cb_ctx;

    struct x509_certificate *server_cert;
};

struct tlsv1_client;
void tlsv1_client_set_cb(struct tlsv1_client *conn, void (*event_cb)(void *ctx, enum tls_event ev,
                        union tls_event_data *data), void *cb_ctx, int cert_in_cb);
struct tlsv1_client * tlsv1_client_init(void);
void tlsv1_client_deinit(struct tlsv1_client *conn);
uint8_t * tlsv1_client_send_alert(struct tlsv1_client *conn, uint8_t level,
                                    uint8_t description, size_t *out_len);
int tlsv1_client_process_handshake(struct tlsv1_client *conn, uint8_t ct,
                                    const uint8_t *buf, size_t *len,
                                    uint8_t **out_data, size_t *out_len);
uint16_t tls_client_highest_ver(struct tlsv1_client *conn);

/***************************   tlsv1_client end    ****************************************/

/***************************   tlsv1_client_write start ****************************************/
uint8_t * tls_send_client_hello(struct tlsv1_client *conn, size_t *out_len);
uint8_t * tlsv1_client_handshake_write(struct tlsv1_client *conn, size_t *out_len,
                                        int no_appl_data);
/***************************   tlsv1_client_write end   ****************************************/

/***************************   tlsv1_cred end    ****************************************/
struct tlsv1_credentials {
    struct x509_certificate *trusted_certs;
    struct x509_certificate *cert;
    struct crypto_private_key *key;

    unsigned int cert_probe:1;
    unsigned int ca_cert_verify:1;
    unsigned int server_cert_only:1;
    uint8_t srv_cert_hash[32];

    /* Diffie-Hellman parameters */
    uint8_t *dh_p; /* prime */
    size_t dh_p_len;
    uint8_t *dh_g; /* generator */
    size_t dh_g_len;

    char *ocsp_stapling_response;
    char *ocsp_stapling_response_multi;
};

void tlsv1_cred_free(struct tlsv1_credentials *cred);

/***************************   tlsv1_cred end    ****************************************/

/***************************   crypto_internal-cipher start**************************************/
struct crypto_cipher {
    enum crypto_cipher_alg alg;
    union {
        struct {
            size_t used_bytes;
            uint8_t key[16];
            size_t keylen;
        } rc4;
        struct {
            uint8_t cbc[32];
            void *ctx_enc;
            void *ctx_dec;
        } aes;
        struct {
            struct des3_key_s key;
            uint8_t cbc[8];
        } des3;
        struct {
            uint32_t ek[32];
            uint32_t dk[32];
            uint8_t cbc[8];
        } des;
    } u;
};
/***************************   crypto_internal-cipher end**************************************/

/***************************   asn1  start         ****************************************/

#define ASN1_TAG_EOC                0x00 /* not used with DER */
#define ASN1_TAG_BOOLEAN            0x01
#define ASN1_TAG_INTEGER            0x02
#define ASN1_TAG_BITSTRING          0x03
#define ASN1_TAG_OCTETSTRING        0x04
#define ASN1_TAG_NULL               0x05
#define ASN1_TAG_OID                0x06
#define ASN1_TAG_OBJECT_DESCRIPTOR  0x07 /* not yet parsed */
#define ASN1_TAG_EXTERNAL           0x08 /* not yet parsed */
#define ASN1_TAG_REAL               0x09 /* not yet parsed */
#define ASN1_TAG_ENUMERATED         0x0A /* not yet parsed */
#define ASN1_TAG_EMBEDDED_PDV       0x0B /* not yet parsed */
#define ASN1_TAG_UTF8STRING         0x0C /* not yet parsed */
#define ANS1_TAG_RELATIVE_OID       0x0D
#define ASN1_TAG_TIME               0x0E
#define ASN1_TAG_SEQUENCE           0x10 /* shall be constructed */
#define ASN1_TAG_SET                0x11
#define ASN1_TAG_NUMERICSTRING      0x12 /* not yet parsed */
#define ASN1_TAG_PRINTABLESTRING    0x13
#define ASN1_TAG_T61STRING          0x14 /* not yet parsed */
#define ASN1_TAG_VIDEOTEXSTRING     0x15 /* not yet parsed */
#define ASN1_TAG_IA5STRING          0x16
#define ASN1_TAG_UTCTIME            0x17
#define ASN1_TAG_GENERALIZEDTIME    0x18 /* not yet parsed */
#define ASN1_TAG_GRAPHICSTRING      0x19 /* not yet parsed */
#define ASN1_TAG_VISIBLESTRING      0x1A
#define ASN1_TAG_GENERALSTRING      0x1B /* not yet parsed */
#define ASN1_TAG_UNIVERSALSTRING    0x1C /* not yet parsed */
#define ASN1_TAG_CHARACTERSTRING    0x1D /* not yet parsed */
#define ASN1_TAG_BMPSTRING          0x1E /* not yet parsed */

#define ASN1_CLASS_UNIVERSAL        0
#define ASN1_CLASS_APPLICATION      1
#define ASN1_CLASS_CONTEXT_SPECIFIC 2
#define ASN1_CLASS_PRIVATE          3

struct asn1_hdr {
    const uint8_t *payload;
    uint8_t identifier, class, constructed;
    unsigned int tag, length;
};

#define ASN1_MAX_OID_LEN 20

struct asn1_oid {
    unsigned long oid[ASN1_MAX_OID_LEN];
    size_t len;
};

static inline bool asn1_is_oid(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_OID;
}

static inline bool asn1_is_sequence(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_SEQUENCE;
}

static inline bool asn1_is_string_type(const struct asn1_hdr *hdr)
{
    if (hdr->class != ASN1_CLASS_UNIVERSAL || hdr->constructed)
        return false;
    return hdr->tag == ASN1_TAG_UTF8STRING       ||
            hdr->tag == ASN1_TAG_NUMERICSTRING   ||
            hdr->tag == ASN1_TAG_PRINTABLESTRING ||
            hdr->tag == ASN1_TAG_T61STRING       ||
            hdr->tag == ASN1_TAG_VIDEOTEXSTRING  ||
            hdr->tag == ASN1_TAG_IA5STRING       ||
            hdr->tag == ASN1_TAG_GRAPHICSTRING   ||
            hdr->tag == ASN1_TAG_VISIBLESTRING   ||
            hdr->tag == ASN1_TAG_GENERALSTRING   ||
            hdr->tag == ASN1_TAG_UNIVERSALSTRING ||
            hdr->tag == ASN1_TAG_CHARACTERSTRING ||
            hdr->tag == ASN1_TAG_BMPSTRING;
}

static inline bool asn1_is_bitstring(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_BITSTRING;
}

static inline bool asn1_is_boolean(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_BOOLEAN;
}

static inline bool asn1_is_octetstring(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_OCTETSTRING;
}

static inline bool asn1_is_integer(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_INTEGER;
}

static inline bool asn1_is_utctime(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_UTCTIME;
}

static inline bool asn1_is_generalizedtime(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_GENERALIZEDTIME;
}

static inline bool asn1_is_cs_tag(const struct asn1_hdr *hdr, unsigned int tag)
{
    return hdr->class == ASN1_CLASS_CONTEXT_SPECIFIC &&
        hdr->tag == tag;
}

static inline bool asn1_is_null(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_NULL;
}

static inline bool asn1_is_set(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_SET;
}

static inline bool asn1_is_enumerated(const struct asn1_hdr *hdr)
{
    return hdr->class == ASN1_CLASS_UNIVERSAL &&
        hdr->tag == ASN1_TAG_ENUMERATED;
}

/***************************   asn1  end           ****************************************/

#define random_get_bytes(b, l) sys_random_bytes_get((b), (l))

/***************************   x509v3  start      ****************************************/
struct x509_algorithm_identifier {
    struct asn1_oid oid;
};

struct x509_name_attr {
    enum x509_name_attr_type {
        X509_NAME_ATTR_NOT_USED,
        X509_NAME_ATTR_DC,
        X509_NAME_ATTR_CN,
        X509_NAME_ATTR_C,
        X509_NAME_ATTR_L,
        X509_NAME_ATTR_ST,
        X509_NAME_ATTR_O,
        X509_NAME_ATTR_OU
    } type;
    char *value;
};

#define X509_MAX_NAME_ATTRIBUTES    20

struct x509_name {
    struct x509_name_attr attr[X509_MAX_NAME_ATTRIBUTES];
    size_t num_attr;
    char *email; /* emailAddress */

    /* from alternative name extension */
    char *alt_email; /* rfc822Name */
    char *dns; /* dNSName */
    char *uri; /* uniformResourceIdentifier */
    uint8_t *ip; /* iPAddress */
    size_t ip_len; /* IPv4: 4, IPv6: 16 */
    struct asn1_oid rid; /* registeredID */
};

#define X509_MAX_SERIAL_NUM_LEN 20

struct x509_certificate {
    struct x509_certificate *next;
    enum { X509_CERT_V1 = 0, X509_CERT_V2 = 1, X509_CERT_V3 = 2 } version;
    uint8_t serial_number[X509_MAX_SERIAL_NUM_LEN];
    size_t serial_number_len;
    struct x509_algorithm_identifier signature;
    struct x509_name issuer;
    struct x509_name subject;
    uint8_t *subject_dn;
    size_t subject_dn_len;
    os_time_t not_before;
    os_time_t not_after;
    struct x509_algorithm_identifier public_key_alg;
    uint8_t *public_key;
    size_t public_key_len;
    struct x509_algorithm_identifier signature_alg;
    uint8_t *sign_value;
    size_t sign_value_len;

    /* Extensions */
    unsigned int extensions_present;
#define X509_EXT_BASIC_CONSTRAINTS      (1 << 0)
#define X509_EXT_PATH_LEN_CONSTRAINT    (1 << 1)
#define X509_EXT_KEY_USAGE              (1 << 2)
#define X509_EXT_SUBJECT_ALT_NAME       (1 << 3)
#define X509_EXT_ISSUER_ALT_NAME        (1 << 4)
#define X509_EXT_EXT_KEY_USAGE          (1 << 5)
#define X509_EXT_CERTIFICATE_POLICY     (1 << 6)

    /* BasicConstraints */
    int ca; /* cA */
    unsigned long path_len_constraint; /* pathLenConstraint */

    /* KeyUsage */
    unsigned long key_usage;
#define X509_KEY_USAGE_DIGITAL_SIGNATURE    (1 << 0)
#define X509_KEY_USAGE_NON_REPUDIATION      (1 << 1)
#define X509_KEY_USAGE_KEY_ENCIPHERMENT     (1 << 2)
#define X509_KEY_USAGE_DATA_ENCIPHERMENT    (1 << 3)
#define X509_KEY_USAGE_KEY_AGREEMENT        (1 << 4)
#define X509_KEY_USAGE_KEY_CERT_SIGN        (1 << 5)
#define X509_KEY_USAGE_CRL_SIGN             (1 << 6)
#define X509_KEY_USAGE_ENCIPHER_ONLY        (1 << 7)
#define X509_KEY_USAGE_DECIPHER_ONLY        (1 << 8)

    /* ExtKeyUsage */
    unsigned long ext_key_usage;
#define X509_EXT_KEY_USAGE_ANY          (1 << 0)
#define X509_EXT_KEY_USAGE_SERVER_AUTH  (1 << 1)
#define X509_EXT_KEY_USAGE_CLIENT_AUTH  (1 << 2)
#define X509_EXT_KEY_USAGE_OCSP         (1 << 3)

    /* CertificatePolicy */
    unsigned long certificate_policy;
#define X509_EXT_CERT_POLICY_ANY        (1 << 0)
#define X509_EXT_CERT_POLICY_TOD_STRICT (1 << 1)
#define X509_EXT_CERT_POLICY_TOD_TOFU   (1 << 2)

    /*
     * The DER format certificate follows struct x509_certificate. These
     * pointers point to that buffer.
     */
    const uint8_t *cert_start;
    size_t cert_len;
    const uint8_t *tbs_cert_start;
    size_t tbs_cert_len;

    /* Meta data used for certificate validation */
    unsigned int ocsp_good:1;
    unsigned int ocsp_revoked:1;
    unsigned int issuer_trusted:1;
};

enum {
    X509_VALIDATE_OK,
    X509_VALIDATE_BAD_CERTIFICATE,
    X509_VALIDATE_UNSUPPORTED_CERTIFICATE,
    X509_VALIDATE_CERTIFICATE_REVOKED,
    X509_VALIDATE_CERTIFICATE_EXPIRED,
    X509_VALIDATE_CERTIFICATE_UNKNOWN,
    X509_VALIDATE_UNKNOWN_CA
};
int x509_check_signature(struct x509_certificate *issuer,
                struct x509_algorithm_identifier *signature,
                const uint8_t *sign_value, size_t sign_value_len,
                const uint8_t *signed_data, size_t signed_data_len);
/***************************   x509v3  end   ****************************************/

struct bignum;

/***************************   rsa start           **************************************/
struct crypto_rsa_key {
    int private_key; /* whether private key is set */
    struct bignum *n; /* modulus (p * q) */
    struct bignum *e; /* public exponent */
    /* The following parameters are available only if private_key is set */
    struct bignum *d; /* private exponent */
    struct bignum *p; /* prime p (factor of n) */
    struct bignum *q; /* prime q (factor of n) */
    struct bignum *dmp1; /* d mod (p - 1); CRT exponent */
    struct bignum *dmq1; /* d mod (q - 1); CRT exponent */
    struct bignum *iqmp; /* 1 / q mod p; CRT coefficient */
};
size_t crypto_rsa_get_modulus_len(struct crypto_rsa_key *key);
void crypto_rsa_free(struct crypto_rsa_key *key);
struct crypto_rsa_key *
crypto_rsa_import_private_key(const uint8_t *buf, size_t len);
int crypto_rsa_exptmod(const uint8_t *in, size_t inlen, uint8_t *out, size_t *outlen,
                        struct crypto_rsa_key *key, int use_private);
/***************************   rsa end             **************************************/
