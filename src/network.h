// Copyright © 2016 The Things Network
// Use of this source code is governed by the MIT license that can be found in the LICENSE file.

#if !defined(__TTN_GW_NETWORK_H_)
#define __TTN_GW_NETWORK_H_

#include <MQTTClient.h>
#include <MQTTPacket.h>

#include "connector.h"
#include "session.h"
#include "github.com/TheThingsNetwork/ttn/api/router/router.pb-c.h"

#define KEEP_ALIVE_INTERVAL 20
#define COMMAND_TIMEOUT 2000
#define READ_BUFFER_SIZE 512
#define SEND_BUFFER_SIZE 512

#define QOS_STATUS QOS1
#define QOS_DOWN QOS1
#define QOS_UP QOS1
#define QOS_CONNECT QOS1
#define QOS_WILL QOS1

#endif
