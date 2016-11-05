#if !defined(__TTN_GW_H_)
#define __TTN_GW_H_

#if defined(__linux__)
	#include <stdint.h>
#endif

#include "github.com/TheThingsNetwork/ttn/api/gateway/gateway.pb-c.h"
#include "github.com/TheThingsNetwork/ttn/api/router/router.pb-c.h"

typedef void TTN;
typedef void (*TTNDownlinkHandler)(Router__DownlinkMessage *, void *);

void ttngwc_init(TTN **session, const char *id, TTNDownlinkHandler, void *);
void ttngwc_cleanup(TTN *session);
int ttngwc_connect(TTN *session, const char *host_name, int port, const char *key);
int ttngwc_disconnect(TTN *session);
int ttngwc_send_uplink(TTN *session, Router__UplinkMessage *uplink);
int ttngwc_send_status(TTN *session, Gateway__Status *status);

#endif
