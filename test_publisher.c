/*
 * test_publisher.c
 * Connects to the public test broker and publishes 5 simulated sensor readings.
 * Usage: ./test_publisher
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <mosquitto.h>

#define BROKER      "localhost"
#define PORT        1883
#define TOPIC       "mqtt-lab/test/sensor"
#define MSG_COUNT   5
#define INTERVAL_S  2

/* Generate a simple JSON payload with dummy sensor data */
static void build_payload(char *buf, size_t len, int seq) {
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    char ts[30];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", t);

    /* Simple simulated values */
    float temp     = 18.0f + (float)(rand() % 100) / 20.0f;   /* 18.0 – 23.0 */
    float humidity = 50.0f + (float)(rand() % 300) / 10.0f;   /* 50.0 – 80.0 */

    snprintf(buf, len,
        "{\"seq\":%d,\"station_id\":\"S1\","
        "\"timestamp\":\"%s\","
        "\"temperature_c\":%.1f,"
        "\"humidity_pct\":%.1f}",
        seq, ts, temp, humidity);
}

/* Callback: called when connection is established */
static void on_connect(struct mosquitto *mosq, void *userdata, int rc) {
    if (rc == 0) {
        printf("[publisher] Connected to %s:%d\n", BROKER, PORT);
    } else {
        fprintf(stderr, "[publisher] Connection failed: %s\n",
                mosquitto_connack_string(rc));
        mosquitto_disconnect(mosq);
    }
}

/* Callback: called after each message is published */
static void on_publish(struct mosquitto *mosq, void *userdata, int mid) {
    printf("[publisher] Message %d delivered to broker\n", mid);
}

int main(void) {
    srand((unsigned int)time(NULL));

    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new("mqtt-lab-publisher", true, NULL);
    if (!mosq) {
        fprintf(stderr, "[publisher] Failed to create mosquitto instance\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_publish_callback_set(mosq, on_publish);

    int rc = mosquitto_connect(mosq, BROKER, PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[publisher] Could not connect: %s\n",
                mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    /* Start the network loop in background thread */
    mosquitto_loop_start(mosq);

    char payload[256];
    printf("[publisher] Sending %d messages to topic: %s\n\n", MSG_COUNT, TOPIC);

    for (int i = 1; i <= MSG_COUNT; i++) {
        build_payload(payload, sizeof(payload), i);
        printf("[publisher] Publishing: %s\n", payload);

        rc = mosquitto_publish(mosq, NULL, TOPIC,
                               (int)strlen(payload), payload, 1, false);
        if (rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "[publisher] Publish error: %s\n",
                    mosquitto_strerror(rc));
        }
        sleep(INTERVAL_S);
    }

    printf("\n[publisher] Done. Disconnecting.\n");
    mosquitto_disconnect(mosq);
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
