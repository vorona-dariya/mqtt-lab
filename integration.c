#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <mosquitto.h>

#define BROKER      "localhost"
#define PORT        1883
#define MSG_COUNT   6
#define INTERVAL_S  2

static int msg_counter = 0;
static const int OUTLIER_N = 4;
static bool message_delivered = false;

typedef struct {
    int station_id;
    char timestamp[20];
    float temperature;
    float humidity;
} SensorData;

int serialize_to_json(SensorData data, char* buffer, size_t buffer_size) {
    if (data.station_id < 1) return 1;
    if (data.timestamp[0] == '\0') return 1;
    if (data.temperature < -30.0f || data.temperature > 50.0f) return 1;
    if (data.humidity < 0.0f || data.humidity > 100.0f) return 1;

    int written = snprintf(buffer, buffer_size,
             "{\"station_id\": %d, \"timestamp\": \"%s\", \"temperature\": %.2f, \"humidity\": %.2f}",
             data.station_id, data.timestamp, data.temperature, data.humidity);

    if (written < 0 || (size_t)written >= buffer_size) {
        return 1;
    }

    return 0;
}

SensorData generate_sensor_data(float* temp) {
    /* Simple simulated values */
    msg_counter++;
    float delta = ((float)rand() / (float)RAND_MAX) - 0.5f;
    *temp += delta;
    if (*temp < -30.0f) *temp = -30.0f;
    if (*temp > 50.0f) *temp = 50.0f;
    float temperature = *temp;
    if (msg_counter % OUTLIER_N == 0) {
        temperature += 15.0f;
        if (temperature > 50.0f) temperature = 50.0f;
    }
    float humidity = 50.0f + (float)(rand() % 300) / 10.0f;   /* 50.0 – 80.0 */

    SensorData data;
    data.station_id = 1;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(data.timestamp, sizeof(data.timestamp), "%Y-%m-%d %H:%M:%S", t);
    data.temperature = temperature;
    data.humidity = humidity;
    return data;
}

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

    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[publish_message] Publish error: %s\n", mosquitto_strerror(rc));
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

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

    printf("\n[publish_message] Done. Disconnected.\n");
    return 0;
}

int main() {
    srand(time(NULL));

    float temp = -30.0f + (rand() % 800) / 10.0f;
    char payload[256];

    for (int i = 1; i <= MSG_COUNT; i++) {
        SensorData data = generate_sensor_data(&temp);
        if (serialize_to_json(data, payload, sizeof(payload)) != 0) {
            fprintf(stderr, "[main] Failed to serialize sensor data\n");
            continue;
        }

        int result = publish_message(payload, "campus/s1");
        if (result != 0) {
            fprintf(stderr, "[main] Failed to publish message #%d\n", i);
        } else {
            printf("[main] Message #%d published successfully\n", i);
        }
        sleep(INTERVAL_S);
    }
    return 0;
}