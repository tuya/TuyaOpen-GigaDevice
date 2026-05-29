#include "tuya_iot_config.h"
#if defined(MM_ENABLE_MQTT_KEEPALIVE)

#if defined(SYSTEM_LINUX) && (OPERATING_SYSTEM != SYSTEM_LINUX)
#include <stdio.h>
#include "tkl_queue.h"
#include "tkl_thread.h"
#include "tkl_semaphore.h"
#include "tkl_memory.h"
#include "tkl_system.h"
#include "tuya_error_code.h"
#include "mqtt_service.h"
#include "tuya_iot.h"
#include "mm_mqtt.h"
#include "tuya_log.h"
#include "mm_cmd_parse.h"
#include "mqtt_client_interface.h"
#include "cJSON.h"
#include "mm_system.h"
#include "netmgr_api.h"

#define MQTT_ONLINE_MAX_TIMEOUT (30 * 1000)
#define MM_DATA_QUEUE_SIZE      32

struct mqtt_data {
    int proto;
    int data_len;
    int topic_len;
    char data[0];
};

#define MQ_THREAD_STACK_SIZE (1024 * 2) // 2K stack size.
#define MQ_THREAD_PRIORITY   4
static TKL_SEM_HANDLE mm_mqtt_online = NULL;
static TKL_QUEUE_HANDLE mm_data_queue = NULL;
static TKL_THREAD_HANDLE mm_mqtt_send_thread = NULL;
static tuya_iot_client_t *mqtt_client;

void mm_mqtt_process_proto(tuya_mqtt_event_t *ev, uint16_t proto)
{
    // check arguments.
    if (NULL == ev) {
        PR_ERR("\n%s:%d invalid args ev:%p\n", __func__, __LINE__, ev);
    }

    cJSON *data = (cJSON *)(ev->data);

    // wakeup by mqtt msg.
    tuya_wakeup_reason_handler(WAKEUP_GW);
    // dump data and forward to mpu.
    char *tmp_str = cJSON_PrintUnformatted(data);
    if (tmp_str) {
        ty_lp_proto_mqtt_send_data_cs(tmp_str, strnlen(tmp_str, MQTT_DATA_MAX_LEN), proto);
        cJSON_free(tmp_str);
    }

    return;
}

void mm_mqtt_process_proto_5(tuya_mqtt_event_t *ev)
{
    mm_mqtt_process_proto(ev, 5);
    return;
}

void mm_mqtt_process_proto_302(tuya_mqtt_event_t *ev)
{
    mm_mqtt_process_proto(ev, 302);
    return;
}

void mm_mqtt_process_proto_312(tuya_mqtt_event_t *ev)
{
    mm_mqtt_process_proto(ev, 312);
    return;
}

void mm_mqtt_process_proto_802(tuya_mqtt_event_t *ev)
{
    mm_mqtt_process_proto(ev, 802);
    return;
}

void mm_mqtt_process_proto_300(tuya_mqtt_event_t *ev)
{
    mm_mqtt_process_proto(ev, 300);
    return;
}

void mm_mqtt_process_proto_307(tuya_mqtt_event_t *ev)
{
    mm_mqtt_process_proto(ev, 307);
    return;
}

void mm_mqtt_process_proto_308(tuya_mqtt_event_t *ev)
{
    mm_mqtt_process_proto(ev, 308);
    return;
}

void mm_mqtt_post_data(char *data, int proto, char *topic)
{
    int data_length = 0;
    int topic_length = 0;
    struct mqtt_data *md = NULL;
    if (NULL == data || NULL == topic) {
        PR_ERR("Invalid args data:%p", data);
        return;
    }
    data_length = strnlen(data, MQTT_DATA_MAX_LEN);
    if (0 == data_length) {
        PR_ERR("error data length");
    }
    topic_length = strnlen(topic, 200);
    if (0 == topic_length) {
        PR_ERR("error topic length");
    }
    md = tkl_system_malloc(sizeof(struct mqtt_data) + data_length + topic_length + 2);
    if (NULL == md) {
        PR_ERR("\n%s:%d failed in malloc\n", __func__, __LINE__);
        goto err;
    }
    // post to queue.
    md->proto = proto;
    md->data_len = data_length + 1;
    md->topic_len = topic_length + 1;
    memset(md->data, 0, md->data_len + md->topic_len);
    strncpy(md->data, data, md->data_len);
    strncpy(md->data + md->data_len, topic, md->topic_len);
    if (OPRT_OK != tkl_queue_post(mm_data_queue, (void *)md, 0)) {
        PR_INFO("\n%s:%d **** proto:%d content:%s\n", __func__, __LINE__, proto, data);
        goto err;
    }
    return;
err:
    if (NULL != md) {
        tkl_system_free(md);
        md = NULL;
    }
    return;
}

