#include <stdio.h>
#include <stdlib.h>

char* buffer;

void serialize_to_json(const int station_id, const char timestamp[17], const float temperature, const float humidity) {
    if (station_id < 1) return;
    if (timestamp == NULL || timestamp[0] == '\0') return;
    if (temperature < -30.0f || temperature > 50.0f) return;
    if (humidity < 0.0f || humidity > 100.0f) return;

    buffer = malloc(256);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for JSON buffer\n");
        return;
    }

    snprintf(buffer, 256,
             "{\"station_id\": %d, \"timestamp\": \"%s\", \"temperature\": %.2f, \"humidity\": %.2f}",
             station_id, timestamp, temperature, humidity);
}