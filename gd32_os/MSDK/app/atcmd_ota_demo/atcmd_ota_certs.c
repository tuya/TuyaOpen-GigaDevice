/*!
    \file    atcmd_ota_certs.c
    \brief   SSL certificate for GD32VW55x SDK

    \version 2023-5-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

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
#ifdef CONFIG_ATCMD_OTA_DEMO

static const char ecs_ca_crt[]=
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIDpjCCAo6gAwIBAgIUfDwa864quY4Vx9k42SSQ8LTJdggwDQYJKoZIhvcNAQEL\r\n" \
"BQAwZDELMAkGA1UEBhMCQ04xDjAMBgNVBAgMBVN0YXRlMQ0wCwYDVQQHDARDaXR5\r\n" \
"MRUwEwYDVQQKDAxPcmdhbml6YXRpb24xEDAOBgNVBAsMB09yZ1VuaXQxDTALBgNV\r\n" \
"BAMMBE15Q0EwHhcNMjUwNzIzMDkzOTAwWhcNMzUwNzIxMDkzOTAwWjBkMQswCQYD\r\n" \
"VQQGEwJDTjEOMAwGA1UECAwFU3RhdGUxDTALBgNVBAcMBENpdHkxFTATBgNVBAoM\r\n" \
"DE9yZ2FuaXphdGlvbjEQMA4GA1UECwwHT3JnVW5pdDENMAsGA1UEAwwETXlDQTCC\r\n" \
"ASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAJk5GAmISqgak3FvxkBZNloy\r\n" \
"0xP7SnZuqUyrtt0zDF25IZrAngptUyydGzR6ZJhPeejKZ5vW7AGrKMybwyV/Sp1N\r\n" \
"qJI3yaB/OWBQr7nkZDi249NmqabbGRbt9zXV47kAi/XkQqBK85Ct1RcACdRSnze0\r\n" \
"+EdkDwtblUgxjLyRNXQdCPFydv5GePDK9b5eT4gJF93YrEflZk1TiJgfvfS78VqU\r\n" \
"Z2MEDdZqv5jW9GpGhV2ssi4yOsT28+++z0mj1Hc0HSOfxD5oj5zLjM/oCUEKApgX\r\n" \
"pLnLRGu5Q8xhDp9KNJ/yrZFFy+ZeWDqf58EULM5dZ4ohguT9+O9KiBw7eEglDr8C\r\n" \
"AwEAAaNQME4wHQYDVR0OBBYEFC0vAGmMcUYPiSMB6ik/C64XLGA2MB8GA1UdIwQY\r\n" \
"MBaAFC0vAGmMcUYPiSMB6ik/C64XLGA2MAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcN\r\n" \
"AQELBQADggEBAGP6IOQeep+WOHG78QG0QIMXbxApaL/uqhqPd/wPDNfGoMtOSCPw\r\n" \
"/JCWR+cRef9qhWVZZMEHYdHlygIqBGIja8DQRJ3h0p1/wwKZtbHZb2/S5+1XI3SA\r\n" \
"ky2a3+toPDE688JSS/iXOZAp9BTCcNV/xrJM4Zi/RR85LQicEF8EQ6kLGlwkkagK\r\n" \
"mE+RUlCX9OzHzDKVX2DOndglgQp1TGm+RRElyLDOA7ejSt/1RfncpZDsJ4frv6Zo\r\n" \
"pbq/fqIPEcqzIIfcA9f1v5FkiA1tQ2nrXNXNBIKOch0TQo7ZYNXH8DagMqlQyP0k\r\n" \
"TdKp0C/KI9FN/6Ca9Y6PrOvB2IC7qhYCiuA=\r\n" \
"-----END CERTIFICATE-----";

static const char ecs_cli_crt[]=
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIDmjCCAoKgAwIBAgIUTiqtkv8NhvHXL5g7eDvMLKCX4CowDQYJKoZIhvcNAQEL\r\n" \
"BQAwZDELMAkGA1UEBhMCQ04xDjAMBgNVBAgMBVN0YXRlMQ0wCwYDVQQHDARDaXR5\r\n" \
"MRUwEwYDVQQKDAxPcmdhbml6YXRpb24xEDAOBgNVBAsMB09yZ1VuaXQxDTALBgNV\r\n" \
"BAMMBE15Q0EwHhcNMjUwNzIzMDk0OTQyWhcNMzUwNzIxMDk0OTQyWjBmMQswCQYD\r\n" \
"VQQGEwJDTjEOMAwGA1UECAwFU3RhdGUxDTALBgNVBAcMBENpdHkxFTATBgNVBAoM\r\n" \
"DE9yZ2FuaXphdGlvbjEQMA4GA1UECwwHT3JnVW5pdDEPMA0GA1UEAwwGY2xpZW50\r\n" \
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAkdmjbg8VuV4BDvkbx/24\r\n" \
"E7OOkDRr1nLMSMVrFaWzqjHWg/IMOg1SE+EcgG6UAkGr0N9h419cDBi+YsPfNbvx\r\n" \
"JRlcm9DnlR0ObQnAwtj3v22VvTh3WOMaQicI0gHu+zBX9TDnVm9WHL8Xg6Ydcnh8\r\n" \
"IMGxnjeRJP0fzh8YporfNVUfQlY0GacZ0rKz4JMh2x3bZ4q2zvcpcg1hmGbzKH5Y\r\n" \
"jZiGouYGmFQB3gEkgkoUwA2Mqa7ktjXFhwg5cODiC2AYOgC98ZwStata2XMXF5A1\r\n" \
"nJD654pjQ9ra0UMhmw15Gx/9ziEx5flak5NPVqXecBuB3fGLIccaNv2QUNNH3uaS\r\n" \
"qwIDAQABo0IwQDAdBgNVHQ4EFgQUR19XVFfhRSJ7KzszWvSbFUm1CsQwHwYDVR0j\r\n" \
"BBgwFoAULS8AaYxxRg+JIwHqKT8LrhcsYDYwDQYJKoZIhvcNAQELBQADggEBAIiF\r\n" \
"dcYzPTL4Nbl48doZlu+Y2fg4WoRtsUo6GzZ+c5Y7ybA2reP7R3zG/XC2zIwmTIjZ\r\n" \
"DWh7ujEubzYKIoGYka7LCSlSqPGsocTzP3rRkWGWPf+q7ouYlgHPe+1aDGpxL3rO\r\n" \
"V5lVEDbAyWidfZcUp7Rt0vYYI/ApvPkfEJMpYMgC4vgNYixG943ymVMwKGg1t/8l\r\n" \
"/DZj/rGfU8PKybeViM61vOJjKliJ6D48ra9FO35dPElicad/HqeIv7YQ8M674rgu\r\n" \
"YddFCcmhaoYjaLoifZPGIdeIXODOYMOzaA0pE5N1etYfHIQNV4udSOt9i6miRURH\r\n" \
"7tCiRjwgom7OlQ+l29c=\r\n" \
"-----END CERTIFICATE-----";

static const char ecs_cli_key[]=
"-----BEGIN PRIVATE KEY-----\r\n" \
"MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQCR2aNuDxW5XgEO\r\n" \
"+RvH/bgTs46QNGvWcsxIxWsVpbOqMdaD8gw6DVIT4RyAbpQCQavQ32HjX1wMGL5i\r\n" \
"w981u/ElGVyb0OeVHQ5tCcDC2Pe/bZW9OHdY4xpCJwjSAe77MFf1MOdWb1YcvxeD\r\n" \
"ph1yeHwgwbGeN5Ek/R/OHximit81VR9CVjQZpxnSsrPgkyHbHdtnirbO9ylyDWGY\r\n" \
"ZvMofliNmIai5gaYVAHeASSCShTADYypruS2NcWHCDlw4OILYBg6AL3xnBK1q1rZ\r\n" \
"cxcXkDWckPrnimND2trRQyGbDXkbH/3OITHl+VqTk09Wpd5wG4Hd8Yshxxo2/ZBQ\r\n" \
"00fe5pKrAgMBAAECggEARZkXEVp2ceXnWGdMOAOdVx1sbujs+sD+Nc2rO4G+ef1d\r\n" \
"Ucprxjn8DEt0R988ltUSsdIfCprDOGyIUWYJukEMojj3hc1K7U+XxjWMvA5ALyOS\r\n" \
"fGCZJxv8OEwbO+e7TDd2CaWEBy2K6J3RkTtwOuM+zHzRjuASTZXSmxAgubx0JiTX\r\n" \
"iBfL3m3/x05kHuJ6CZnrunfQg1pZgCi5VJ/tTSRqF73xfCLduaWNL2x36OzYzT8i\r\n" \
"Of+daZDeViI2LlgLSoOyd46QBtE/z6zP7AAUM/Yp2YcEDW3wDb1RuhE9ZBPERlA/\r\n" \
"xsG/17OL0ag2dwn+vf3PqbR4qf+KmqkdFJIrhJ/FdQKBgQDH02fLQVcNzD8Ij+w6\r\n" \
"57KmTHwTg1CD/keoYbNqEHrPcEpb06BDY6Oi0GcDbEtWX3aUqljECPgY/TTH9MjS\r\n" \
"2Pb+kRu6cqkz3Pis/i7+Uv/3Z9H+O9x/quiBqd97JsnuXy2EF9XY0OdNdW4JWnsc\r\n" \
"E1aTczCNkAprSSwjvXsQbPIhzwKBgQC62db+h4tGIUJTVd6gv5gzy/95R8FwlG2N\r\n" \
"yPB2nzOF7+TiFPW223xMZdYFt6jJTfzoQ1yykDhEnW9LjAesnk/2Ly7gwIcAH1+f\r\n" \
"8CG3+Ep36RwlNRZAIgP9MOB+LFExavKzJvCEXHwXKxZQEFdj5uWSNA4Ryd+2u4bL\r\n" \
"wDiwJagEZQKBgHrUw5+YSYfMvg5oUcCfvhwlNbvU40BPpQKnE98lxCgREVRC0Oe7\r\n" \
"rtGdI0ViYyjzw3N9BtVx4feSZchn+q5I+gLfddj1NHcR5LaKBJ8GrBsLcXTCnTbI\r\n" \
"+uUBY8NSw5vIW7Uv5Z7CQvxFK9+KA9TtOjGnA/hwy0bJkt1hUu102p3XAoGAC+ub\r\n" \
"2K8jBn1WsuRWiKDQf/VyvfUC9UCy4InCNX8glVnhmz7FEj5cZq9UgRbfcMBF8zdg\r\n" \
"8Y9b15zHneU4FB4hcc4+yl3d/vcBbb2vGQKBFbdMcV86pSrGYF++4q3HHET7aMyC\r\n" \
"KZ+Q1xWLnd39BUWUvcgOYbokSI8cIWGI/EvatTUCgYARh4c8vFNoHlJuwIqQhi7+\r\n" \
"p9hhiqiVfkPqxBwFfhWnidVEaz2FyzMj445tiTzKD18VZNpuheHx2iXEm2usz+ZM\r\n" \
"z13CNzxY6sJJa4/PfpdAUqeeiZUt2AxfD9iNXpqYTOXJ2WOIJb4kLlFfZ62W0NNq\r\n" \
"kB2tuptEys/+Iw3uRpKtzw==\r\n" \
"-----END PRIVATE KEY-----";

#endif /* CONFIG_ATCMD_OTA_DEMO */
