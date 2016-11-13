# Microchip Harmony Demo

This is a demonstration firmware of a project setup for Microchip Harmony. The project has been composed with MPLAB X IDE v3.45 and Microchip Harmony 1.08.1.

For this project to compile, clone this repository and third party dependencies to the right places:

- `git clone https://github.com/TheThingsNetwork/ttn-gateway-connector.git ~/microchip/harmony/v1_08_01/apps/ttn-gateway-connector`
- `git clone https://github.com/TheThingsNetwork/paho.mqtt.embedded-c.git ~/microchip/harmony/v1_08_01/third_party/paho.mqtt.embedded-c`
- `git clone https://github.com/protobuf-c/protobuf-c.git ~/microchip/harmony/v1_08_01/third_party/protobuf-c`

Then, open `~/microchip/harmony/v1_08_01/apps/ttn-gateway-connector/examples/harmony/demo/firmware/demo.X` and make sure that the project compiles.

## Integration

For integrating the library in an existing project, some defines and the include paths have to be set as follows: 

- Define `MQTT_TASK`
- Define `__harmony__`
- Include this library's source
- Include `MQTTClient-C/src` headers of Paho
- Include `MQTTPacket/src` headers of Paho
- Include Protobuf headers
- Include the Gogo protobuf headers (part of this repository)
- Include the The Things Network base (part of this repository)

You can do this in the Makefile or in MPLAB X IDE. In MPLAB X IDE, go to the project properties, the concerning configuration, XC32 and enter the following in the *Additional options* of the `xc32-gcc` section:

```
-DMQTT_TASK -D__harmony__ -I../../src -I../../../../third_party/paho.mqtt.embedded-c/MQTTClient-C/src -I../../../../third_party/paho.mqtt.embedded-c/MQTTPacket/src -I../../../../third_party/protobuf-c -I../../src/github.com/gogo/protobuf/protobuf -I../../src/github.com/TheThingsNetwork
```
