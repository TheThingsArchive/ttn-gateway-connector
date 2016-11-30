#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "connector.h"

int running = 1;

void stop(int sig) {
  signal(SIGINT, NULL);
  running = 0;
}

void print_downlink(Router__DownlinkMessage *msg, void *arg) {
  if (msg->has_payload) {
    printf("down: have %zu bytes downlink\n", msg->payload.len);
    switch (msg->protocol_configuration->protocol_case) {
    case PROTOCOL__TX_CONFIGURATION__PROTOCOL_LORAWAN: {
      Lorawan__TxConfiguration *lora = msg->protocol_configuration->lorawan;
      printf("down: modulation: %d, data rate: %s, bit rate: %d, coding rate: "
             "%s, fcnt: %d\n",
             lora->modulation, lora->data_rate, lora->bit_rate,
             lora->coding_rate, lora->f_cnt);

      Gateway__TxConfiguration *gtw = msg->gateway_configuration;
      printf("down: timestamp: %d, rf chain: %d, frequency: %llu, power: %d, "
             "polarization inversion: %d, frequency deviation: %d\n",
             gtw->timestamp, gtw->rf_chain, gtw->frequency, gtw->power,
             gtw->polarization_inversion, gtw->frequency_deviation);

      break;
    }
    default:
      printf("down: invalid protocol %d\n",
             msg->protocol_configuration->protocol_case);
      break;
    }
  }
}

int main(int argc, char **argv) {
  srand(time(NULL));

  signal(SIGINT, stop);
  signal(SIGTERM, stop);

  // Initialize the TTN gateway with ID test
  TTN *ttn;
  ttngwc_init(&ttn, "test", &print_downlink, NULL);
  if (!ttn) {
    printf("failed to initialize TTN gateway\n");
    return -1;
  }

  // Connect to the test broker. The secret key is NULL for simplicity
  printf("connecting...\n");
  int err = ttngwc_connect(ttn, "localhost", 1883, NULL);
  if (err != 0) {
    printf("connect failed: %d\n", err);
    ttngwc_cleanup(ttn);
    return err;
  }
  printf("connected\n");

  // Start the loop
  int i = 0;
  while (running) {
    // Send gateway status. The only parameter we're sending is the time
    Gateway__Status status = GATEWAY__STATUS__INIT;
    status.has_time = 1;
    status.time = ++i;
    err = ttngwc_send_status(ttn, &status);
    if (err)
      printf("status: send failed: %d\n", err);
    else
      printf("status: sent with time %d\n", i);

    // Enter the payload
    unsigned char buf[] = {0x1, 0x2, 0x3, 0x4, 0x5};
    Router__UplinkMessage up = ROUTER__UPLINK_MESSAGE__INIT;
    up.has_payload = 1;
    up.payload.len = sizeof(buf);
    up.payload.data = buf;

    // Set protocol metadata
    Protocol__RxMetadata protocol = PROTOCOL__RX_METADATA__INIT;
    protocol.protocol_case = PROTOCOL__RX_METADATA__PROTOCOL_LORAWAN;
    Lorawan__Metadata lorawan = LORAWAN__METADATA__INIT;
    lorawan.has_modulation = 1;
    lorawan.modulation = LORAWAN__MODULATION__LORA;
    lorawan.data_rate = "SF9BW250";
    lorawan.coding_rate = "4/5";
    lorawan.has_f_cnt = 1;
    lorawan.f_cnt = i;
    protocol.lorawan = &lorawan;
    up.protocol_metadata = &protocol;

    // Set gateway metadata
    Gateway__RxMetadata gateway = GATEWAY__RX_METADATA__INIT;
    gateway.has_timestamp = 1;
    gateway.timestamp = 10000 + i * 100;
    gateway.has_rf_chain = 1;
    gateway.rf_chain = 5;
    gateway.has_frequency = 1;
    gateway.frequency = 86800;
    up.gateway_metadata = &gateway;

    // Send uplink message
    err = ttngwc_send_uplink(ttn, &up);
    if (err)
      printf("up: send failed: %d\n", err);
    else
      printf("up: sent with timestamp %d\n", i);

    sleep(rand() % 10);
  }

  printf("disconnecting\n");
  ttngwc_disconnect(ttn);
  ttngwc_cleanup(ttn);

  return 0;
}
