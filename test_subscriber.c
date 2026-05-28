/*
 * test_subscriber.c
 * Connects to the public test broker and prints every message on the lab topic.
 * Usage: ./test_subscriber   (keep running while publisher sends)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>

#define BROKER  "localhost"
#define PORT    1883
#define TOPIC   "campus/s1"

static int msg_count = 0;

typedef struct {
    int   station_id;
    char  timestamp[20];
    float temperature;
    float humidity;
} sensor_reading_t;

typedef struct {
    int code;
    char field[32];
    char error[128];
} validation_error_t;

void getObject(cJSON* content, sensor_reading_t* reading, validation_error_t* error) {
    memset(error, 0, sizeof(validation_error_t));
    
    cJSON* item = NULL;
    
    // Extract station_id
    item = cJSON_GetObjectItem(content, "station_id");
    if (!item || item->type != cJSON_Number) {
        error->code = 1;
        strncpy(error->field, "station_id", sizeof(error->field) - 1);
        strncpy(error->error, "Missing or invalid station_id", sizeof(error->error) - 1);
        return;
    }
    reading->station_id = item->valueint;
    
    // Extract timestamp
    item = cJSON_GetObjectItem(content, "timestamp");
    if (!item || item->type != cJSON_String) {
        error->code = 2;
        strncpy(error->field, "timestamp", sizeof(error->field) - 1);
        strncpy(error->error, "Missing or invalid timestamp", sizeof(error->error) - 1);
        return;
    }
    strncpy(reading->timestamp, item->valuestring, sizeof(reading->timestamp) - 1);
    
    // Extract temperature
    item = cJSON_GetObjectItem(content, "temperature");
    if (!item || item->type != cJSON_Number) {
        error->code = 3;
        strncpy(error->field, "temperature", sizeof(error->field) - 1);
        strncpy(error->error, "Missing or invalid temperature", sizeof(error->error) - 1);
        return;
    }
    reading->temperature = (float)item->valuedouble;
    
    // Extract humidity
    item = cJSON_GetObjectItem(content, "humidity");
    if (!item || item->type != cJSON_Number) {
        error->code = 4;
        strncpy(error->field, "humidity", sizeof(error->field) - 1);
        strncpy(error->error, "Missing or invalid humidity", sizeof(error->error) - 1);
        return;
    }
    reading->humidity = (float)item->valuedouble;
    
    error->code = 0;  // Success
}

/* Callback: called when connection is established */
static void on_connect(struct mosquitto *mosq, void *userdata, int rc) {
    if (rc == 0) {
        printf("[subscriber] Connected to %s:%d\n", BROKER, PORT);
        printf("[subscriber] Subscribing to: %s\n\n", TOPIC);

        int sub_rc = mosquitto_subscribe(mosq, NULL, TOPIC, 1);
        if (sub_rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "[subscriber] Subscribe failed: %s\n",
                    mosquitto_strerror(sub_rc));
        }
    } else {
        fprintf(stderr, "[subscriber] Connection failed: %s\n",
                mosquitto_connack_string(rc));
    }
}

/* Callback: called when a message arrives */
static void on_message(struct mosquitto *mosq, void *userdata,
                       const struct mosquitto_message *msg) {
    msg_count++;
    cJSON* content = cJSON_Parse((const char *)msg->payload);
    if (!content) {
        fprintf(stderr, "[subscriber] Failed to parse JSON payload\n");
        return;
    }
    sensor_reading_t reading;
    validation_error_t error;
    getObject(content, &reading, &error);
    printf("[subscriber] Message #%d received:\n", msg_count);
    printf("  Station:     %d\n", reading.station_id);
    printf("  Timestamp:   %s\n", reading.timestamp);
    printf("  Temperature: %.2f°C\n", reading.temperature);
    printf("  Humidity:    %.2f%%\n\n", reading.humidity);
    cJSON_Delete(content);
}


/* Callback: called on disconnect */
static void on_disconnect(struct mosquitto *mosq, void *userdata, int rc) {
    if (rc != 0) {
        printf("[subscriber] Unexpected disconnect (rc=%d) – will reconnect\n", rc);
    } else {
        printf("[subscriber] Disconnected cleanly. Received %d messages total.\n",
               msg_count);
    }
}

int main(void) {
    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new("mqtt-lab-subscriber", true, NULL);
    if (!mosq) {
        fprintf(stderr, "[subscriber] Failed to create mosquitto instance\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);

    int rc = mosquitto_connect(mosq, BROKER, PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[subscriber] Could not connect: %s\n",
                mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    printf("[subscriber] Waiting for messages. Press Ctrl+C to stop.\n\n");

    /* Loop forever – handles reconnects automatically */
    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}