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
#include "app_cfg.h"
#include "atcmd_ota_certs.c"

#define MQTT_BAIDU_SERVER_CERTS

#ifdef MQTT_BAIDU_SERVER_CERTS
static const char ca[] =
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G\r\n" \
"A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp\r\n" \
"Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4\r\n" \
"MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG\r\n" \
"A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\r\n" \
"hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8\r\n" \
"RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT\r\n" \
"gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm\r\n" \
"KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd\r\n" \
"QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ\r\n" \
"XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw\r\n" \
"DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o\r\n" \
"LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU\r\n" \
"RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp\r\n" \
"jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK\r\n" \
"6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX\r\n" \
"mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs\r\n" \
"Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH\r\n" \
"WD9f\r\n" \
"-----END CERTIFICATE-----\r\n";

static const char client_crt[] =
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIEmzCCA4OgAwIBAgIDH0WKMA0GCSqGSIb3DQEBCwUAMG4xCzAJBgNVBAYTAkNO\r\n" \
"MSMwIQYDVQQDDBpvbmxpbmUuaW90ZGV2aWNlLmJhaWR1LmNvbTEOMAwGA1UECgwF\r\n" \
"QkFJRFUxDDAKBgNVBAsMA0JDRTEcMBoGCSqGSIb3DQEJARYNaW90QGJhaWR1LmNv\r\n" \
"bTAeFw0yMzEwMjAwNTIwMDFaFw0zMzEwMTcwNTIwMDFaMEMxDjAMBgNVBAoMBUJh\r\n" \
"aWR1MQswCQYDVQQGEwJDTjEWMBQGA1UEAwwNYWZzdW16eS9mZWlnZTEMMAoGA1UE\r\n" \
"CwwDQkNFMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAgMlgtHQqtKg5\r\n" \
"CxpumURDIADkJlcBQJWxa6XmfW+NxyDyT/BsGUiq8L/D9ptnJ7p5hyWDqhAXJSa7\r\n" \
"tGA9433RmlC43grSpxEw64EQkQ6LGLxXQ8Qk50W2ySOnR+zvH0VyB69wmUXwLa/L\r\n" \
"/wv5DPzoZhcAZV5AIPJJt3WwDwZHLMtJyY+XURFGGIuMYTs+CA5yvxEZeCWFi9Y7\r\n" \
"P87YZrgX6xlSfBp962pTNeiwdVgvvOPrNWMIjCkpxPSxbYtcJYrwaAmit1dUoNhP\r\n" \
"bS0Udyfw3dhoK9MzIabiqdECTZnsWRHyKCSJAx3sgJphgqAz9UHVYDfjRJjWswNm\r\n" \
"0dGKAwD3TwIDAQABo4IBazCCAWcwHQYDVR0OBBYEFDWqaWRg6M5WUHmPTZzZyLN4\r\n" \
"bnI7MAwGA1UdEwEB/wQCMAAwHwYDVR0jBBgwFoAUs+sTxDBPAMKn6xUOzNgrJ3YT\r\n" \
"ZFcwgaMGA1UdHwSBmzCBmDCBlaCBkqCBj4aBjGh0dHA6Ly9wa2lpb3YuYmFpZHVi\r\n" \
"Y2UuY29tL3YxL3BraS9jcmw/Y21kPWNybCZmb3JtYXQ9UEVNJmlzc3Vlcj1DPUNO\r\n" \
"LENOPW9ubGluZS5pb3RkZXZpY2UuYmFpZHUuY29tLEVNQUlMQUREUkVTUz1pb3RA\r\n" \
"YmFpZHUuY29tLE89QkFJRFUsT1U9QkNFMEIGCCsGAQUFBwEBBDYwNDAyBggrBgEF\r\n" \
"BQcwAYYmaHR0cDovL3BraWlvdi5iYWlkdWJjZS5jb20vdjEvcGtpL29jc3AwDgYD\r\n" \
"VR0PAQH/BAQDAgP4MB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcDBDANBgkq\r\n" \
"hkiG9w0BAQsFAAOCAQEAg4pvwaMa89SspoQbfFPZxIw1N94qxXNuF/wgCMHACmRj\r\n" \
"T4omZezVXmfxG8aSPDMPnUYpaIYqdyhWijid3jNG2c5K2RXGfvxkF6sFeb+0rOd4\r\n" \
"4XYfdF1SyZil5bIVxmWKsikb9zM4VW/hQ76b8cOZNqmBWbx+YlphwQ/FQU7tD4Hl\r\n" \
"qD2AcikbRFYv153/0c8gjFF2HKhsAZhL7LoHrB8MQkBNGy4679TiYy4id/JEIA8j\r\n" \
"N3JWqLgOO7txDQZCIO5axvbSxIUWkgv4TVhU1dxRzzFswpJHWIwICoadaJh9ceCX\r\n" \
"iYZLprcqj/WE5BRR7PvtSngBZn2VBxhth0yE4CX4aA==\r\n" \
"-----END CERTIFICATE-----\r\n";

