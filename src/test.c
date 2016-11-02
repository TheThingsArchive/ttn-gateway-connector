#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "connector.h"

int running = 1;

void stop(int sig)
{
	signal(SIGINT, NULL);
	running = 0;
}

void downlink(Router__DownlinkMessage *msg, void *arg)
{
	printf("Got downlink\n");
}

int main(int argc, char **argv) {
	srand(time(NULL));

	signal(SIGINT, stop);
	signal(SIGTERM, stop);

	// Initialize the TTN gateway with ID office
	TTN *ttn;
	ttngwc_init(&ttn, "office", &downlink, NULL);
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
		// Send gateway status. The only parameter we're sending is the time
		Gateway__Status status = GATEWAY__STATUS__INIT;
		status.has_time = 1;
		status.time = ++i;

		err = ttngwc_send_status(ttn, &status);
		if (err != 0) {
			printf("Send status failed: %d\n", err);
			break;
		}

		printf("Published time %d\n", i);
		sleep(rand() % 20);
	}

	printf("Disconnecting\n");
	ttngwc_disconnect(ttn);
	ttngwc_cleanup(ttn);

	return 0;
}
