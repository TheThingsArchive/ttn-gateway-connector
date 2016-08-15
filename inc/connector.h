#if !defined(__TTN_GW_H_)
#define __TTN_GW_H_

#if defined(__linux__)
	#include <stdint.h>
#endif

typedef void TTN;

void ttngwc_init(TTN **session, const char *id);
void ttngwc_cleanup(TTN *session);
int ttngwc_connect(TTN *session, const char *host_name, int port, const char *key);
int ttngwc_disconnect(TTN *session);
int ttngwc_yield(TTN *session);
int ttngwc_send_status(TTN *session, int64_t time);

#endif
