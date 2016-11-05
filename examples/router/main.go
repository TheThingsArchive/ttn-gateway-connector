package main

import (
	"fmt"
	"os"
	"os/signal"
	"time"

	"github.com/TheThingsNetwork/ttn/api/gateway"
	"github.com/TheThingsNetwork/ttn/api/protocol"
	"github.com/TheThingsNetwork/ttn/api/protocol/lorawan"
	"github.com/TheThingsNetwork/ttn/api/router"
	"github.com/eclipse/paho.mqtt.golang"
	"github.com/golang/protobuf/proto"
)

var fcnt uint32

func makeDownlink() *router.DownlinkMessage {
	return &router.DownlinkMessage{
		Payload: []byte{0x1, 0x2, 0x3},
		ProtocolConfiguration: &protocol.TxConfiguration{
			Protocol: &protocol.TxConfiguration_Lorawan{
				Lorawan: &lorawan.TxConfiguration{
					Modulation: lorawan.Modulation_LORA,
					DataRate:   "SF7BW125",
					BitRate:    300,
					CodingRate: "4/5",
					FCnt:       fcnt,
				},
			},
		},
		GatewayConfiguration: &gateway.TxConfiguration{
			Timestamp:             10000 + fcnt*10,
			RfChain:               100,
			Frequency:             86830,
			PolarizationInversion: false,
			FrequencyDeviation:    10,
		},
	}
}

func sendDownlink(client mqtt.Client) error {
	down := makeDownlink()
	buf, err := proto.Marshal(down)
	if err != nil {
		fmt.Println("failed to marshal downlink", err)
		return nil
	}
	token := client.Publish("test/down", 1, false, buf)
	if token.Wait() && token.Error() != nil {
		fmt.Println("failed to publish downlink", token.Error())
		return token.Error()
	}
	fmt.Println("sent down", down)
	return nil
}

func main() {
	options := mqtt.NewClientOptions().
		AddBroker("tcp://localhost:1883")
	client := mqtt.NewClient(options)

	token := client.Connect()
	if token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	token = client.Subscribe("test/status", 1, func(_ mqtt.Client, m mqtt.Message) {
		status := &gateway.Status{}
		if err := proto.Unmarshal(m.Payload(), status); err != nil {
			fmt.Println("failed to unmarshal status", err)
			return
		}
		fmt.Println("received status", status)
	})
	if token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	token = client.Subscribe("test/up", 1, func(_ mqtt.Client, m mqtt.Message) {
		up := &router.UplinkMessage{}
		if err := proto.Unmarshal(m.Payload(), up); err != nil {
			fmt.Println("failed to unmarshal uplink", err)
			return
		}
		fmt.Println("received up", up)
		sendDownlink(client)
	})
	if token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	go func() {
		for {
			time.Sleep(4 * time.Second)
			sendDownlink(client)
		}
	}()

	fmt.Println("running...")

	s := make(chan os.Signal)
	signal.Notify(s, os.Interrupt)
	<-s
}
