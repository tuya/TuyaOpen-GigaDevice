/*
 * Wi-Fi Protected Setup - device attributes
 * Copyright (c) 2008, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef WPAS_WPS_ATTR_H
#define WPAS_WPS_ATTR_H

struct wps_parse_attr
{
    /* fixed length fields */
    const uint8_t *version; /* 1 octet */
    const uint8_t *version2; /* 1 octet */
    const uint8_t *msg_type; /* 1 octet */
    const uint8_t *enrollee_nonce; /* WPS_NONCE_LEN (16) octets */
    const uint8_t *registrar_nonce; /* WPS_NONCE_LEN (16) octets */
    const uint8_t *uuid_r; /* WPS_UUID_LEN (16) octets */
    const uint8_t *uuid_e; /* WPS_UUID_LEN (16) octets */
    const uint8_t *auth_type_flags; /* 2 octets */
    const uint8_t *encr_type_flags; /* 2 octets */
    const uint8_t *conn_type_flags; /* 1 octet */
    const uint8_t *config_methods; /* 2 octets */
    const uint8_t *sel_reg_config_methods; /* 2 octets */
    const uint8_t *primary_dev_type; /* 8 octets */
    const uint8_t *rf_bands; /* 1 octet */
    const uint8_t *assoc_state; /* 2 octets */
    const uint8_t *config_error; /* 2 octets */
    const uint8_t *dev_password_id; /* 2 octets */
    const uint8_t *os_version; /* 4 octets */
    const uint8_t *wps_state; /* 1 octet */
    const uint8_t *authenticator; /* WPS_AUTHENTICATOR_LEN (8) octets */
    const uint8_t *r_hash1; /* WPS_HASH_LEN (32) octets */
    const uint8_t *r_hash2; /* WPS_HASH_LEN (32) octets */
    const uint8_t *e_hash1; /* WPS_HASH_LEN (32) octets */
    const uint8_t *e_hash2; /* WPS_HASH_LEN (32) octets */
    const uint8_t *r_snonce1; /* WPS_SECRET_NONCE_LEN (16) octets */
    const uint8_t *r_snonce2; /* WPS_SECRET_NONCE_LEN (16) octets */
    const uint8_t *e_snonce1; /* WPS_SECRET_NONCE_LEN (16) octets */
    const uint8_t *e_snonce2; /* WPS_SECRET_NONCE_LEN (16) octets */
    const uint8_t *key_wrap_auth; /* WPS_KWA_LEN (8) octets */
    const uint8_t *auth_type; /* 2 octets */
    const uint8_t *encr_type; /* 2 octets */
    const uint8_t *network_idx; /* 1 octet */
    const uint8_t *network_key_idx; /* 1 octet */
    const uint8_t *mac_addr; /* ETH_ALEN (6) octets */
    const uint8_t *selected_registrar; /* 1 octet (Bool) */
    const uint8_t *request_type; /* 1 octet */
    const uint8_t *response_type; /* 1 octet */
    const uint8_t *ap_setup_locked; /* 1 octet */
    const uint8_t *settings_delay_time; /* 1 octet */
    const uint8_t *network_key_shareable; /* 1 octet (Bool) */
    const uint8_t *request_to_enroll; /* 1 octet (Bool) */
    const uint8_t *ap_channel; /* 2 octets */
    const uint8_t *registrar_configuration_methods; /* 2 octets */

    /* variable length fields */
    const uint8_t *manufacturer;
    const uint8_t *model_name;
    const uint8_t *model_number;
    const uint8_t *serial_number;
    const uint8_t *dev_name;
    const uint8_t *public_key;
    const uint8_t *encr_settings;
    const uint8_t *ssid; /* <= 32 octets */
    const uint8_t *network_key; /* <= 64 octets */
    const uint8_t *authorized_macs; /* <= 30 octets */
    const uint8_t *sec_dev_type_list; /* <= 128 octets */
    const uint8_t *oob_dev_password; /* 38..54 octets */
    uint16_t manufacturer_len;
    uint16_t model_name_len;
    uint16_t model_number_len;
    uint16_t serial_number_len;
    uint16_t dev_name_len;
    uint16_t public_key_len;
    uint16_t encr_settings_len;
    uint16_t ssid_len;
    uint16_t network_key_len;
    uint16_t authorized_macs_len;
    uint16_t sec_dev_type_list_len;
    uint16_t oob_dev_password_len;

    /* attributes that can occur multiple times */
#define MAX_CRED_COUNT 10
#define MAX_REQ_DEV_TYPE_COUNT 10

    unsigned int num_cred;
    unsigned int num_req_dev_type;
    unsigned int num_vendor_ext;

    uint16_t cred_len[MAX_CRED_COUNT];
    uint16_t vendor_ext_len[MAX_WPS_PARSE_VENDOR_EXT];

    const uint8_t *cred[MAX_CRED_COUNT];
    const uint8_t *req_dev_type[MAX_REQ_DEV_TYPE_COUNT];
    const uint8_t *vendor_ext[MAX_WPS_PARSE_VENDOR_EXT];
    uint8_t multi_ap_ext;
};


