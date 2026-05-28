CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra
LDFLAGS = -lmosquitto

TARGETS = test_publisher test_subscriber publish_message integration

.PHONY: all clean test

all: $(TARGETS)

test_publisher: test_publisher.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test_subscriber: test_subscriber.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

publish_message: publish_message.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

integration: integration.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)

# Quick smoke-test: subscribe for 15 seconds using the CLI tool
test:
	@echo "Listening on mqtt-lab/test/sensor for 15 seconds..."
	mosquitto_sub -h test.mosquitto.org -t "mqtt-lab/test/sensor" -v -W 15 || true
