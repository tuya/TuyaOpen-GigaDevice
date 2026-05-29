
cmake_minimum_required(VERSION 3.15)

target_sources(${TARGET_EXE}
    PRIVATE
        atcmd.c
        ../ble/app/ble_init.c
        cmd_shell.c
        # iperf.c
        main.c
        mqtt_app/mqtt_cmd.c
        ota_demo.c
        ping.c
        mm_app/mm_cmd_parse.c
        mm_app/mm_icc.c
        mm_app/mm_keepalive.c
        mm_app/mm_mqtt.c
        mm_app/mm_spi.c
    )
