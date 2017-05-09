// Copyright © 2016 The Things Network
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "network.h"

bool mqttconnected = false;


void ttngwc_init(TTN **s, const char *id, TTNDownlinkHandler downlink_handler, void *cb_arg)
{
   struct Session *session = (struct Session *)malloc(sizeof(struct Session));
   memset(session, 0, sizeof(struct Session));
   
   session->id = malloc(strlen(id)+1);
   session->key = NULL;
   strcpy(session->id, id);
   session->downlink_handler = downlink_handler;
   session->cb_arg = cb_arg;
   session->read_buffer = malloc(READ_BUFFER_SIZE);
   session->send_buffer = malloc(SEND_BUFFER_SIZE);

   NetworkInit(&session->network);
   MQTTClientInit(&session->client, &session->network, COMMAND_TIMEOUT, session->send_buffer, SEND_BUFFER_SIZE, session->read_buffer, READ_BUFFER_SIZE);

   *s = (TTN *)session;
}

void ttngwc_cleanup(TTN *s)
{
   struct Session *session = (struct Session *)s;

   MQTTClientDestroy(&session->client);

    if (session->key != NULL) { 
        free(session->key); // Free only when set (done in connect)
    }
    // Rest is always set (in init)
    free(session->id);
    free(session->read_buffer);
    free(session->send_buffer);
    free(session);
}

void ttngwc_downlink_cb(struct MessageData *data, void *s)
{
   struct Session *session = (struct Session *)s;

   Router__DownlinkMessage *downlink = router__downlink_message__unpack(NULL, data->message->payloadlen, data->message->payload);
   if (!downlink)
      return;

   if (session->downlink_handler)
      session->downlink_handler(downlink, session->cb_arg);

   router__downlink_message__free_unpacked(downlink, NULL);
}


int ttngwc_open_socket(TTN *s, const char *host_name, int port)
{
   struct Session *session = (struct Session *)s;
   int err;
   err = NetworkConnect(&session->network, (char *)host_name, port);
   if (err != SUCCESS) return 1;
   return 0;
   
}

int ttngwc_close_socket(TTN *s)
{
   struct Session *session = (struct Session *)s;
   NetworkDisconnect(&session->network);
   return 0;
   
}

int ttngwc_check_socket(TTN *s)
{
   struct Session *session = (struct Session *)s;
   if (!NET_PRES_SocketIsConnected(session->network.my_socket))
    {
        return 1;
    }
   if (NET_PRES_SocketWasReset(session->network.my_socket))
    {
        return 1;
    }
    if (NET_PRES_SocketWriteIsReady(session->network.my_socket, 1, 1) == 0)
    {
        return 1;
    }  
   return 0;
}

int ttngwc_socket_tls_start(TTN *s)
{
    struct Session *session = (struct Session *)s;
    return NET_PRES_SocketEncryptSocket(session->network.my_socket) ? 0 : -1;
}

int ttngwc_socket_tls_is_busy(TTN *s)
{
    struct Session *session = (struct Session *)s;
    return NET_PRES_SocketIsNegotiatingEncryption(session->network.my_socket) ? 1 : 0;
}

int ttngwc_socket_tls_check(TTN *s)
{
   struct Session *session = (struct Session *)s;
   return NET_PRES_SocketIsSecure(session->network.my_socket) ? 0 : -1;
}

int ttngwc_connect(TTN *s, const char *key)
{
   struct Session *session = (struct Session *)s;
   if (key) session->key = strdup(key);
   int err;
   MQTTPacket_connectData connect = MQTTPacket_connectData_initializer;
/*
   err = NetworkConnect(&session->network, (char *)host_name, port);
   if (err != SUCCESS)
      goto exit;
*/
   connect.clientID.cstring = session->id;
   connect.keepAliveInterval = KEEP_ALIVE_INTERVAL;
   // Only set credentials when we have a key
   if (key)
   {
      connect.username.cstring = session->id;
      connect.password.cstring = (char *)key;
   }
#if SEND_DISCONNECT_WILL
   Types__DisconnectMessage will = TYPES__DISCONNECT_MESSAGE__INIT;
   will.id = session->id;
   if (session->key) will.key = session->key;
   connect.willFlag = 1;
   connect.will.topicName.cstring = "disconnect";
   connect.will.message.lenstring.len = types__disconnect_message__get_packed_size(&will);
   connect.will.message.lenstring.data = malloc(connect.will.message.lenstring.len);
   connect.will.qos = QOS_WILL;
   connect.will.retained = 0;
   types__disconnect_message__pack(&will, (uint8_t *)connect.will.message.lenstring.data);
#endif

   err = MQTTConnect(&session->client, &connect);
#if SEND_DISCONNECT_WILL
   free(connect.will.message.lenstring.data);
#endif
   if (err != SUCCESS)
      goto exit;

#if SEND_CONNECT
   Types__ConnectMessage conn = TYPES__CONNECT_MESSAGE__INIT;
   conn.id = session->id;
   conn.key = (char *)key;
   MQTTMessage message;
   message.qos = QOS_CONNECT;
   message.retained = 0;
   message.dup = 0;
   message.payloadlen = types__connect_message__get_packed_size(&conn);
   message.payload = malloc(message.payloadlen);
   types__connect_message__pack(&conn, (uint8_t *)message.payload);
   MQTTPublish(&session->client, "connect", &message);
   free(message.payload);
#endif

   const char *fmt = "%s/down";
   session->topic = malloc(strlen(fmt) + strlen(session->id));
   if (session->topic == NULL)
      goto exit;
   sprintf(session->topic, fmt, session->id);

   err = MQTTSubscribe(&session->client, session->topic, QOS_DOWN, &ttngwc_downlink_cb, session);
 
exit:
    if (err != SUCCESS) 
    {
        if(session->topic != NULL) {
            free(session->topic);
            session->topic = NULL;
        }
        if(session->key != NULL) {
            free(session->key);
            session->key = NULL;
        }
    }
    else
    {
        mqttconnected = true;
    }
   return err;
}

