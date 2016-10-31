#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include "connector.h"

int running = 1;

void stop(int sig)
{
	signal(SIGINT, NULL);
	running = 0;
}

int main(int argc, char **argv) {
	signal(SIGINT, stop);
	signal(SIGTERM, stop);

	// Initialize the TTN gateway with ID office
	TTN *ttn;
	ttngwc_init(&ttn, "office");
	if (!ttn) {
		printf("Failed to initialize TTN gateway\n");
		return -1;
	}

	// Connect to the test broker. The secret key is NULL for simplicity
	printf("Connecting...\n");
	int err = ttngwc_connect(ttn, "localhost", 1883, NULL);
	if (err != 0) {
		printf("Connect failed: %d\n", err);
		ttngwc_cleanup(ttn);
		return err;
	}
	printf("Connected\n");

	// Start the loop
	int i = 0;
	while (running) {
		// Handle data. You want to do this on a thread
		err = ttngwc_yield(ttn);
		if (err != 0) {
			break;
		}

		// Send gateway status. The only parameter we're sending is the time
		err = ttngwc_send_status(ttn, ++i);
		if (err != 0) {
			printf("Send status failed: %d\n", err);
			break;
		}

		printf("Published time %d\n", i);
	}

	printf("Disconnecting\n");
	ttngwc_disconnect(ttn);
	ttngwc_cleanup(ttn);

	return 0;
}
