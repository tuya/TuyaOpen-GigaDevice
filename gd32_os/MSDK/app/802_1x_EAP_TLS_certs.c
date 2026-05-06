/*!
    \file    802_1x_EAP_TLS_certs.c
    \brief   Certs and key files for GD32VW55x SDK when connect to Enterprise AP
              with EAP-TLS.

    \version 2024-07-10, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

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

#ifdef CFG_8021x_EAP_TLS

static const char client_cert[]=
"-----BEGIN CERTIFICATE-----" \
"MIID3jCCAsagAwIBAgIBAjANBgkqhkiG9w0BAQwFADBxMQswCQYDVQQGEwJDTjEL" \
"MAkGA1UECAwCSlMxCzAJBgNVBAcMAlNaMQwwCgYDVQQKDANHRC4xIDAeBgkqhkiG" \
"9w0BCQEWEWFkbWluQGV4YW1wbGUub3JnMRgwFgYDVQQDDA9FQVBfVEVTVCBDQSBD" \
"TkYwHhcNMjQwNjIwMDgwNTIxWhcNMzQwNDI5MDgwNTIxWjBhMQswCQYDVQQGEwJD" \
"TjELMAkGA1UECAwCSlMxDDAKBgNVBAoMA0dELjEWMBQGA1UEAwwNRUFQX1RFU1Rf" \
"VVNFUjEfMB0GCSqGSIb3DQEJARYQdXNlckBleGFtcGxlLm9yZzCCASIwDQYJKoZI" \
"hvcNAQEBBQADggEPADCCAQoCggEBAKBWjvvZUtGrc0Z1DRZGY2qwx+KTtXuUyWqQ" \
"j0HijYPQ1iOnWoH7EDZ6gtRVNe7T3zz4tCf3QjK9junhMhS8w9ePCR3DBTf8ObMK" \
"Scq13IaFrmyLnp6Z+DQHLlSUlt7b+cXR+ZH2gZa/eITo2Y3i8B2puNBD/Yop4NmR" \
"0P+vA0uAUMbCMX6HtJX1Z++nR1uQxJ4ROILmK3FEP+GVxMTbkpO5y6G5xI1+pGlE" \
"bqfbvHXdENs/TWiG6VqWzeurMi8TH/0NkQs942bPLei5ajoABAQmY7GenfxX4qBU" \
"54qs28pIeF/sB2dcAyMHOPYAD2C2EtqKrmNTIRMGWVo/0KqO3S8CAwEAAaOBkDCB" \
"jTATBgNVHSUEDDAKBggrBgEFBQcDAjA2BgNVHR8ELzAtMCugKaAnhiVodHRwOi8v" \
"d3d3LmV4YW1wbGUuY29tL2V4YW1wbGVfY2EuY3JsMB0GA1UdDgQWBBSsPkX4/FLR" \
"J6mwje9p/YHQLBAlUDAfBgNVHSMEGDAWgBS8efW0gzC3cPt98BFJWZN6R8wx4jAN" \
"BgkqhkiG9w0BAQwFAAOCAQEAgPYCVZgDfadYdGs7l9o2taq8LaOzPlAyeF/uif2R" \
"V5XSzFSDoUr/5pomuN4p2piCR2tb5bAc9NZ2iEJvZZi8o7sDoMpoeimF5g1rZ3hA" \
"wIfen9sCoiUXcAzisf6Ff4P2xSCJD5ti1JHHwIDQLQ/nfwEqfsV8bck0BG+txtx5" \
"l6mS7iWoXn/P5Q9FkUmugCw8JoTkxhiuIkqPAZQrIfgLkkq6Nc0eX+azCOV+7DHz" \
"JzU5tCjx+n1xV25UAcjpHYK1ii4j6dOx4DVeNdLGgZE2ecmaC/Q2hE7Tq6yAZbwn" \
"lpEO6pPKHQ+uqB2tLkpwIze7kB6qSFyD8/u3cAnS18l6nA==" \
"-----END CERTIFICATE----- \0";

static const char client_key[]=
"-----BEGIN ENCRYPTED PRIVATE KEY-----" \
"MIIFHDBOBgkqhkiG9w0BBQ0wQTApBgkqhkiG9w0BBQwwHAQIjvl/+UgNmTQCAggA" \
"MAwGCCqGSIb3DQIJBQAwFAYIKoZIhvcNAwcECGWaY3IjObRwBIIEyPZdC+U4M3+n" \
"WdVE3oODvxV8bDnC92HwKL8zEM77z92DjuAm5kp1HdY4AY1TRj7DV0PbohWWCmd6" \
"ubQ6Uxn9Ig3tasEktxHJEDPIFPlwS6JenvdosOc1kxWVDubrBqYvU2sMhSXV5Kru" \
"Yk4rt1n5EFOEu/0TpF59hrbrN7N6W340KgOKghO36wvqp8uGI7U6SKTJhqEiaGv/" \
"X1vX4JPa1mCEhabCT5KkjWpTcaxvezZsVgDIAkKXEAjp0CO0o/ocmXruTnhTe4E7" \
"uTb25gHK+sML1jUHEl+uWHrVcU+RTZ7wybmKcJWr3SmlJxKEb292Rzphb1Zs9IAn" \
"p5DEO3OADWgNDuQF6oI7SJN1UHDhUY/OVxvT8WvSAj9lEXXtNAkizpj3bxvCLR0q" \
"yYKckq8Cql7c3isbWPLV//jq/3l/dYkLjXeFpBU5GixbKGDm2hdLRSQPGiFARvgx" \
"r2XZzu3/ZFfinQKIYw1/SiiLgo6WDCcImqGN3d6js20t6U1FQnWWJ3gqLX7mL0DJ" \
"3vek389AzQttODHgdO2Lhc/G4EwPS486QX1PeOXAXV7FUM8ocQwxQSd5UgZ5bWOd" \
"jeg0ekVOHIh2Nk2LK81QKReFzTG+EiBHGGpqJd6GQ9gozHSrvs4nkFCilC3tXqCT" \
"77rnP5uGDbIvhJ55mDANG+/oHNgVnWUOzPlRk2aJbUaXUMvKDsZpbLNhjxpxogGF" \
"+1qVmlqQMvrG9a7xRKQ3GmK2/4ZwOiogBDFugbHE2lwcXEkrMQhv575wHVvjeTHf" \
"Y7AfCXwRsw5Tew3EM005xGj8nTfIqxhgtu9nlB3NCIJuKMVNVg3E1MTEn2Dv8OS4" \
"RIDu+g2jMHirO2HqAQA09OwY3j455L3Wfm3/FPcXj+qLULlZySs8ajbtDjP0ajGH" \
"MHfvMLt826+nU9L3TJuO00H80cArjETuFQrg7NVtP0T5NpIfL8pr89/TUuy3lN/1" \
"vJQXXmcXyMHnFBIq1y5lMTpNSMf0M3AkUbjcpHYXfuT2hcZD8j5uTM0YgAW+w5Kq" \
"JpL/vR1Nhz5HRlzvmVQLFQO9qsHWHxnj60RJYU5KiUSbPsql0f1EUWIAPZ7RS22O" \
"/mzXoPPCtw3kiCZis0Ye38SVZfOoU+YYOpXoB1bJBRPWeTXe3ZU8tPDvAGrmzqK1" \
"XltSCM7notzBgxKtoo6m9AG8gWGC7oHHjZiURVulAoodayGYm0Rr0db/yF0njAOK" \
"BymXeOF7/TNY3fjb6OBy7Qkz5x7bJEiaUq5kP3bHVo/2hziH+Uj+zZpGHXec2Awn" \
"DUrzRMoaMl+N//7ZdTkvTkng/bgpdavJeOU6L8w6ZMntP4i71G93KhgsvrRGw+ld" \
"6xy1rDh+Wq1ZNOddg8LPsrlW8ut2b61905c2iCDSUIqX5ijoGWdq8lMNeWjE+zn9" \
"laaPDtX+r4Ka/Totuif2yXVpSuHvYF0mBvZ279fsaA9naVEHczWJG8vbaWBQ2BiY" \
"iN6nsuOzQsn9HZLoglewX6BrDtcBBHXCgtVeyi4pPIoKaNhK/fuSCN59rsU7JtNJ" \
"EJO8jqhX7+S9wQ+dRiXWWEMLosXxBGBMBopiAlq9d0oEco/mrkigKu9iJS/MrXhR" \
"bdPgVNaGU5d10zx6eRAOnw==" \
"-----END ENCRYPTED PRIVATE KEY----- \0";

static const char ca_cert[]=
"-----BEGIN CERTIFICATE-----" \
"MIIEjzCCA3egAwIBAgIUczpZZA4JjCxY/dEjW1ztrvVgyiMwDQYJKoZIhvcNAQEL" \
"BQAwcTELMAkGA1UEBhMCQ04xCzAJBgNVBAgMAkpTMQswCQYDVQQHDAJTWjEMMAoG" \
"A1UECgwDR0QuMSAwHgYJKoZIhvcNAQkBFhFhZG1pbkBleGFtcGxlLm9yZzEYMBYG" \
"A1UEAwwPRUFQX1RFU1QgQ0EgQ05GMB4XDTI0MDYyMDA4MDUyMFoXDTM0MDQyOTA4" \
"MDUyMFowcTELMAkGA1UEBhMCQ04xCzAJBgNVBAgMAkpTMQswCQYDVQQHDAJTWjEM" \
"MAoGA1UECgwDR0QuMSAwHgYJKoZIhvcNAQkBFhFhZG1pbkBleGFtcGxlLm9yZzEY" \
"MBYGA1UEAwwPRUFQX1RFU1QgQ0EgQ05GMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8A" \
"MIIBCgKCAQEAqayJMtD0xI5jRhg3X7woVcWJPEpTFkoLzVL2Ojfy8B6nmSgB0ldz" \
"yrDUaJZYQ/BFOiQVjnMjI13J2kzHzIoofhKsd+I+TcwF6v+Jon4JjGRF7/r/nVFY" \
"rHGnJSULDxCVEg7g4VGxh/7icgEIJLA9SWzB+QkdRYQX4YzEBSX2eZXJsmrXEVxm" \
"qeZUzl6fLDrvjV28IwgHxa3Nc/GZU4ilqSAo0Ai5++1Xh6DYAokV8Okpkg3y/8wE" \
"PQrNLHPFmtR5wq8DOWZD6RM3wkby5nF1+UISyxjof7ArL20fQNhBaAAdq7JdFzv/" \
"8/xRjRNUOk8Bho4aDaZFvgAqrcE7xuHLkwIDAQABo4IBHTCCARkwHQYDVR0OBBYE" \
"FLx59bSDMLdw+33wEUlZk3pHzDHiMIGuBgNVHSMEgaYwgaOAFLx59bSDMLdw+33w" \
"EUlZk3pHzDHioXWkczBxMQswCQYDVQQGEwJDTjELMAkGA1UECAwCSlMxCzAJBgNV" \
"BAcMAlNaMQwwCgYDVQQKDANHRC4xIDAeBgkqhkiG9w0BCQEWEWFkbWluQGV4YW1w" \
"bGUub3JnMRgwFgYDVQQDDA9FQVBfVEVTVCBDQSBDTkaCFHM6WWQOCYwsWP3RI1tc" \
"7a71YMojMA8GA1UdEwEB/wQFMAMBAf8wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDov" \
"L3d3dy5leGFtcGxlLm9yZy9leGFtcGxlX2NhLmNybDANBgkqhkiG9w0BAQsFAAOC" \
"AQEAMIENd8d8hNGHEKC5o9bd/rsou7vlBLuUoZ3MdbPkJF3JhSozy/QFc9xcDz3Z" \
"jYo42SdI6zvG1RDeb/USA0wDNOL5eJkFsRPA93QfjQBqAwTAlzQ6sFyiwHKexatI" \
"lbbO+ZRBdg/y9DruyypBlwi34QjDB/gdSiuwmrc0v716ELDKk8Yi6D6pDIzdses3" \
"ZcniECfvxmgjmEee6dSGTjQgfwVwuwEfN7DikM9CWkEeglanXak4dkycfbRzvzRa" \
"V/mcPRVQJbKW7AodQ+4BmdkCLCCE5CDeBToCxtoWe36+U/+oTfMUfIwK/zer4E/o" \
"I145ap+b8wP0Tu8Jpi5eDmyfAw==" \
"-----END CERTIFICATE----- \0";

static const char identity[] = "user\0";

static const char client_key_password[] = "whatever\0";

static const char phase1[] = "tls_disable_time_checks=1\0";

#endif /* CFG_8021x_EAP_TLS */
