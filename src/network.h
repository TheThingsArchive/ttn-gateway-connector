#if !defined(__TTN_GW_NETWORK_H_)
#define __TTN_GW_NETWORK_H_

#include "platform.h"
#include <MQTTClient.h>
#include <MQTTPacket.h>

#include "connector.h"
#include "session.h"
#include "github.com/TheThingsNetwork/ttn/api/router/router.pb-c.h"

#define KEEP_ALIVE_INTERVAL 10
#define YIELD_TIME 1000
#define COMMAND_TIMEOUT 1000
#define READ_BUFFER_SIZE 100
#define SEND_BUFFER_SIZE 100

#endif
