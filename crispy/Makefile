CC = cc
OPENSSL_CFLAGS = $(shell pkg-config --cflags openssl 2>/dev/null)
OPENSSL_LIBS = $(shell pkg-config --libs openssl 2>/dev/null || echo "-lssl -lcrypto")
CFLAGS = -Wall -I. $(OPENSSL_CFLAGS) -pthread
BUILD_DIR = build

LIB_SRC = crispy_smtp.c crispy_pop3.c crispy_msg.c crispy_headparse.c \
          crispy_base64.c crispy_transport.c crispy_conf.c
LIB_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(LIB_SRC))

all: $(BUILD_DIR) libcrispy.a crispy_demo

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

libcrispy.a: $(LIB_OBJ)
	ar rcs $@ $^
	@echo "✓ Built libcrispy.a"

crispy_demo: $(BUILD_DIR)/crispy_demo.o $(BUILD_DIR)/crispy_conf.o libcrispy.a
	$(CC) $(CFLAGS) -o $@ $(BUILD_DIR)/crispy_demo.o $(BUILD_DIR)/crispy_conf.o libcrispy.a $(OPENSSL_LIBS)
	@echo "✓ Built crispy_demo"

clean:
	rm -rf $(BUILD_DIR) libcrispy.a crispy_demo

.PHONY: all clean
