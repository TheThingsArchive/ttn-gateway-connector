# The Things Network Gateway Connector

Embedded C library for The Things Gateway and Linux-based gateways to connect with The Things Network Router.

## Introduction

The connector is an embedded C library intended to use on The Things Gateway and Linux-based gateways to connect with The Things Network.

The connector uses a MQTT connection using the [Paho Embedded C/C++ Library](http://www.eclipse.org/paho/clients/c/embedded/). It uses `MQTTPacket` and `MQTTClient-C` which are not reliant on particular libraries for networking, threading or memory management.

Messages are packed using [Protocol Buffers](https://developers.google.com/protocol-buffers/), a language-neutral, platform-neutral extensible mechanism for serializing structured data. The protos used are [The Things Network API](https://github.com/TheThingsNetwork/ttn/tree/master/api) compiled to C sources and headers.

## Is This Necessary?

Yes. This library provides the following advantages over [Semtech's packet forwarder](https://github.com/Lora-net/packet_forwarder) reference implementation:

1. When using TCP/IP, there is an open and more reliable connection with The Things Network as compared to UDP which suffers from congestion and firewalls. It is my intention to use port 80 instead of 1883 (default MQTT) to minimize outgoing firewall restrictions
2. When using MQTT, we build on top of a proven and well-supported messaging protocol as compared to custom headers and breaking changes on message protocol level
3. When using authentication, we can identify gateways according to their ID and security key as compared to user-defined EUIs which deemed unreliable
4. When using Protocol Buffers, we use proven and well-supported (un)packing and we are binary compatible with The Things Network back-end while minimizing payload and allow for protocol changes

## Building

The connector requires a C compiler and [`protobuf-c`](https://github.com/protobuf-c/protobuf-c) to be installed.

Clone the source of the [forked Paho Embedded C/C++ Library](https://github.com/TheThingsNetwork/paho.mqtt.embedded-c). Set its relative path to `PAHO_SRC` in `config.mk` (copy from `config.mk.in`). Build the library with `make` and install with `sudo make install`.

Build `ttn-gateway-connector`:

```
make
```

## Example

```c
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

void print_downlink(Router__DownlinkMessage *msg, void *arg)
{
   if (msg->has_payload)
   {
      printf("down: have %zu bytes downlink\n", msg->payload.len);
      switch (msg->protocol_configuration->protocol_case)
      {
      case PROTOCOL__TX_CONFIGURATION__PROTOCOL_LORAWAN:
      {
         Lorawan__TxConfiguration *lora = msg->protocol_configuration->lorawan;
         printf("down: modulation: %d, data rate: %s, bit rate: %d, coding rate: %s, fcnt: %d\n",
                lora->modulation, lora->data_rate, lora->bit_rate, lora->coding_rate, lora->f_cnt);

         Gateway__TxConfiguration *gtw = msg->gateway_configuration;
         printf("down: timestamp: %d, rf chain: %d, frequency: %llu, power: %d, polarization inversion: %d, frequency deviation: %d\n",
                gtw->timestamp, gtw->rf_chain, gtw->frequency, gtw->power, gtw->polarization_inversion, gtw->frequency_deviation);

         break;
      }
      default:
         printf("down: invalid protocol %d\n", msg->protocol_configuration->protocol_case);
         break;
      }
   }
}

int main(int argc, char **argv)
{
   srand(time(NULL));

   signal(SIGINT, stop);
   signal(SIGTERM, stop);

   // Initialize the TTN gateway with ID test
   TTN *ttn;
   ttngwc_init(&ttn, "test", &print_downlink, NULL);
   if (!ttn)
   {
      printf("failed to initialize TTN gateway\n");
      return -1;
   }

   // Connect to the test broker. The secret key is NULL for simplicity
   printf("connecting...\n");
   int err = ttngwc_connect(ttn, "localhost", 1883, NULL);
   if (err != 0)
   {
      printf("connect failed: %d\n", err);
      ttngwc_cleanup(ttn);
      return err;
   }
   printf("connected\n");

   // Start the loop
   int i = 0;
   while (running)
   {
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
```

## Testing

There is an example Router in `examples/router` which is written in Go. This requires the Go compiler, [see here](https://golang.org/doc/install):

```
cd examples/router
go run main.go
```

The sample application `src/test.c` publishes a message to the MQTT broker on `localhost` every second as gateway `test`.

```
make test
```

Subscribe to the topic, for example: `mosquitto_sub -t test/+ -d`. The output should look like this:

```
➜  ~ mosquitto_sub -t 'test/+' -d
Client mosqsub/14673-Johans-Ma sending CONNECT
Client mosqsub/14673-Johans-Ma received CONNACK
Client mosqsub/14673-Johans-Ma sending SUBSCRIBE (Mid: 1, Topic: test/+, QoS: 0)
Client mosqsub/14673-Johans-Ma received SUBACK
Subscribed (mid: 1): 0
Client mosqsub/14673-Johans-Ma received PUBLISH (d0, q0, r0, m0, 'test/status', ... (2 bytes))

Client mosqsub/14673-Johans-Ma received PUBLISH (d0, q0, r0, m0, 'test/up', ... (43 bytes))

Z
XSF9BW250r4/5xb
               X�R����
Client mosqsub/14673-Johans-Ma received PUBLISH (d0, q0, r0, m0, 'test/status', ... (2 bytes))

Client mosqsub/14673-Johans-Ma received PUBLISH (d0, q0, r0, m0, 'test/up', ... (43 bytes))

Z
XSF9BW250r4/5xb
               X�S����
```

## Next Steps

- Implement platform specific `Timer`, `Mutex`, `Condition` and `Network` for Microchip Harmony

## Update

### Optional: Update Protos

To update the protos to the latest version of The Things Network API, in your `$GOPATH/src` or sources folder:

```
mkdir -p github.com/gogo
cd github.com/gogo
git clone https://github.com/gogo/protobuf.git
cd ../..
mkdir -p github.com/TheThingsNetwork
cd github.com/TheThingsNetwork
git clone https://github.com/TheThingsNetwork/ttn.git
cd ttn
```

If you do not have Go installed or if you do not have cloned the repository to your `$GOPATH`, set the `GOPATH` variable in `config.mk` to the absolute or relative path where you executed the commands above.

```
make proto
```

## License

Source code for The Things Network is released under the MIT License, which can be found in the [LICENSE](LICENSE) file. A list of authors can be found in the [AUTHORS](AUTHORS) file.
