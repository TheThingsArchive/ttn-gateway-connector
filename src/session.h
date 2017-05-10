// Copyright © 2016 The Things Network
// Use of this source code is governed by the MIT license that can be found in the LICENSE file.

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
  char *key;
  char *downlink_topic;
};

#endif