int ttngwc_disconnect(TTN *s)
{
   struct Session *session = (struct Session *)s;

   if (!mqttconnected)
       return 0;
   
#if SEND_DISCONNECT_WILL
   Types__DisconnectMessage will = TYPES__DISCONNECT_MESSAGE__INIT;
   will.id = session->id;
   if (session->key) will.key = session->key;
   MQTTMessage message;
   message.qos = QOS_WILL;
   message.retained = 0;
   message.dup = 0;
   message.payloadlen = types__disconnect_message__get_packed_size(&will);
   message.payload = malloc(message.payloadlen);
   types__disconnect_message__pack(&will, (uint8_t *)message.payload);
   MQTTPublish(&session->client, "disconnect", &message);
   free(message.payload);
#endif

   MQTTDisconnect(&session->client);
   NetworkDisconnect(&session->network);
   mqttconnected = false;

    if(session->key != NULL) {
        free(session->key);
        session->key = NULL;
    }

    if(session->topic != NULL) {
        free(session->topic);
        session->topic = NULL;
    }

   
   return 0;
}

int ttngwc_send_uplink(TTN *s, Router__UplinkMessage *uplink)
{
   struct Session *session = (struct Session *)s;

   int rc = FAILURE;
   void *payload = NULL;
   char *topic = NULL;

   if (!NET_PRES_SocketIsConnected(session->network.my_socket))
    {
       SYS_DEBUG(SYS_ERROR_ERROR, "CONN: UP: Error: TCP/IP not connected\r\n"); 
       goto exit;
    }
   if (NET_PRES_SocketIsConnected(session->network.my_socket) == 0)
    {
       SYS_DEBUG(SYS_ERROR_ERROR, "CONN: UP: Error: put not ready\r\n"); 
       goto exit;
    }
   
   size_t len = router__uplink_message__get_packed_size(uplink);
   payload = malloc(len);
   if (!payload){
      SYS_DEBUG(SYS_ERROR_ERROR, "CONN: Failed to allocate memory for packet\r\n");
      goto exit;
   }

   router__uplink_message__pack(uplink, payload);

   MQTTMessage message;
   message.qos = QOS_UP;
   message.retained = 0;
   message.dup = 0;
   message.payload = payload;
   message.payloadlen = len;

   const char *fmt = "%s/up";
   topic = malloc(strlen(fmt) + strlen(session->id));
   if (topic == NULL)
      goto exit;
   sprintf(topic, fmt, session->id);

   rc = MQTTPublish(&session->client, topic, &message);

exit:
    if (topic != NULL) free(topic);
    if (payload != NULL) free(payload);
   return rc;
}

int ttngwc_send_status(TTN *s, Gateway__Status *status)
{
   struct Session *session = (struct Session *)s;

   int rc = FAILURE;
   void *payload = NULL;
   char *topic = NULL;
   
   if (!NET_PRES_SocketIsConnected(session->network.my_socket))
    {
        SYS_DEBUG(SYS_ERROR_ERROR, "CONN: Status: Error: TCP/IP not connected\r\n");
        goto exit;
    }
   if (NET_PRES_SocketWriteIsReady(session->network.my_socket, 1, 1) == 0)
    {
        SYS_DEBUG(SYS_ERROR_ERROR, "CONN: Status: Error: put not ready\r\n");
        goto exit;
    }

   size_t len = gateway__status__get_packed_size(status);
   payload = malloc(len);
   if (!payload){
      SYS_DEBUG(SYS_ERROR_ERROR, "CONN: Failed to allocate memory for packet\r\n");
      goto exit;
   }

   gateway__status__pack(status, payload);

   MQTTMessage message;
   message.qos = QOS_STATUS;
   message.retained = 0;
   message.dup = 0;
   message.payload = payload;
   message.payloadlen = len;
   
   const char *fmt = "%s/status";
   topic = malloc(strlen(fmt) + strlen(session->id));
   if (topic == NULL)
      goto exit;
   sprintf(topic, fmt, session->id);
   
   rc = MQTTPublish(&session->client, topic, &message);

exit:
    if (topic != NULL) free(topic);
    if (payload != NULL) free(payload);
   return rc;
}


void MQTT_POLL_RX(TTN *s)
{
   struct Session *session = (struct Session *)s;
   if(mqttconnected) MQTTYield(&session->client,1000);
   //SYS_CONSOLE_MESSAGE("polling\r\n");
}
