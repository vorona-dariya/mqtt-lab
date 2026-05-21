#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <mosquitto.h>

#define BROKER      "localhost"
#define PORT        1883

static bool message_delivered = false;

static void on_connect(struct mosquitto *mosq, void *userdata, int rc) {
    if (rc == 0) {
        printf("[publish_message] Connected to %s:%d\n", BROKER, PORT);
    } else {
        fprintf(stderr, "[publish_message] Connection failed: %s\n",
                mosquitto_connack_string(rc));
        mosquitto_disconnect(mosq);
    }
}

static void on_publish(struct mosquitto *mosq, void *userdata, int mid) {
    printf("[publish_message] Message %d delivered to broker\n", mid);
    message_delivered = true;
}

int publish_message(char* payload, char* topic) {
    message_delivered = false;
    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new("mqtt-lab-publisher", true, NULL);
    if (!mosq) {
        fprintf(stderr, "[publish_message] Failed to create mosquitto instance\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_publish_callback_set(mosq, on_publish);

    int rc = mosquitto_connect(mosq, BROKER, PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[publish_message] Could not connect: %s\n",
                mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    printf("[publish_message] Sending message to topic: %s\n", topic);
    printf("[publish_message] Message content: %s\n", payload);

    rc = mosquitto_publish(mosq, NULL, topic, (int)strlen(payload), payload, 1, false);

    time_t start = time(NULL);
    while(!message_delivered) {
        mosquitto_loop(mosq, 100, 1);

        if (time(NULL) - start > 5) {
            fprintf(stderr, "[publish_message] Timeout waiting for message delivery\n");
            break;
        }
    }

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[publish_message] Publish error: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    printf("\n[publish_message] Done. Disconnected.\n");
    return 0;
}

int main() {
    int result = publish_message("Hello, MQTT!", "campus/s1");
    if (result != 0) {
        fprintf(stderr, "[main] Failed to publish message\n");
    } else {
        printf("[main] Message published successfully\n");
    }
    return 0;
}