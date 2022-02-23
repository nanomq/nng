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
#include <cJSON.h>

// File
#define FILENAME "demo.txt"

// Subcommands
#define PUBLISH "pub"
#define SUBSCRIBE "sub"

void fatal(const char *, int);
static void disconnect_cb(void *, nng_msg *);
static void connect_cb(void *, nng_msg *);
int client_connect(nng_socket *, const char *);
int client_subscribe(nng_socket, nng_mqtt_topic_qos *, int);
int client_publish(nng_socket, const char *, uint8_t *,uint32_t, uint8_t);
int mqtt_nftp_recver_start(nng_socket, char *);

// char * nftp_topic_cmd    = "test/room1/emqx/client/user1";
char * nftp_topic_cmd    = "test/watson/001";
char * nftp_topic_sender = "file/001";
char * nftp_topic_recver = "file/001";

static int test_log(void * t) {printf("%s\n", t); return 0;}

int
main(const int argc, const char **argv)
{
	nng_socket sock;
	nftp_proto_init();

	// const char *url = "mqtt-tcp://127.0.0.1:1883";
	// const char *url = "mqtt-tcp://broker-cn.emqx.io:1883";
	const char *url = "mqtt-tcp://112.13.203.122:1883";

	client_connect(&sock, url);
	nng_msleep(1000);

	mqtt_nftp_recver_start(sock, FILENAME);

	nftp_proto_fini();
	nng_msleep(1000);
	nng_close(sock);

	return 0;
}

int
mqtt_nftp_recver_start(nng_socket sock, char * fname)
{
	uint8_t * data;
	size_t    datasz;

	// nftp recver register
	nftp_proto_register(fname, test_log,
			(void *)"Specfic file [demo.txt] transfer finished.", NFTP_RECVER);
	nftp_proto_register("*", NULL, NULL, NFTP_RECVER);

	// nng-mqtt sub for recving ack
	nng_mqtt_topic_qos subscriptions[] = {
		{
			.qos   = 1,
			.topic = {
				.buf    = nftp_topic_sender,
				.length = strlen(nftp_topic_sender)
			}
		},
		{
			.qos   = 1,
			.topic = {
				.buf    = nftp_topic_cmd,
				.length = strlen(nftp_topic_cmd)
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
	nng_mqtt_msg_set_connect_clean_session(connmsg, true);
	nng_mqtt_msg_set_connect_client_id(connmsg, "3c988e4b4ef60f77513a24bbe7699527");
	nng_mqtt_msg_set_connect_user_name(connmsg, "KZXQZI8T-user4");
	nng_mqtt_msg_set_connect_password(connmsg, "ce65b5a71b5a0685a528376a13d553d7");

	uint8_t buff[1024] = { 0 };

	printf("Connecting to server ...");
	nng_dialer_set_ptr(dialer, NNG_OPT_MQTT_CONNMSG, connmsg);
	nng_dialer_set_cb(dialer, &user_cb);
	nng_dialer_start(dialer, NNG_FLAG_NONBLOCK);

	// TODO Connmsg would be free when client disconnected
	return (0);
}

// Subscribe to the given subscriptions, and start receiving messages forever.
int
client_subscribe(nng_socket sock, nng_mqtt_topic_qos *subscriptions, int count)
{
	int rv;

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
	while(1) {
		nng_msg *msg;
		int rv;
		uint8_t *payload;
		uint32_t payload_len;
		uint8_t *retpayload;
		uint32_t retpayload_len;
		char * topic, *topicTmp;
		uint32_t topic_len;
		cJSON * cmdjson = NULL;
		cJSON * cmd_str = NULL, *recvpath_str = NULL, *filename_str = NULL;

		if ((rv = nng_recvmsg(sock, &msg, 0)) != 0)
			fatal("nng_recvmsg", rv);

		// we should only receive publish messages
		if (nng_mqtt_msg_get_packet_type(msg) != NNG_MQTT_PUBLISH)
			continue;
		
		topicTmp = nng_mqtt_msg_get_publish_topic(msg, &topic_len);
		topic = strndup(topicTmp, topic_len);
		payload = nng_mqtt_msg_get_publish_payload(msg, &payload_len);

		printf("\ntopic is %s. \n", topic);
		printf("payload is %s. \n\n", payload);
		if (0 == strcmp(topic, nftp_topic_cmd)) {
			cmdjson = cJSON_Parse(payload);
			if (NULL == cmdjson) {
				char *error_ptr = cJSON_GetErrorPtr();
				printf("Error before: %s\n", error_ptr);
				goto next;
			}

			cmd_str = cJSON_GetObjectItemCaseSensitive(cmdjson, "cmd");
			if (cJSON_IsNull(cmd_str)) {
				printf("cmd is null\n");
				goto next;
			}
			if (!cJSON_IsString(cmd_str)) {
				printf("cmd is not string [%d]\n", cmd_str->type);
				goto next;
			}
			if (0 != strcmp("recv", cmd_str->valuestring)) {
				printf("type is not recv [%s]\n", cmd_str->valuestring);
				goto next;
			}

			recvpath_str = cJSON_GetObjectItemCaseSensitive(cmdjson, "recvpath");
			if (!cJSON_IsString(recvpath_str)) {
				printf("recvpath is not string\n");
				goto next;
			}
			printf("recvpath is %s\n", recvpath_str->valuestring);
			if (0 == strcmp("", recvpath_str->valuestring))
				nftp_set_recvdir("./");
			else
				nftp_set_recvdir(recvpath_str->valuestring);

			filename_str = cJSON_GetObjectItemCaseSensitive(cmdjson, "filename");
			if (!cJSON_IsString(filename_str)) {
				printf("filename is not string\n");
				goto next;
			}
			printf("filename is %s\n", filename_str->valuestring);
			goto next;
		}

		// nftp protocol handler
		rv = 1;
		if (0 == strcmp(topic, nftp_topic_sender)) {
			printf("nftp handler start.\n");
			rv = nftp_proto_handler(payload, payload_len, &retpayload, &retpayload_len);
		}

		if (rv != 0) {
			printf("Abandon the msg (Not comply to NFTP)\n");
			goto next;
		}

		if (retpayload != NULL && retpayload_len != 0) {
			printf("Recved NFTP_HELLO\n");
			client_publish(sock, nftp_topic_recver, retpayload, retpayload_len, 1);
			printf("Sent NFTP_ACK\n");
		} else {
			printf("Recved NFTP_FILE/END\n");
		}

next:
		free(topic);
		payload = NULL;  topic = NULL;
		payload_len = 0; topic_len = 0;

		nng_msg_free(msg);
		cJSON_Delete(cmdjson);
		nng_msleep(200);
	}

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
