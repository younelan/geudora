CC = cc
KRB5_PREFIX = $(shell brew --prefix krb5 2>/dev/null)
OPENSSL_CFLAGS = $(shell pkg-config --cflags openssl 2>/dev/null)
OPENSSL_LIBS = $(shell pkg-config --libs openssl 2>/dev/null || echo "-lssl -lcrypto")
CFLAGS = $(shell pkg-config --cflags gtk4 json-glib-1.0 libxml-2.0) -Wall -Iinclude -Icrispy -IgEditCtrl -Imacmbx -DTHREADING_ON -DESSL -pthread \
         -I$(KRB5_PREFIX)/include $(OPENSSL_CFLAGS)
LIBS = $(shell pkg-config --libs gtk4 json-glib-1.0 libxml-2.0) -pthread \
       $(shell pkg-config --libs libcurl 2>/dev/null || echo -lcurl) \
       -L$(KRB5_PREFIX)/lib -lgssapi_krb5 -lkrb5 -lk5crypto \
       -lresolv $(OPENSSL_LIBS) \
       $(shell uname | grep -q Darwin && echo "-framework Security -framework CoreFoundation")
TARGET = geudora
BUILD_DIR = build


# Resource compilation
RESOURCE_XML = resources/eudora.gresource.xml
RESOURCE_C = resources/eudora_resources.c
RESOURCE_H = resources/eudora_resources.h

# Source files in src/ directory
SRC_ALL = $(wildcard src/*.c)
# Exclude: platform scripting + TCP/SSL (now in CrispinIMAP)
# Excluded: legacy network (crispy), legacy IMAP (crispy_imap), legacy parsing
# (crispy_rfc822/lex822/richtext), legacy filters/junk/compact (macmbx)
SRC_EXCLUDE = src/scripting_ae.c src/tcp.c src/ssl.c src/TransStream.c \
              src/pop.c src/sendmail.c src/uudecode.c src/binhex.c \
              src/rich.c src/peteglue.c \
              src/imapnetlib.c src/imapdownload.c src/imapmailboxes.c \
              src/imapconnections.c src/imapauth.c \
              src/compact.c src/junk.c src/filtrun.c \
              src/trans.c src/lex822.c src/header.c src/mime.c \
              src/hexbin.c src/toc.c src/buildtoc.c src/filters.c \
              src/sasl.c src/scripting_dbus.c \
              src/vcard.c src/FiltDefs.c src/sort.c
ifeq ($(shell uname),Darwin)
  SRC_EXCLUDE += src/scripting_ae.c
endif
SRC = $(filter-out $(SRC_EXCLUDE),$(SRC_ALL)) $(RESOURCE_C)
OBJ = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(filter src/%.c,$(SRC))) \
      $(patsubst resources/%.c,$(BUILD_DIR)/%.o,$(filter resources/%.c,$(SRC)))

# Sub-libraries
gedit_LIB = geditCtrl/libgedit.a
gedit_SRC = $(wildcard geditCtrl/*.c)
gedit_OBJ = $(gedit_SRC:.c=.o)

crispy_LIB = crispy/libcrispy.a
macmbx_LIB = macmbx/libmacmbx.a

all: $(BUILD_DIR) $(RESOURCE_C) $(crispy_LIB) $(macmbx_LIB) $(gedit_LIB) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile GResource
$(RESOURCE_C): $(RESOURCE_XML) resources/features.xml resources/strings.json resources/mime_maps.json
	glib-compile-resources --target=$@ --sourcedir=resources --generate-source $(RESOURCE_XML)
	glib-compile-resources --target=$(RESOURCE_H) --sourcedir=resources --generate-header $(RESOURCE_XML)
	@echo "✓ Compiled GResource bundle"

# Build geditCtrl library
$(gedit_LIB): $(gedit_OBJ)
	ar rcs $@ $^
	@echo "✓ Built geditCtrl library"

# Build crispy (delegates to crispy/Makefile)
$(crispy_LIB):
	$(MAKE) -C crispy libcrispy.a
	@echo "✓ Built crispy library"

# Build macmbx (delegates to macmbx/Makefile)
$(macmbx_LIB): $(crispy_LIB)
	$(MAKE) -C macmbx libmacmbx.a
	@echo "✓ Built macmbx library"

# CrispinIMAP removed — IMAP handled by crispy_imap + macmbx

# Build main application
$(TARGET): $(OBJ) $(crispy_LIB) $(macmbx_LIB) $(gedit_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(macmbx_LIB) $(crispy_LIB) $(gedit_LIB) $(LIBS)
	@echo "✓ Built $(TARGET)"

# Compile source files from src/
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile source files from resources/
$(BUILD_DIR)/%.o: resources/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile geditCtrl files
geditCtrl/%.o: geditCtrl/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(gedit_OBJ) $(gedit_LIB) $(RESOURCE_C) $(RESOURCE_H)
	rm -rf $(BUILD_DIR)
	$(MAKE) -C crispy clean
	$(MAKE) -C CrispinIMAP clean
	@echo "✓ Cleaned build artifacts"

.PHONY: all clean
