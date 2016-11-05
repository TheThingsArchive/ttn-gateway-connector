#include "network.h"
#include <MQTTClient.c>

void ttngwc_init(TTN **s, const char *id, TTNDownlinkHandler downlink_handler, void *cb_arg)
{
   struct Session *session = (struct Session *)malloc(sizeof(struct Session));

   session->id = strdup(id);
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

int ttngwc_connect(TTN *s, const char *host_name, int port, const char *key)
{
   struct Session *session = (struct Session *)s;
   int err;
   char *downlink_topic = NULL;

   err = NetworkConnect(&session->network, (char *)host_name, port);
   if (err != SUCCESS)
      goto exit;

   MQTTPacket_connectData connect = MQTTPacket_connectData_initializer;

   connect.clientID.cstring = session->id;
   connect.keepAliveInterval = KEEP_ALIVE_INTERVAL;

   // Only set credentials when we have a key
   if (key)
   {
      connect.username.cstring = session->id;
      connect.password.cstring = (char *)key;
   }

   err = MQTTConnect(&session->client, &connect);
   if (err != SUCCESS)
      goto exit;

   err = asprintf(&downlink_topic, "%s/down", session->id);
   if (err == -1)
      goto exit;

   err = MQTTSubscribe(&session->client, downlink_topic, QOS_DOWN, &ttngwc_downlink_cb, session);

exit:
   if (err != SUCCESS)
      free(downlink_topic);
   return err;
}

int ttngwc_disconnect(TTN *s)
{
   struct Session *session = (struct Session *)s;

   MQTTDisconnect(&session->client);
   NetworkDisconnect(&session->network);

   return 0;
}

int ttngwc_send_uplink(TTN *s, Router__UplinkMessage *uplink)
{
      struct Session *session = (struct Session *)s;

   int rc = FAILURE;
   void *payload = NULL;
   char *topic = NULL;

   size_t len = router__uplink_message__get_packed_size(uplink);
   payload = malloc(len);
   if (!payload)
      goto exit;

   router__uplink_message__pack(uplink, payload);

   MQTTMessage message;
   message.qos = QOS_UP;
   message.retained = 0;
   message.dup = 0;
   message.payload = payload;
   message.payloadlen = len;

   if (asprintf(&topic, "%s/up", session->id) == -1)
      goto exit;

   rc = MQTTPublish(&session->client, topic, &message);

exit:
   free(topic);
   free(payload);
   return rc;
}

int ttngwc_send_status(TTN *s, Gateway__Status *status)
{
   struct Session *session = (struct Session *)s;

   int rc = FAILURE;
   void *payload = NULL;
   char *topic = NULL;

   size_t len = gateway__status__get_packed_size(status);
   payload = malloc(len);
   if (!payload)
      goto exit;

   gateway__status__pack(status, payload);

   MQTTMessage message;
   message.qos = QOS_STATUS;
   message.retained = 0;
   message.dup = 0;
   message.payload = payload;
   message.payloadlen = len;

   if (asprintf(&topic, "%s/status", session->id) == -1)
      goto exit;

   rc = MQTTPublish(&session->client, topic, &message);

exit:
   free(topic);
   free(payload);
   return rc;
}