/* wps_attr_build.c */
int wps_build_public_key(struct wps_data *wps, struct wpabuf *msg);
int wps_build_req_type(struct wpabuf *msg, enum wps_request_type type);
int wps_build_resp_type(struct wpabuf *msg, enum wps_response_type type);
int wps_build_config_methods(struct wpabuf *msg, uint16_t methods);
int wps_build_uuid_e(struct wpabuf *msg, const uint8_t *uuid);
int wps_build_dev_password_id(struct wpabuf *msg, uint16_t id);
int wps_build_config_error(struct wpabuf *msg, uint16_t err);
int wps_build_authenticator(struct wps_data *wps, struct wpabuf *msg);
int wps_build_key_wrap_auth(struct wps_data *wps, struct wpabuf *msg);
int wps_build_encr_settings(struct wps_data *wps, struct wpabuf *msg,
                struct wpabuf *plain);
int wps_build_version(struct wpabuf *msg);
int wps_build_wfa_ext(struct wpabuf *msg, int req_to_enroll,
              const uint8_t *auth_macs, size_t auth_macs_count,
              uint8_t multi_ap_subelem);
int wps_build_msg_type(struct wpabuf *msg, enum wps_msg_type msg_type);
int wps_build_enrollee_nonce(struct wps_data *wps, struct wpabuf *msg);
int wps_build_registrar_nonce(struct wps_data *wps, struct wpabuf *msg);
int wps_build_auth_type_flags(struct wps_data *wps, struct wpabuf *msg);
int wps_build_encr_type_flags(struct wps_data *wps, struct wpabuf *msg);
int wps_build_conn_type_flags(struct wps_data *wps, struct wpabuf *msg);
int wps_build_assoc_state(struct wps_data *wps, struct wpabuf *msg);
int wps_build_oob_dev_pw(struct wpabuf *msg, uint16_t dev_pw_id,
             const struct wpabuf *pubkey, const uint8_t *dev_pw,
             size_t dev_pw_len);
struct wpabuf * wps_ie_encapsulate(struct wpabuf *data);
int wps_build_mac_addr(struct wpabuf *msg, const uint8_t *addr);
int wps_build_rf_bands_attr(struct wpabuf *msg, uint8_t rf_bands);
int wps_build_ap_channel(struct wpabuf *msg, uint16_t ap_channel);

/* wps_dev_attr.c */
int wps_build_manufacturer(struct wps_device_data *dev, struct wpabuf *msg);
int wps_build_model_name(struct wps_device_data *dev, struct wpabuf *msg);
int wps_build_model_number(struct wps_device_data *dev, struct wpabuf *msg);
int wps_build_serial_number(struct wps_device_data *dev, struct wpabuf *msg);
int wps_build_dev_name(struct wps_device_data *dev, struct wpabuf *msg);
int wps_build_device_attrs(struct wps_device_data *dev, struct wpabuf *msg);
int wps_build_os_version(struct wps_device_data *dev, struct wpabuf *msg);
int wps_build_vendor_ext_m1(struct wps_device_data *dev, struct wpabuf *msg);
int wps_build_rf_bands(struct wps_device_data *dev, struct wpabuf *msg,
               uint8_t rf_band);
int wps_build_primary_dev_type(struct wps_device_data *dev,
                   struct wpabuf *msg);
int wps_build_secondary_dev_type(struct wps_device_data *dev,
                 struct wpabuf *msg);
int wps_build_dev_name(struct wps_device_data *dev, struct wpabuf *msg);
int wps_process_device_attrs(struct wps_device_data *dev,
                 struct wps_parse_attr *attr);
int wps_process_os_version(struct wps_device_data *dev, const uint8_t *ver);
void wps_process_vendor_ext_m1(struct wps_device_data *dev, const uint8_t ext);
int wps_process_rf_bands(struct wps_device_data *dev, const uint8_t *bands);
void wps_device_data_free(struct wps_device_data *dev);
int wps_build_vendor_ext(struct wps_device_data *dev, struct wpabuf *msg);
int wps_build_application_ext(struct wps_device_data *dev, struct wpabuf *msg);
int wps_build_req_dev_type(struct wps_device_data *dev, struct wpabuf *msg,
               unsigned int num_req_dev_types,
               const uint8_t *req_dev_types);

/* wps_attr_parse.c */
int wps_parse_msg(const struct wpabuf *msg, struct wps_parse_attr *attr);

/* wps_attr_process.c */
int wps_process_authenticator(struct wps_data *wps, const uint8_t *authenticator,
                  const struct wpabuf *msg);
int wps_process_key_wrap_auth(struct wps_data *wps, struct wpabuf *msg,
                  const uint8_t *key_wrap_auth);
int wps_process_cred(struct wps_parse_attr *attr,
             struct wps_credential *cred);
int wps_process_ap_settings(struct wps_parse_attr *attr,
                struct wps_credential *cred);

#endif /* WPAS_WPS_ATTR_H */
