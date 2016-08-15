#include "network.h"
#include <MQTTClient.c>

void ttngwc_init(TTN **s, const char *id) {
	struct Session *session = (struct Session *)malloc(sizeof(struct Session));

	session->id = strdup(id);
	session->read_buffer = malloc(READ_BUFFER_SIZE);
	session->send_buffer = malloc(SEND_BUFFER_SIZE);

	NetworkInit(&session->network);
	MQTTClientInit(&session->client, &session->network, COMMAND_TIMEOUT, session->send_buffer, SEND_BUFFER_SIZE, session->read_buffer, READ_BUFFER_SIZE);

	*s = (TTN *)session;
}

void ttngwc_cleanup(TTN *s) {
	struct Session *session = (struct Session *)s;

	free(session->id);
	free(session->read_buffer);
	free(session->send_buffer);
	free(session);
}

int ttngwc_connect(TTN *s, const char *host_name, int port, const char *key) {
	struct Session *session = (struct Session *)s;

	int err = NetworkConnect(&session->network, (char *)host_name, port);
	if (err != 0) {
		return err;
	}

	MQTTPacket_connectData connect = MQTTPacket_connectData_initializer;

	connect.clientID.cstring = session->id;
	connect.keepAliveInterval = KEEP_ALIVE_INTERVAL;

	// Only set credentials when we have a key
	if (key) {
		connect.username.cstring = session->id;
		connect.password.cstring = (char *)key;
	}

	return MQTTConnect(&session->client, &connect);
}

int ttngwc_disconnect(TTN *s) {
	struct Session *session = (struct Session *)s;

	MQTTDisconnect(&session->client);
	NetworkDisconnect(&session->network);

	return 0;
}

int ttngwc_yield(TTN *s) {
	struct Session *session = (struct Session *)s;

	return MQTTYield(&session->client, YIELD_TIME);
}

int ttngwc_send_status(TTN *s, int64_t time) {
	struct Session *session = (struct Session *)s;
	
	int rc = FAILURE;

	Gateway__Status	status = GATEWAY__STATUS__INIT;
	status.has_time = 1;
	status.time = time;
	// TODO: fill out the other fields

	size_t len = gateway__status__get_packed_size(&status);
	void *payload = malloc(len);
	if (!payload)
		goto exit;

	gateway__status__pack(&status, payload);

	MQTTMessage message;
	message.qos = QOS0;
	message.retained = 0;
	message.dup = 0;
	message.payload = payload;
	message.payloadlen = len;

	char *topic;
	if (asprintf(&topic, "status/%s", session->id) == -1)
		goto exit;

	rc = MQTTPublish(&session->client, topic, &message);

exit:
	free(topic);
	free(payload);
	return rc;
}