void mm_set_mqtt_timeout(int timeout)
{
    if (NULL != mqtt_client) {
        mqtt_set_select_timeout(mqtt_client->mqctx.mqtt_client, timeout);
    }
    return;
}

// called in mqtt start.
static void mm_mqtt_proto_register(tuya_iot_client_t *client)
{
    const tuya_endpoint_t *endpoint = tuya_endpoint_get();
    char a_top_host[65];
    char mqtt_host[65];
    char region[3];
    mqtt_client = client;
    tuya_mqtt_protocol_register(&client->mqctx, PRO_CMD, mm_mqtt_process_proto_5, client); // CMD 5
    tuya_mqtt_protocol_register(&client->mqctx, PRO_CLOUD_STORAGE_ORDER_REQ, mm_mqtt_process_proto_300, client);
    tuya_mqtt_protocol_register(&client->mqctx, 302, mm_mqtt_process_proto_302, client);
    tuya_mqtt_protocol_register(&client->mqctx, PRO_CLOUD_STORAGE_EVENT_REQ, mm_mqtt_process_proto_307, client);
    tuya_mqtt_protocol_register(&client->mqctx, PRO_DOORBELL_STATUS_REQ, mm_mqtt_process_proto_308, client);
    tuya_mqtt_protocol_register(&client->mqctx, 312, mm_mqtt_process_proto_312, client);
    tuya_mqtt_protocol_register(&client->mqctx, 802, mm_mqtt_process_proto_802, client);
    strncpy(a_top_host, endpoint->atop.host, sizeof(endpoint->atop.host));
    strncpy(mqtt_host, endpoint->mqtt.host, sizeof(endpoint->mqtt.host));
    strncpy(region, endpoint->region, sizeof(endpoint->region));
    uint16_t port = endpoint->mqtt.port;
    ty_system_save_conf(client->activate.devid, client->activate.seckey, client->activate.localkey, a_top_host,
                        mqtt_host, port, region);
    ty_system_report_sdk_config();
    return;
}

// called after mqtt disconnected.
void mm_mqtt_disconnect_cb(void)
{
    PR_INFO("\n%s:%d ***************\n", __func__, __LINE__);
    mm_mqtt_clear_send_queue();
    if (!tuya_keepalive_is_host_sleep()) {
        ty_lp_proto_mqtt_set_stat_cs(0);
    }
    return;
}

int mm_get_mqtt_stat(void)
{
    if (NULL == mqtt_client) {
        return 0;
    }

    if (netmgr_network_available()) {
        return tuya_mqtt_connected(&mqtt_client->mqctx);
    } else {
        return 0;
    }
}

// called after mqtt connected.
void mm_mqtt_connected_cb(tuya_iot_client_t *client)
{
    mqtt_client = client;

    // subscribe mqtt topic.
    if (NULL != mqtt_client) {
        tuya_mqtt_context_t *context = &client->mqctx;
        context->is_connected = true;
        PR_INFO("\n%s:%d before subscribe ************\n", __func__, __LINE__);
        char topic_ext[TUYA_MQTT_TOPIC_MAXLEN];
        memset(topic_ext, 0, sizeof(topic_ext));
        snprintf(topic_ext, sizeof(topic_ext), "/av/d/%s", mqtt_client->activate.devid);
        tuya_mqtt_subscribe_message_callback_register(context, topic_ext, NULL, context);
        // TY_LOGD("SUBSCRIBE sent for topic %s to broker.", context->signature.topic_in);
        PR_DEBUG("SUBSCRIBE sent for topic %s to broker.", topic_ext);
        // mqtt_client_subscribe(mqtt_client->mqctx.mqtt_client, topic_ext, MQTT_QOS_1);
        mm_mqtt_proto_register(mqtt_client);
        mm_set_mqtt_timeout(2000);
    }

    if (!tuya_keepalive_is_host_sleep()) {
        ty_lp_proto_mqtt_set_stat_cs(1);
    }

    return;
}

void mm_mqtt_post_proc(void *args)
{
    (void)args;
    struct mqtt_data *md = NULL;
    while (1) {
        if (NULL == md && NULL != mm_data_queue) {
            tkl_queue_fetch(mm_data_queue, &md, TKL_QUEUE_WAIT_FROEVER);
        }

        if (NULL == md || NULL == mqtt_client) {
            /*PR_ERR( "\n%s:%d Invalid md:%p mqtt_client:%p\n", __func__, __LINE__, md, mqtt_client );*/
            tkl_system_sleep(1000);
            continue;
        }

        PR_INFO("post mqtt proto:%d len:%d\n", md->proto, md->data_len);
        tuya_mqtt_protocol_data_publish_with_topic(&mqtt_client->mqctx, md->data + md->data_len, md->proto,
                                                   (uint8_t *)md->data, md->data_len); // need -1 ??
        if (NULL != md) {
            tkl_system_free(md);
            md = NULL;
        }
    }
    tkl_thread_release(NULL);
    mm_mqtt_send_thread = NULL;
    return;
}

