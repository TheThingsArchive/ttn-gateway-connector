#if !defined(__TTN_GW_SESSION_H_)
#define __TTN_GW_SESSION_H_

#include "platform.h"
#include <MQTTClient.h>

struct Session {
	Network network;
	MQTTClient client;
	unsigned char *read_buffer;
	unsigned char *send_buffer;
	char *id;
};

#endif
