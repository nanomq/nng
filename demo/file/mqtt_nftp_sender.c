// Author: wangha <wangha at emqx dot io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//
// This is a demo for file transfer using mqtt which based nftp submodule.
//


#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <nng/mqtt/mqtt_client.h>
#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>

#include <nftp.h>

// File
#define FILENAME "demo.txt"

// Subcommands
#define PUBLISH "pub"
#define SUBSCRIBE "sub"
#define TOPIC_FILES "files"

void fatal(const char *, int);
static void disconnect_cb(void *, nng_msg *);
static void connect_cb(void *, nng_msg *);
int client_connect(nng_socket *, const char *);
int client_subscribe(nng_socket, nng_mqtt_topic_qos *, int);
int client_publish(nng_socket, const char *, uint8_t *,uint32_t, uint8_t);
int mqtt_nftp_sender_start(nng_socket, char *);

char * nftp_topic_sender = "sender----recver";
char * nftp_topic_recver = "recver----sender";

char * topic_files = TOPIC_FILES;

static int test_log(void * t) { printf("%s\n", (char *)t); return 0;}

int
main(const int argc, const char **argv)
{
	nng_socket sock;
	nftp_proto_init();

	const char *url = "mqtt-tcp://127.0.0.1:1883";
	// const char *url = "mqtt-tcp://192.168.23.105:1885";

	if (argc > 1) {
		topic_files = argv[1];
	}

	client_connect(&sock, url);
	nng_msleep(1000);

	mqtt_nftp_sender_start(sock, FILENAME);
	nftp_proto_fini();

	nng_msleep(1000);
	nng_close(sock);

	return 0;
}

int
mqtt_nftp_sender_start(nng_socket sock, char * fname)
{
	// nng-mqtt sub for recving ack
	nng_mqtt_topic_qos subscriptions[] = {
		{
			.qos   = 1,
			.topic = {
				.buf    = nftp_topic_recver,
				.length = strlen(nftp_topic_recver)
			}
		},
		{
			.qos  = 1,
			.topic = {
				.buf    = topic_files,
				.length = strlen(topic_files),
			}
		},
	};

	client_subscribe(sock, subscriptions, 2);
}

nng_mqtt_cb user_cb = {
	.name            = "user_cb",
	.on_connected    = connect_cb,
	.on_disconnected = disconnect_cb,
	.connect_arg     = "Args",
	.disconn_arg     = "Args",
};

static void
connect_cb(void *arg, nng_msg *ackmsg)
{
	char *  userarg = (char *) arg;
	uint8_t status  = nng_mqtt_msg_get_connack_return_code(ackmsg);
	printf("Connected cb. \n"
	       "  -> Status  [%d]\n"
	       "  -> Userarg [%s].\n",
	    status, userarg);

	// Free ConnAck msg
	nng_msg_free(ackmsg);
}

static void
disconnect_cb(void *disconn_arg, nng_msg *msg)
{
	(void) disconn_arg;
	printf("%s\n", __FUNCTION__);
}

// Connect to the given address.
int
client_connect(nng_socket *sock, const char *url)
{
	nng_dialer dialer;
	int        rv;

	if ((rv = nng_mqtt_client_open(sock)) != 0) {
		fatal("nng_socket", rv);
	}

	if ((rv = nng_dialer_create(&dialer, *sock, url)) != 0) {
		fatal("nng_dialer_create", rv);
	}

	// create a CONNECT message
	/* CONNECT */
	nng_msg *connmsg;
	nng_mqtt_msg_alloc(&connmsg, 0);
	nng_mqtt_msg_set_packet_type(connmsg, NNG_MQTT_CONNECT);
	nng_mqtt_msg_set_connect_proto_version(connmsg, 4);
	nng_mqtt_msg_set_connect_keep_alive(connmsg, 60);
	nng_mqtt_msg_set_connect_will_msg(connmsg, "bye-bye", strlen("bye-bye"));
	nng_mqtt_msg_set_connect_will_topic(connmsg, "will_topic");
	nng_mqtt_msg_set_connect_clean_session(connmsg, true);

	uint8_t buff[1024] = { 0 };

	printf("Connecting to server ...");
	nng_dialer_set_ptr(dialer, NNG_OPT_MQTT_CONNMSG, connmsg);
	nng_dialer_set_cb(dialer, &user_cb);
	nng_dialer_start(dialer, NNG_FLAG_NONBLOCK);

	printf("connected\n");

	// TODO Connmsg would be free when client disconnected
	return (0);
}