void mm_mqtt_clear_send_queue(void)
{
    struct mqtt_data *md = NULL;
    if (NULL == mm_data_queue) {
        PR_ERR("queue not initialized!");
        return;
    }
    while (NULL == md) {
        int ret = 0;
        ret = tkl_queue_fetch(mm_data_queue, &md, 0);
        if (ret != OPRT_OK) {
            break;
        }
        if (NULL != md) {
            tkl_system_free(md);
            md = NULL;
        }
    }
    return;
}

void mm_mqtt_deinit(void)
{
    if (NULL != mm_data_queue) {
        mm_mqtt_clear_send_queue();
        tkl_queue_free(mm_data_queue);
        mm_data_queue = NULL;
    }
    if (NULL != mm_mqtt_online) {
        tkl_semaphore_release(mm_mqtt_online);
        mm_mqtt_online = NULL;
    }
    if (NULL != mm_mqtt_send_thread) {
        tkl_thread_release(mm_mqtt_send_thread);
        mm_mqtt_send_thread = NULL;
    }
    mqtt_client = NULL;
    return;
}

void mm_mqtt_init(void)
{
    int ret = 0;
    mqtt_client = NULL;
    ret = tkl_queue_create_init(&mm_data_queue, sizeof(void *), MM_DATA_QUEUE_SIZE);
    if (OPRT_OK != ret) {
        PR_ERR("failed in tkl_queue_create_init.");
        goto err;
    }
    ret = tkl_semaphore_create_init(&mm_mqtt_online, 0, 1);
    if (OPRT_OK != ret) {
        PR_ERR("failed in tkl_semaphore_create_init.");
        goto err;
    }
    ret = tkl_thread_create(&mm_mqtt_send_thread, "mm_mqtt_send_thread", MQ_THREAD_STACK_SIZE, MQ_THREAD_PRIORITY,
                            mm_mqtt_post_proc, NULL);
    if (OPRT_OK != ret) {
        PR_ERR("failed in tkl_queue_create_init.");
        goto err;
    }

    return;
err:
    mm_mqtt_deinit();
    return;
}

void mm_mqtt_process_upgrade_info(cJSON *upgrade)
{
    if (!upgrade) {
        return;
    }
    // mm_wakeup_wrapper(WAKEUP_GW);
    tuya_wakeup_reason_handler(WAKEUP_GW);
    ty_lp_proto_sdk_set_upgrade_url_cs(upgrade);
}

void mm_mqtt_process_sub_upgrade_info(void)
{
    // mm_wakeup_wrapper(WAKEUP_GW);
    tuya_wakeup_reason_handler(WAKEUP_GW);
    tkl_system_sleep(1000);
    ty_lp_proto_set_ota_start_cs();
}

int mm_mqtt_post_online_sem(void)
{
    if (mm_mqtt_online != NULL) {
        if (tkl_semaphore_post(mm_mqtt_online) == 0) {
            return OPRT_OK;
        } else {
            PR_ERR("post mqtt semaphore failed");
        }
    } else {
        PR_ERR("mqtt semaphore not creat!");
    }
    return OPRT_COM_ERROR;
}

void mm_active_finished_cb(tuya_iot_client_t *client)
{
    int ret;
    const tuya_endpoint_t *endpoint = tuya_endpoint_get();
    char a_top_host[65];
    char mqtt_host[65];
    char region[3];
    strncpy(a_top_host, endpoint->atop.host, sizeof(endpoint->atop.host));
    strncpy(mqtt_host, endpoint->mqtt.host, sizeof(endpoint->mqtt.host));
    strncpy(region, endpoint->region, sizeof(endpoint->region));
    uint16_t port = endpoint->mqtt.port;
    ty_system_save_conf(client->activate.devid, client->activate.seckey, client->activate.localkey, a_top_host,
                        mqtt_host, port, region);
    ty_system_report_sdk_config();
    ty_system_set_time_async();
    PR_INFO("\n%s:%d *********************\n", __func__, __LINE__);

    // block here.
    if (mm_mqtt_online != NULL) {
        ret = tkl_semaphore_wait(mm_mqtt_online, MQTT_ONLINE_MAX_TIMEOUT);
        if (OPRT_OK != ret) {
            PR_ERR("wait for mqtt online after active failed! mqtt force online\n\r");
        }
    } else {
        PR_ERR("semaphre not created!!!\n\r");
    }
    return;
}

#endif /* #if defined(SYSTEM_LINUX) && (OPERATING_SYSTEM != SYSTEM_LINUX) */
#endif /* #if defined(MM_ENABLE_MQTT_KEEPALIVE) */
