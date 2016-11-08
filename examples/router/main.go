package main

import (
	"fmt"
	"math/rand"
	"os"
	"os/signal"
	"time"

	"github.com/TheThingsNetwork/ttn/api/gateway"
	"github.com/TheThingsNetwork/ttn/api/protocol"
	"github.com/TheThingsNetwork/ttn/api/protocol/lorawan"
	"github.com/TheThingsNetwork/ttn/api/router"
	"github.com/TheThingsNetwork/ttn/core/types"
	"github.com/eclipse/paho.mqtt.golang"
	"github.com/golang/protobuf/proto"
)

var fcnt uint32

type config struct {
	m  lorawan.Modulation
	dr string
	br uint32
	f  uint64
}

var configs = []config{
	config{m: lorawan.Modulation_LORA, dr: "SF7BW125", f: 867100000},
	config{m: lorawan.Modulation_LORA, dr: "SF8BW125", f: 867300000},
	config{m: lorawan.Modulation_LORA, dr: "SF9BW125", f: 869525000},
	config{m: lorawan.Modulation_LORA, dr: "SF10BW125", f: 867700000},
	config{m: lorawan.Modulation_LORA, dr: "SF11BW125", f: 867900000},
	config{m: lorawan.Modulation_LORA, dr: "SF12BW125", f: 867500000},
	config{m: lorawan.Modulation_FSK, br: 50000, f: 868800000},
	config{m: lorawan.Modulation_LORA, dr: "SF7BW250", f: 868300000},
	config{m: lorawan.Modulation_LORA, dr: "SF10BW125", f: 915200000},
	config{m: lorawan.Modulation_LORA, dr: "SF9BW125", f: 915400000},
	config{m: lorawan.Modulation_LORA, dr: "SF8BW125", f: 915600000},
	config{m: lorawan.Modulation_LORA, dr: "SF7BW125", f: 915800000},
	config{m: lorawan.Modulation_LORA, dr: "SF12BW500", f: 915900000},
	config{m: lorawan.Modulation_LORA, dr: "SF11BW500", f: 923300000},
	config{m: lorawan.Modulation_LORA, dr: "SF10BW500", f: 923900000},
	config{m: lorawan.Modulation_LORA, dr: "SF9BW500", f: 923300000},
	config{m: lorawan.Modulation_LORA, dr: "SF8BW500", f: 923900000},
	config{m: lorawan.Modulation_LORA, dr: "SF7BW500", f: 923300000},
}

func makeConfig() (*protocol.TxConfiguration, *gateway.TxConfiguration) {
	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	config := configs[r.Intn(len(configs))]

	protoConfig := &protocol.TxConfiguration{
		Protocol: &protocol.TxConfiguration_Lorawan{
			Lorawan: &lorawan.TxConfiguration{
				Modulation: config.m,
				DataRate:   config.dr,
				CodingRate: "4/5",
				FCnt:       fcnt,
			},
		},
	}
	gatewayConfig := &gateway.TxConfiguration{
		Timestamp:             10000 + fcnt*10,
		RfChain:               uint32(r.Intn(2)),
		Frequency:             config.f,
		PolarizationInversion: config.m == lorawan.Modulation_LORA,
		FrequencyDeviation:    config.br / 2,
	}

	return protoConfig, gatewayConfig
}

var payloads = []string{
	"apple", "apricot",
	"avocado", "banana",
	"berry", "blackberry",
	"blood orange", "blueberry",
	"boysenberry", "breadfruit",
}

func makeDownlink() *router.DownlinkMessage {
	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	payload := payloads[r.Intn(len(payloads))]

	message := &lorawan.Message{
		MHDR: lorawan.MHDR{
			MType: lorawan.MType_UNCONFIRMED_DOWN,
			Major: lorawan.Major_LORAWAN_R1,
		},
		Payload: &lorawan.Message_MacPayload{MacPayload: &lorawan.MACPayload{
			FHDR: lorawan.FHDR{
				DevAddr: types.DevAddr{1, 2, 3, 4},
				FCnt:    fcnt,
			},
			FPort:      1,
			FrmPayload: []byte(payload),
		}},
		Mic: []byte{1, 2, 3, 4},
	}
	bytes, _ := message.PHYPayload().MarshalBinary()

	protoConfig, gatewayConfig := makeConfig()

	return &router.DownlinkMessage{
		Payload:               bytes,
		ProtocolConfiguration: protoConfig,
		GatewayConfiguration:  gatewayConfig,
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
	fcnt++
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
