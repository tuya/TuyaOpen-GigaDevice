#ifndef _MM_MQTT_H__
#define _MM_MQTT_H__



void mm_mqtt_init( void );
void mm_mqtt_deinit( void );
//void mm_mqtt_process_proto( tuya_mqtt_event_t* ev );
void mm_mqtt_post_data( char *data, int proto, char* topic);
int mm_mqtt_post_online_sem(void);
void mm_set_mqtt_timeout( int timeout );
int mm_get_mqtt_stat( void );


#endif
