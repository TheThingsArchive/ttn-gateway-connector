#if !defined(__TTN_GW_NETWORK_H_)
#define __TTN_GW_NETWORK_H_

#define MQTT_TASK 1

#include <MQTTClient.h>
#include <MQTTPacket.h>

#include "connector.h"
#include "session.h"
#include "github.com/TheThingsNetwork/ttn/api/router/router.pb-c.h"

#define KEEP_ALIVE_INTERVAL 20
#define COMMAND_TIMEOUT     1000
#define READ_BUFFER_SIZE    100
#define SEND_BUFFER_SIZE    100

#define QOS_STATUS   QOS1
#define QOS_DOWN     QOS1
#define QOS_UP       QOS1

#endif