// Subscribe to the given subscriptions, and start receiving messages forever.
int
client_subscribe(nng_socket sock, nng_mqtt_topic_qos *subscriptions, int count)
{
	int rv;
	char *fname = NULL;

	// create a SUBSCRIBE message
	nng_msg *submsg;
	nng_mqtt_msg_alloc(&submsg, 0);
	nng_mqtt_msg_set_packet_type(submsg, NNG_MQTT_SUBSCRIBE);

	nng_mqtt_msg_set_subscribe_topics(submsg, subscriptions, count);

	uint8_t buff[1024] = { 0 };

	printf("Subscribing ...");
	if ((rv = nng_sendmsg(sock, submsg, 0)) != 0) {
		fatal("nng_sendmsg", rv);
	}
	printf("done.\n");

	nng_msg_free(submsg);

	printf("Start receiving loop:\n");
	while (1) {
		nng_msg *msg;
		int rv;
		uint8_t *payload;
		uint32_t payload_len;
		uint8_t *retpayload;
		uint32_t retpayload_len;
		char     topic[40];
		uint32_t topic_len;
		uint8_t *data;
		size_t   datasz;
		char    *buf;

		if ((rv = nng_recvmsg(sock, &msg, 0)) != 0) {
			fatal("nng_recvmsg", rv);
		}

		// we should only receive publish messages
		assert(nng_mqtt_msg_get_packet_type(msg) == NNG_MQTT_PUBLISH);

		payload = nng_mqtt_msg_get_publish_payload(msg, &payload_len);
		buf = nng_mqtt_msg_get_publish_topic(msg, &topic_len);
		strncpy(topic, buf, topic_len); topic[topic_len] = '\0';
		// fprintf(stderr, "topic %s \n", topic);

		rv = nftp_proto_handler(payload, payload_len, &retpayload, &retpayload_len);

		if (0 == strcmp(topic_files, topic)) {
			fname = malloc(sizeof(char) *(topic_len + 1));
			strncpy(fname, payload, payload_len);
			fname[payload_len] = '\0';

			if (0 == nftp_file_exist(fname)) {
				printf("No such file [%s]\n", fname);
				exit(0);
			}
			// nftp filename register
			printf("File [%s] registered\n", fname);
			nftp_proto_register(fname, test_log,
			    (void *) "File transfer finished.", NFTP_SENDER);

			// For sender
			nftp_proto_send_start(fname);
			nftp_proto_maker(fname, NFTP_TYPE_HELLO, 0, &data, &datasz);

			client_publish(sock, nftp_topic_sender, data, datasz, 1);
			printf("Send NFTP_HELLO\n");
		} else {
			if (fname == NULL) {
				printf("fname is empty\n");
				continue;
			}
			if (rv == 0 && retpayload == NULL) {
				int blocks;
				printf("Recv NFTP_ACK\n");
				nftp_file_blocks(fname, &blocks);
				for (int i = 0; i < blocks; ++i) {
					if (0 != nftp_proto_maker(fname,
					        NFTP_TYPE_FILE, i, &payload, &payload_len)) {
						fatal("nftp_proto_maker", rv);
					}
					client_publish(sock, nftp_topic_sender, payload, payload_len, 1);
					nng_msleep(200);
					printf("SEND NFTP_FILE/END\n");
					free(payload);
				}
			}
			printf("Send Done.\n");
			nng_msg_free(msg);
			break;
		}

		nng_msg_free(msg);
	};

	return rv;
}

// Publish a message to the given topic and with the given QoS.
int
client_publish(nng_socket sock, const char *topic, uint8_t *payload,
    uint32_t payload_len, uint8_t qos)
{
	int rv;

	// create a PUBLISH message
	nng_msg *pubmsg;
	nng_mqtt_msg_alloc(&pubmsg, 0);
	nng_mqtt_msg_set_packet_type(pubmsg, NNG_MQTT_PUBLISH);
	nng_mqtt_msg_set_publish_dup(pubmsg, 0);
	nng_mqtt_msg_set_publish_qos(pubmsg, qos);
	nng_mqtt_msg_set_publish_retain(pubmsg, 0);
	nng_mqtt_msg_set_publish_payload(
	    pubmsg, (uint8_t *) payload, payload_len);
	nng_mqtt_msg_set_publish_topic(pubmsg, topic);

	printf("Publishing to '%s' ...\n", topic);
	if ((rv = nng_sendmsg(sock, pubmsg, NNG_FLAG_NONBLOCK)) != 0) {
		fatal("nng_sendmsg", rv);
	}

	return rv;
}

void
fatal(const char *msg, int rv)
{
	fprintf(stderr, "%s: %s\n", msg, nng_strerror(rv));
	exit(1);
}
