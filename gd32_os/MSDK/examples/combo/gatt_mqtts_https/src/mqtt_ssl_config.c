/*!
    \file    mqtt_ssl_config.c
    \brief   MQTT ssl shell config for GD32VW55x WiFi SDK

    \version 2023-10-27, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2021, GigaDevice Semiconductor Inc.

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

#include "lwip/altcp_tls.h"

static const char ca[] =
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIDbzCCAlcCFGQclt1+JCi4oDu68EDyXIm0KJ9OMA0GCSqGSIb3DQEBCwUAMHQx\r\n" \
"CzAJBgNVBAYTAkNOMQswCQYDVQQIDAJITjELMAkGA1UEBwwCQ1MxDzANBgNVBAoM\r\n" \
"BkZOTElOSzELMAkGA1UECwwCRk4xCzAJBgNVBAMMAlRTMSAwHgYJKoZIhvcNAQkB\r\n" \
"FhFlbWFpbEBleGFtcGxlLmNvbTAeFw0yMzA5MjcxMTAwMjhaFw0zMzA5MjQxMTAw\r\n" \
"MjhaMHQxCzAJBgNVBAYTAkNOMQswCQYDVQQIDAJITjELMAkGA1UEBwwCQ1MxDzAN\r\n" \
"BgNVBAoMBkZOTElOSzELMAkGA1UECwwCRk4xCzAJBgNVBAMMAlRTMSAwHgYJKoZI\r\n" \
"hvcNAQkBFhFlbWFpbEBleGFtcGxlLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEP\r\n" \
"ADCCAQoCggEBALRF35NLDDFDw9HV/AWxvsi5mz2kdGfSTtrasPAUJDGjbgg4PBvZ\r\n" \
"4U2XDxH90i2reOBcuxeTFngkBENtvtkEKCkbcD3UPICNoQunwKW1R2B3OBk9M1xm\r\n" \
"Km4pd9XZlRl7d6NSnM0WEsShpCePtDFkwJYiHKGsMDIiAYWOS4twRS8larydw1bV\r\n" \
"DvMP6wsnpSJIRv3MtyiifmnHCZk9NnHk3r1iBIcJWVOhnPdUto6MKNWm6Iqz8op7\r\n" \
"XkykRjvoGbo3vUCsvlI+I4qCE486dl7/C8BLSnga+nv2VtCKGmznGMS28ztBibXY\r\n" \
"GCR2K7EjbaqUmaJqjs44jehppie5hzEy9GECAwEAATANBgkqhkiG9w0BAQsFAAOC\r\n" \
"AQEAYgk7efzakip6v//469e3wQDZ5IAZemr2AIyxCp7dxSLO2AMPVK455xWiQp3R\r\n" \
"Ko2u2o8EIOd7jdh5yDUl/La/LFTeJAJVqzpXKRaU7gHEBmGDd7anCHE8nnlQ8XlB\r\n" \
"law8wW0zgTz0A3sP88+WNQ6lg+oSkMytlZItydqWLgdj22gmJlixgdt0fmUYoW02\r\n" \
"68uKveaacdxXpygGIU4VphoQM4mbfOl5H1qkmKFLqA5aGS8pl0Kf20sm1BYmpnLI\r\n" \
"r2BfCKn2uQn+fQBKu0Zv221/rXdlWkXA8stgL0gTb4r4QrKTv7cjTr//RO7z9GHX\r\n" \
"rVpPGbOEhZgZpUkWkFU7CQxakg==\r\n" \
"-----END CERTIFICATE-----\r\n";

static const char client_crt[] =
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIDdzCCAl8CFGC+YgH4GxeJuZjCmue/4yfUBZgcMA0GCSqGSIb3DQEBCwUAMHQx\r\n" \
"CzAJBgNVBAYTAkNOMQswCQYDVQQIDAJITjELMAkGA1UEBwwCQ1MxDzANBgNVBAoM\r\n" \
"BkZOTElOSzELMAkGA1UECwwCRk4xCzAJBgNVBAMMAlRTMSAwHgYJKoZIhvcNAQkB\r\n" \
"FhFlbWFpbEBleGFtcGxlLmNvbTAeFw0yMzA5MjgwNTQ5MjdaFw0zMzA5MjUwNTQ5\r\n" \
"MjdaMHwxCzAJBgNVBAYTAkNOMQswCQYDVQQIDAJaSjELMAkGA1UEBwwCSFoxDzAN\r\n" \
"BgNVBAoMBkZOTElOSzEOMAwGA1UECwwFRGVsdGExDTALBgNVBAMMBE5hbWUxIzAh\r\n" \
"BgkqhkiG9w0BCQEWFHpoYW5nc2FuQGV4YW1wbGUuY29tMIIBIjANBgkqhkiG9w0B\r\n" \
"AQEFAAOCAQ8AMIIBCgKCAQEAzAFYTnyDrOLkJdvV+d+iIknTTF5P89n55z6F2lCS\r\n" \
"AK1KPb2+B0hT6rdFExNaxtDodYo3tkc5+QabGPxB4h338dPK6e6pjPN59xTa+wJ0\r\n" \
"Rnhn83iv9OqSwEIpml7lSpQRZ9xrHxbANAQyg/4RtYmBI9UVsdtsZhgDhvTeJx1M\r\n" \
"ofpCZ4bbBtkSl8tXlSZc/QXZ0CzSZHQKOeJ7bJqE4ChkhozO7XLhBiro+sYGnyBA\r\n" \
"pbI21V5mqNprPzCQhckWFr+HUnHCb9aB0sJwjW/KXkdb/AqlumE3gYhQZScCCsu4\r\n" \
"27s0jtH22tqgagOOIdB8WLXZKYgsiSfS4hElJAvLDFbjKwIDAQABMA0GCSqGSIb3\r\n" \
"DQEBCwUAA4IBAQCNmuc74O2xtBHuVXSI7a2+dvWzBJBb06ncsKB2CSpgIy9sHJJQ\r\n" \
"wSOpkEREZyhHVQZ51upPY/RAevigSs4rz8rN1Ko6swHPKWPDdb9T54NxcX0mU8e5\r\n" \
"oYkDNayv9Xf0dzNoa4eI+dI6nPAPpKuMtEq/E5bILI9PX8zEIEebsUU47r5QLB1k\r\n" \
"HF8oxscke7tC5VZZHHsgkb4xZUmUFZtAejEI95rmS5arCLZoiLpvf3eteOJ3fk6A\r\n" \
"f8FgvLK+zuiiwYn3bnZm4S/aHlqFqU9DTePO7UOL9LsQEdXf2ING74MhkHvLEaF/\r\n" \
"1dYM1aJ3IDQ08Gxr2itiwGj2Ofl3hVdUFcw0\r\n" \
"-----END CERTIFICATE-----\r\n";

static const char client_private_key[] =
"-----BEGIN RSA PRIVATE KEY-----\r\n" \
"MIIEpAIBAAKCAQEAzAFYTnyDrOLkJdvV+d+iIknTTF5P89n55z6F2lCSAK1KPb2+\r\n" \
"B0hT6rdFExNaxtDodYo3tkc5+QabGPxB4h338dPK6e6pjPN59xTa+wJ0Rnhn83iv\r\n" \
"9OqSwEIpml7lSpQRZ9xrHxbANAQyg/4RtYmBI9UVsdtsZhgDhvTeJx1MofpCZ4bb\r\n" \
"BtkSl8tXlSZc/QXZ0CzSZHQKOeJ7bJqE4ChkhozO7XLhBiro+sYGnyBApbI21V5m\r\n" \
"qNprPzCQhckWFr+HUnHCb9aB0sJwjW/KXkdb/AqlumE3gYhQZScCCsu427s0jtH2\r\n" \
"2tqgagOOIdB8WLXZKYgsiSfS4hElJAvLDFbjKwIDAQABAoIBADV7QMhKskZ3sPIP\r\n" \
"4NfB/gJZMlC6BOHbyj0KUhL6vlv5EhZ/kLreBknpQ+2NTGYEzHxMAPEDWTpktfJl\r\n" \
"52u9CYxXRsHvNHnkNpxE1meprAvxcucMQ6zEdjZ64ec7a4cSrEF7MoYS885vL0MV\r\n" \
"L1VVOArJyQc4jAFz2DDgLwE4GCf6OHLhoDkBFH+yX+Gr4gtaST84IuFA0DdF/SJ2\r\n" \
"DP4ZB8lPDxGKVGYKYskp9BiOvzWmiEFgfMO4LEoOOdTZULbuNgX+Ocb94po8bgmI\r\n" \
"TPInrBbl3e7K6VqfUv6OwALkCTDIvFTak+t2XN9r/bDy/uvClidveq9OznL8a6nd\r\n" \
"PP+wbFECgYEA+ev3429t5cLPA8qkDgrPkq1dmNFnT8edQFDPN/tvTndG/KlYAxpB\r\n" \
"2s/UgmDYKeLe4Qlnn+kShazAPEhriWcfd6pk+2EEmePB9ZuIV3IRsLR8EeK7LjjN\r\n" \
"3pp+9dvrDqiuyqLc5NIQEIAftgDpb6ZmHN2fQyA5H+jyq3WxMa9ZopMCgYEA0Pd/\r\n" \
"PvQ2ZCUPpOv+RGh/59d/jVox+MXEm0Cy+fyT7UXW6edYa6GyHveKq/HUw9XqoCKV\r\n" \
"IiTOFIizh9KxXpxrhhhN3W9J6wfxvWpK4rZDBBdLwLJuJLy3UjhLt7RWercYUTkr\r\n" \
"4G5Yv+U6cUIxt2G5Rliref1Q5Rm1KoMdc2YOpAkCgYAL4vI2Sf1zGRHZf1DQLilz\r\n" \
"M3WpKAR+4eKTxYQliHrhw4CH0dISy7PoIUxdKxf83gJ1t4nLLD2qR+4Yr+UVcucn\r\n" \
"PN5yqkmavB9kfBI+/nji66KfAyad6yu3c6eoL959Iv61OovBAlCrSDy53k879TEU\r\n" \
"HGJPwf+VifHIv98+mrVrqwKBgQCJJfcw1U2heqWC45SVR1SzUD+aAVmM4hgAoX0W\r\n" \
"D4zzDIRaLgldUtm581PjOCwhgBOmmCvzJd7PoUzbVxAsfVMgxnDMZY9JH4Ssgce2\r\n" \
"dlMTq0p/pVwe5nEXGHWxkz2y+tqN7Iz8ls/O2l76GKzSo+Pa5LVeskBWrQG3bih1\r\n" \
"JcV0KQKBgQCuKHfOdmGXe6wpjXnyQb5W4Ray8GovS/AZiwu2+PrTz8L3+xSAVFnH\r\n" \
"gs4yRtb9bOGriXIm/P03swsDiboQd+tRxa8SaO2LB+1HAwPM8kaLrL7gkK7851Td\r\n" \
"vKuSTkCA5RSjLyfR0x6jF98hicjgaWEkpGfumh3ngbitTg5gCmxUoA==\r\n" \
"-----END RSA PRIVATE KEY-----\r\n";

static const u8_t psk[] = {0x12, 0x34, 0x56, 0x78};
static const char psk_identity[] = "my_psk_test";

enum tls_auth_mode {
    TLS_AUTH_MODE_NONE,
    TLS_AUTH_MODE_KEY_SHARE,
    TLS_AUTH_MODE_CERT_1WAY,
    TLS_AUTH_MODE_CERT_2WAY,
    TLS_AUTH_MODE_PSK,
};

int mqtt_ssl_cfg(struct mqtt_client_s *client, u8_t tls_auth_mode)
{
    int ret = 0;

    if (tls_auth_mode == TLS_AUTH_MODE_CERT_2WAY) {
        ret = mqtt_ssl_cfg_with_cert(client, (u8_t *)ca, sizeof(ca), (u8_t *)client_private_key, sizeof(client_private_key), (u8_t *)client_crt, sizeof(client_crt));
    } else if (tls_auth_mode == TLS_AUTH_MODE_CERT_1WAY) {
        ret = mqtt_ssl_cfg_with_cert(client, (u8_t *)ca, sizeof(ca), NULL, 0, NULL, 0);
    } else if (tls_auth_mode == TLS_AUTH_MODE_KEY_SHARE) {
        ret = mqtt_ssl_cfg_without_cert(client, NULL, 0, NULL, 0);
    } else if (tls_auth_mode == TLS_AUTH_MODE_PSK) {
        ret = mqtt_ssl_cfg_without_cert(client, psk, sizeof(psk), (const u8_t *)psk_identity, sizeof(psk_identity));
    }

    return ret;
}