static const char client_private_key[] =
"-----BEGIN RSA PRIVATE KEY-----\r\n" \
"MIIEowIBAAKCAQEAgMlgtHQqtKg5CxpumURDIADkJlcBQJWxa6XmfW+NxyDyT/Bs\r\n" \
"GUiq8L/D9ptnJ7p5hyWDqhAXJSa7tGA9433RmlC43grSpxEw64EQkQ6LGLxXQ8Qk\r\n" \
"50W2ySOnR+zvH0VyB69wmUXwLa/L/wv5DPzoZhcAZV5AIPJJt3WwDwZHLMtJyY+X\r\n" \
"URFGGIuMYTs+CA5yvxEZeCWFi9Y7P87YZrgX6xlSfBp962pTNeiwdVgvvOPrNWMI\r\n" \
"jCkpxPSxbYtcJYrwaAmit1dUoNhPbS0Udyfw3dhoK9MzIabiqdECTZnsWRHyKCSJ\r\n" \
"Ax3sgJphgqAz9UHVYDfjRJjWswNm0dGKAwD3TwIDAQABAoIBACAD78psZoFqoGOm\r\n" \
"OoBXe+hk0FLHpkQL8oM0RsxAAxrRketVdUoDypr20RxpHYe+z59NMCICxf4yVs9M\r\n" \
"bZY0HPsjvhFU13E40NR6zUeCOgn4KClshVAJAJuYBWnX+MnpVaObdX5k6IQzA59v\r\n" \
"toYICS334d1RRNfr4298Djxt7xuvQul4g0MVoDGnnTDyfu06WtYYonfc1qL+MZYw\r\n" \
"KV3s+Cq9X2k2TypN5+VrrG0XEqDVV84UDzw9TfimBEF699Z5YW2osOFyBX+rtrds\r\n" \
"HKcQRXL1FHWtPNOCHYQMzbyByqwAjQOJSzKomMpbVVW130DUMZaRPriiapqkuPpr\r\n" \
"m7nM/QECgYEAyg2G2tRLSWUsNKJ9rQjKkhcEm5RTQP7iLciDQAF/AH1F5/NCg/IW\r\n" \
"UL+pZ7LnaEfflAkPx1r/Yj3VIslSNWWGgbo2LC5hZZFiipSPiA0Yr9DvZSnx5M0h\r\n" \
"vsDtW9TheKCmAk5ObQhtnvJJGMMuPun6oHDt+Cm1MQ+ZkeLyaBoHgmECgYEAoywM\r\n" \
"DlQH3kqJaNUO5msp2v4aT38kUH7MYh92V5/D5pjLBd3LNh1TeOMtl/E2BByVVQuU\r\n" \
"ug20pk+PvEfXdQrLDJSmzCuD/4BTKYzVTEEguBsjWTLAxYzqcW+sH4F26MmHo5vj\r\n" \
"1MSDPv2QtScaIvJ1vimCRvdsHNqLxz6fbcBcN68CgYBYqXLhl4Kp5EFvn1Xylgb9\r\n" \
"8CfPdVjLDo2FdZVSgtWOC2qfi7lGWPa93DykCndM5S0QsqE/44hpPaTHLPxr/e72\r\n" \
"AhY/cOLARPmuwd3x331TuSUziSJiOjlykQoW3+VIn4X5QQ9c/PPNaZf1y8ABT37w\r\n" \
"5F0oJnUh4CyNPb8NO07MQQKBgHHUbYilWGPbjaZjU6Ssx6MtNv+US6oX+s7M8grI\r\n" \
"uqoolyE9i+DxbmTb3terfyo5IngUvylYHFkVEcmgOI8++02Ieh/ej5PzWpCW3cn3\r\n" \
"eTLWQ7+bJ13pIzgFVocYEvLsfEJHoxWwDXso+wVVBOeySy9g17BYNMSgNXjGmAPB\r\n" \
"eNOjAoGBAI0w1Zq/YXL/fXnvIpzPfUzewCOtEUk0DwjsKly96pEm5HM91zwH5w8L\r\n" \
"VFesuvRAkrXs+OLizwwfAAwRt2uf3Fq4tIuJeLDx2x9CxqPJQEkXmwWkSptFVyg6\r\n" \
"kJZPMWZeH1iWOlm5aVI5GwRoLaqIw/2KOhprfVRsU6uiedmWecp/\r\n" \
"-----END RSA PRIVATE KEY-----\r\n";

