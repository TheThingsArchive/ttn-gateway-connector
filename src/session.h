#if !defined(__TTN_GW_SESSION_H_)
#define __TTN_GW_SESSION_H_

#include <MQTTClient.h>

struct Session {
  Network network;
  MQTTClient client;
  TTNDownlinkHandler downlink_handler;
  void *cb_arg;
  unsigned char *read_buffer;
  unsigned char *send_buffer;
  char *id;
};

#endif
