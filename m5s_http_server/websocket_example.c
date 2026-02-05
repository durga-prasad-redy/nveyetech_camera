#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* for sleep() */
#include <pthread.h> /* for mutex */

#include "civetweb.h"

/* Global options for this example. */
static const char WS_URL[] = "/wsURL";
static const char *SERVER_OPTIONS[] =
    {"listening_ports", "8081", "num_threads", "10", NULL, NULL};

/* Define websocket sub-protocols. */
static const char subprotocol_bin[] = "Company.ProtoName.bin";
static const char subprotocol_json[] = "Company.ProtoName.json";
static const char *subprotocols[] = {subprotocol_bin, subprotocol_json, NULL};
static struct mg_websocket_subprotocols wsprot = {2, subprotocols};

/* Exit flag for the server */
volatile int g_exit = 0;

/* User defined data structure for websocket client context. */
struct tClientContext {
	uint32_t connectionNumber;
};

/* Linked list to store active connections */
typedef struct ClientNode {
    struct mg_connection *conn;
    struct ClientNode *next;
} ClientNode;

ClientNode *g_clients = NULL;
pthread_mutex_t g_clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Helper to add client to list */
void add_client(struct mg_connection *conn) {
    pthread_mutex_lock(&g_clients_mutex);
    ClientNode *new_node = (ClientNode *)calloc(1, sizeof(ClientNode));
    new_node->conn = conn;
    new_node->next = g_clients;
    g_clients = new_node;
    pthread_mutex_unlock(&g_clients_mutex);
}

/* Helper to remove client from list */
void remove_client(const struct mg_connection *conn) {
    pthread_mutex_lock(&g_clients_mutex);
    ClientNode **current = &g_clients;
    while (*current) {
        if ((*current)->conn == conn) {
            ClientNode *to_free = *current;
            *current = (*current)->next;
            free(to_free);
            break;
        }
        current = &(*current)->next;
    }
    pthread_mutex_unlock(&g_clients_mutex);
}

/* Handler for new websocket connections. */
static int
ws_connect_handler(const struct mg_connection *conn, void *user_data)
{
	(void)user_data; /* unused */

	/* Allocate data for websocket client context, and initialize context. */
	struct tClientContext *wsCliCtx =
	    (struct tClientContext *)calloc(1, sizeof(struct tClientContext));
	if (!wsCliCtx) {
		/* reject client */
		return 1;
	}
	static uint32_t connectionCounter = 0;
	wsCliCtx->connectionNumber = __sync_add_and_fetch(&connectionCounter, 1);
	mg_set_user_connection_data(conn, wsCliCtx);

	const struct mg_request_info *ri = mg_get_request_info(conn);
	printf("Client %u connected with subprotocol: %s\n",
	       wsCliCtx->connectionNumber,
	       ri->acceptedWebSocketSubprotocol);

	return 0;
}

/* Handler indicating the client is ready to receive data. */
static void
ws_ready_handler(struct mg_connection *conn, void *user_data)
{
	(void)user_data; /* unused */

	struct tClientContext *wsCliCtx =
	    (struct tClientContext *)mg_get_user_connection_data(conn);

	/* Add to global list */
	add_client(conn);

	/* Send initial "hello" message. */
	const char *hello = "hello";
	size_t hello_len = strlen(hello);
	mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, hello, hello_len);

	printf("Client %u ready to receive data\n", wsCliCtx->connectionNumber);
}

/* Handler indicating the client sent data to the server. */
static int
ws_data_handler(struct mg_connection *conn,
                int opcode,
                char *data,
                size_t datasize,
                void *user_data)
{
	(void)user_data; /* unused */
    struct tClientContext *wsCliCtx =
	    (struct tClientContext *)mg_get_user_connection_data(conn);

	/* Just print received data */
    printf("Received %lu bytes from client %u\n", (unsigned long)datasize, wsCliCtx->connectionNumber);

	return 1; /* Keep connection open */
}

/* Handler indicating the connection to the client is closing. */
static void
ws_close_handler(const struct mg_connection *conn, void *user_data)
{
	(void)user_data; /* unused */

	struct tClientContext *wsCliCtx =
	    (struct tClientContext *)mg_get_user_connection_data(conn);

	printf("Client %u closing connection\n", wsCliCtx->connectionNumber);

    /* Remove from global list */
    remove_client(conn);

	free(wsCliCtx);
}

int
main(int argc, char *argv[])
{
	/* Initialize CivetWeb library */
	mg_init_library(0);

	/* Start the server */
	struct mg_callbacks callbacks = {0};
	void *user_data = NULL;

	struct mg_init_data mg_start_init_data = {0};
	mg_start_init_data.callbacks = &callbacks;
	mg_start_init_data.user_data = user_data;
	mg_start_init_data.configuration_options = SERVER_OPTIONS;

	struct mg_error_data mg_start_error_data = {0};
	char errtxtbuf[256] = {0};
	mg_start_error_data.text = errtxtbuf;
	mg_start_error_data.text_buffer_size = sizeof(errtxtbuf);

	struct mg_context *ctx =
	    mg_start2(&mg_start_init_data, &mg_start_error_data);
	if (!ctx) {
		fprintf(stderr, "Cannot start server: %s\n", errtxtbuf);
		mg_exit_library();
		return 1;
	}

	/* Register the websocket callback functions. */
	mg_set_websocket_handler_with_subprotocols(ctx,
	                                           WS_URL,
	                                           &wsprot,
	                                           ws_connect_handler,
	                                           ws_ready_handler,
	                                           ws_data_handler,
	                                           ws_close_handler,
	                                           user_data);

	printf("Websocket server running on port %s\n", SERVER_OPTIONS[1]);
    printf("Sending 'hello' to all clients every second...\n");

	while (!g_exit) {
		sleep(1);
        
        /* Broadcast "hello" to all connected clients */
        pthread_mutex_lock(&g_clients_mutex);
        ClientNode *current = g_clients;
        while (current) {
            mg_websocket_write(current->conn, MG_WEBSOCKET_OPCODE_TEXT, "hello", 5);
            current = current->next;
        }
        pthread_mutex_unlock(&g_clients_mutex);
	}

	printf("Websocket server stopping\n");

	mg_stop(ctx);
	mg_exit_library();
    return 0;
}
