// Copyright © 2016 The Things Network
// Use of this source code is governed by the MIT license that can be found in the LICENSE file.

#if !defined(__TTN_GW_H_)
#define __TTN_GW_H_

#define __harmony__

#if defined(__linux__)
#include <stdint.h>
#endif

#define MAX_ID_LENGTH 32
#define SEND_DISCONNECT_WILL 1
#define SEND_CONNECT 1

#include "github.com/TheThingsNetwork/ttn/api/gateway/gateway.pb-c.h"
#include "github.com/TheThingsNetwork/ttn/api/router/router.pb-c.h"
#include "github.com/TheThingsNetwork/gateway-connector-bridge/types/types.pb-c.h"

typedef void TTN;
typedef void (*TTNDownlinkHandler)(Router__DownlinkMessage *, void *);

// Initializes a new session
void ttngwc_init(TTN **session, const char *id, TTNDownlinkHandler, void *);

// Cleans up a message
void ttngwc_cleanup(TTN *session);

// Connects to The Things Network router.
// Returns 0 on success, -1 on failure
int ttngwc_connect(TTN *session, const char *key);

int ttngwc_open_socket(TTN *session, const char *host_name, int port);
int ttngwc_check_socket(TTN *s);

int ttngwc_close_socket(TTN *s);

int ttngwc_socket_tls_start(TTN *s);

int ttngwc_socket_tls_is_busy(TTN *s);

int ttngwc_socket_tls_check(TTN *s);


// Disconnects from The Things Network Router
// Returns always 0
int ttngwc_disconnect(TTN *session);

// Sends uplink message
// Returns 0 on success, -1 on failure or -2 on timeout
int ttngwc_send_uplink(TTN *session, Router__UplinkMessage *uplink);

// Sends status message
// Returns 0 on success, -1 on failure or -2 on timeout
int ttngwc_send_status(TTN *session, Gateway__Status *status);

#endif
