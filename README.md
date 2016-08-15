# The Things Network Gateway Connector

Embedded C library for The Things Gateway and Linux-based gateways to connect with The Things Network Router.

## Introduction

The connector is an embedded C library intended to use on The Things Gateway and Linux-based gateways to connect with The Things Network.

The connector uses a MQTT connection using the [Paho Embedded C/C++ Library](http://www.eclipse.org/paho/clients/c/embedded/). It uses `MQTTPacket` and `MQTTClient-C` which are not reliant on particular libraries for networking, threading or memory management.

Messages are packed using [Protocol Buffers](https://developers.google.com/protocol-buffers/), a language-neutral, platform-neutral extensible mechanism for serializing structured data. The protos used are [The Things Network API](https://github.com/TheThingsNetwork/ttn/tree/refactor/api) compiled to C sources and headers.

## Is This Necessary?

Yes. This library provides the following advantages over [Semtech's packet forwarder](https://github.com/Lora-net/packet_forwarder) reference implementation:

1. When using TCP/IP, there is an open and more reliable connection with The Things Network as compared to UDP which suffers from congestion and firewalls. It is my intention to use port 80 instead of 1883 (default MQTT) to minimize outgoing firewall restrictions
2. When using MQTT, we build on top of a proven and well-supported messaging protocol as compared to custom headers and breaking changes on message protocol level
3. When using authentication, we can identify gateways according to their ID and security key as compared to user-defined EUIs which deemed unreliable
4. When using Protocol Buffers, we use proven and well-supported (un)packing and we are binary compatible with The Things Network back-end while minimizing payload and allow for protocol changes

## Building

The connector requires a C compiler and [`protobuf-c`](https://github.com/protobuf-c/protobuf-c) to be installed.

Clone the source of the [forked Paho Embedded C/C++ Library](https://github.com/johanstokking/paho.mqtt.embedded-c) (or use [official sources](https://github.com/eclipse/paho.mqtt.embedded-c) when pull requests [#45](https://github.com/eclipse/paho.mqtt.embedded-c/pull/45) and [#46](https://github.com/eclipse/paho.mqtt.embedded-c/pull/46) have been merged). Set its relative path to `PAHO_SRC` in `config.mk`. Build the library with `make` and install with `sudo make install`.

Build `ttn-gateway-connector`:

```
make
```

## Example

```c
// Initialize the TTN gateway with ID office
TTN *ttn;
ttngwc_init(&ttn, "office");
if (!ttn) {
	printf("Failed to initialize TTN gateway\n");
	return -1;
}

// Connect to the test broker. The secret key is NULL for simplicity
printf("Connecting...\n");
int err = ttngwc_connect(ttn, "test.mosquitto.org", 1883, NULL);
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
```

## Testing

The sample application `src/test.c` publishes a message to the Mosquitto test broker `test.mosquitto.org` every second as gateway `office`.

Test `ttn-gateway-connector`:

```
make test
```

Subscribe to the topic, for example: `mosquitto_sub -h test.mosquitto.org -t status/office -d`. The output should look like this:

```
mosquitto_sub -h test.mosquitto.org -t 'status/office' -d
Client mosqsub/31847-dev sending CONNECT
Client mosqsub/31847-dev received CONNACK
Client mosqsub/31847-dev sending SUBSCRIBE (Mid: 1, Topic: status/office, QoS: 0)
Client mosqsub/31847-dev received SUBACK
Subscribed (mid: 1): 0
Client mosqsub/31847-dev received PUBLISH (d0, q0, r0, m0, 'status/office', ... (2 bytes))

Client mosqsub/31847-dev received PUBLISH (d0, q0, r0, m0, 'status/office', ... (2 bytes))

Client mosqsub/31847-dev received PUBLISH (d0, q0, r0, m0, 'status/office', ... (2 bytes))

```

## Next Steps

- Implement platform specific `Timer` and `Network` for Microchip Harmony. See `src/linux/MQTTLinux.h` and `src/linux/MQTTLinux.c` for a Linux example
	- Add the include flag and sources to the `Makefile` (see `TODO`)
	- Include the headers in `src/platform.h` (see `TODO`)
- Implement sending full gateway status and uplink messages, and subscribe to downlink messages. See `ttngw_send_status()` in `src/network.c` for a basic example
- Make the library concurrent. The library we use is in `$PAHO_SRC/MQTTClient-C/src`. Three options:
	1. Enable the background task of `MQTTClient` (see `MQTTRun()`, define `MQTT_TASK`). This task blocks 500 ms until it releases the lock to allow a publish, which is not fast enough for uplink messages
	2. Use non-blocking sockets. This requires refactoring the `waitfor()` method in `MQTTClient`
	3. Rely on the platform's thread safety of sockets

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
git checkout refactor
```

If you do not have Go installed or if you do not have cloned the repository to your `$GOPATH`, set the `GOPATH` variable in `config.mk` to the absolute or relative path where you executed the commands above.

```
make proto
```

### Optional: Update Dependencies

Update the platform dependencies to the latest version of the Paho Embedded C/C++ client.

```
make update-deps
```