#else   // LOCAL MQTT SERVER CERTS

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
#endif

static const u8_t psk[] = {0x12, 0x34, 0x56, 0x78};
static const char psk_identity[] = "my_psk_test";

enum tls_auth_mode {
    TLS_AUTH_MODE_NONE,
    TLS_AUTH_MODE_KEY_SHARE,
    TLS_AUTH_MODE_CERT_1WAY,
    TLS_AUTH_MODE_CERT_2WAY,
    TLS_AUTH_MODE_PSK,
    TLS_AUTH_MODE_CERT_CLIENT_ONLY,
};

int mqtt_ssl_cfg(struct mqtt_client_s *client, u8_t tls_auth_mode, bool at_ota_enabled)
{
    int ret = 0;

    if (tls_auth_mode == TLS_AUTH_MODE_CERT_2WAY) {
#ifdef CONFIG_ATCMD_OTA_DEMO
        if (at_ota_enabled)
            ret = mqtt_ssl_cfg_with_cert(client, (u8_t *)ecs_ca_crt, sizeof(ecs_ca_crt), (u8_t *)ecs_cli_key, sizeof(ecs_cli_key), (u8_t *)ecs_cli_crt, sizeof(ecs_cli_crt));
        else
            ret = mqtt_ssl_cfg_with_cert(client, (u8_t *)ca, sizeof(ca), (u8_t *)client_private_key, sizeof(client_private_key), (u8_t *)client_crt, sizeof(client_crt));
#else
        ret = mqtt_ssl_cfg_with_cert(client, (u8_t *)ca, sizeof(ca), (u8_t *)client_private_key, sizeof(client_private_key), (u8_t *)client_crt, sizeof(client_crt));
#endif
    } else if (tls_auth_mode == TLS_AUTH_MODE_CERT_1WAY) {
        ret = mqtt_ssl_cfg_with_cert(client, (u8_t *)ca, sizeof(ca), NULL, 0, NULL, 0);
    } else if (tls_auth_mode == TLS_AUTH_MODE_KEY_SHARE) {
        ret = mqtt_ssl_cfg_without_cert(client, NULL, 0, NULL, 0);
    } else if (tls_auth_mode == TLS_AUTH_MODE_PSK) {
        ret = mqtt_ssl_cfg_without_cert(client, psk, sizeof(psk), (const u8_t *)psk_identity, sizeof(psk_identity));
    } else if (tls_auth_mode == TLS_AUTH_MODE_CERT_CLIENT_ONLY) {
        ret = mqtt_ssl_cfg_with_cert(client, NULL, 0, (uint8_t *)client_private_key, sizeof(client_private_key), (uint8_t *)client_crt, sizeof(client_crt));
    }

    return ret;
}
