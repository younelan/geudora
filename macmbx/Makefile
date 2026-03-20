CC = cc
CRISPY_DIR = ../crispy
OPENSSL_CFLAGS = $(shell pkg-config --cflags openssl 2>/dev/null)
CFLAGS = -Wall -I. -I$(CRISPY_DIR) -std=c99 -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE $(OPENSSL_CFLAGS)
BUILD_DIR = build

LIB_SRC = macmbx.c macmbx_build.c macmbx_compact.c macmbx_ops.c macmbx_store.c \
          macmbx_thread.c macmbx_filter.c macmbx_junk.c macmbx_nicknames.c \
          macmbx_sigstat.c macmbx_vcard.c macmbx_conf.c macmbx_emoji.c \
          macmbx_concentrator.c macmbx_spell.c macmbx_mailer.c
LIB_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(LIB_SRC))

all: $(BUILD_DIR) libmacmbx.a

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

libmacmbx.a: $(LIB_OBJ)
	ar rcs $@ $^
	@echo "✓ Built libmacmbx.a"

clean:
	rm -rf $(BUILD_DIR) libmacmbx.a

.PHONY: all clean
