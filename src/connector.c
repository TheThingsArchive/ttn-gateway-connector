// Copyright © 2016 The Things Network
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "network.h"

void ttngwc_init(TTN **s, const char *id, TTNDownlinkHandler downlink_handler,
                 void *cb_arg) {
  struct Session *session = (struct Session *)malloc(sizeof(struct Session));

  session->id = strdup(id);
  session->key = NULL;
  session->downlink_handler = downlink_handler;
  session->cb_arg = cb_arg;
  session->read_buffer = malloc(READ_BUFFER_SIZE);
  session->send_buffer = malloc(SEND_BUFFER_SIZE);

  NetworkInit(&session->network);
  MQTTClientInit(&session->client, &session->network, COMMAND_TIMEOUT,
                 session->send_buffer, SEND_BUFFER_SIZE, session->read_buffer,
                 READ_BUFFER_SIZE);

  *s = (TTN *)session;
}

void ttngwc_cleanup(TTN *s) {
  struct Session *session = (struct Session *)s;

  MQTTClientDestroy(&session->client);

  free(session->id);
  free(session->key);
  free(session->read_buffer);
  free(session->send_buffer);
  free(session);
}

void ttngwc_downlink_cb(struct MessageData *data, void *s) {
  struct Session *session = (struct Session *)s;

  Router__DownlinkMessage *downlink = router__downlink_message__unpack(
      NULL, data->message->payloadlen, data->message->payload);
  if (!downlink)
    return;

  if (session->downlink_handler)
    session->downlink_handler(downlink, session->cb_arg);

  router__downlink_message__free_unpacked(downlink, NULL);
}

int ttngwc_connect(TTN *s, const char *host_name, int port, const char *key) {
  struct Session *session = (struct Session *)s;
  if (key)
    session->key = strdup(key);

  int err;
  MQTTPacket_connectData connect = MQTTPacket_connectData_initializer;
  char *downlink_topic = NULL;

  err = NetworkConnect(&session->network, (char *)host_name, port);
  if (err != SUCCESS)
    goto exit;

  connect.clientID.cstring = session->id;
  connect.keepAliveInterval = KEEP_ALIVE_INTERVAL;
  // Only set credentials when we have a key
  if (key) {
    connect.username.cstring = session->id;
    connect.password.cstring = (char *)key;
  }
#if SEND_DISCONNECT_WILL
  Types__DisconnectMessage will = TYPES__DISCONNECT_MESSAGE__INIT;
  will.id = session->id;
  will.key = (char *)key;
  connect.willFlag = 1;
  connect.will.topicName.cstring = "disconnect";
  connect.will.message.lenstring.len =
      types__disconnect_message__get_packed_size(&will);
  connect.will.message.lenstring.data =
      malloc(connect.will.message.lenstring.len);
  connect.will.qos = QOS_WILL;
  connect.will.retained = 0;
  types__disconnect_message__pack(
      &will, (uint8_t *)connect.will.message.lenstring.data);
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

  asprintf(&downlink_topic, "%s/down", session->id);
  err = MQTTSubscribe(&session->client, downlink_topic, QOS_DOWN,
                      &ttngwc_downlink_cb, session);

exit:
  if (err != SUCCESS && downlink_topic != NULL)
    free(downlink_topic);

  return err;
}

int ttngwc_disconnect(TTN *s) {
  struct Session *session = (struct Session *)s;

#if SEND_DISCONNECT_WILL
  Types__DisconnectMessage will = TYPES__DISCONNECT_MESSAGE__INIT;
  will.id = session->id;
  if (session->key)
    will.key = session->key;
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

  return 0;
}

int ttngwc_send_uplink(TTN *s, Router__UplinkMessage *uplink) {
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
  if (topic != NULL)
    free(topic);
  if (payload != NULL)
    free(payload);
  return rc;
}

int ttngwc_send_status(TTN *s, Gateway__Status *status) {
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
  if (topic != NULL)
    free(topic);
  if (payload != NULL)
    free(payload);
  return rc;
}
