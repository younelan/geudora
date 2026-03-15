CC = cc
KRB5_PREFIX = $(shell brew --prefix krb5 2>/dev/null)
OPENSSL_CFLAGS = $(shell pkg-config --cflags openssl 2>/dev/null)
OPENSSL_LIBS = $(shell pkg-config --libs openssl 2>/dev/null || echo "-lssl -lcrypto")
CFLAGS = $(shell pkg-config --cflags gtk4 json-glib-1.0 libxml-2.0) -Wall -Iinclude -IgEditCtrl -ICrispinIMAP/Include -DIMAP -DTHREADING_ON -DESSL -pthread \
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
# Exclude platform-specific scripting backends; pick the right one below
SRC_ALL = $(wildcard src/*.c)
SRC_EXCLUDE = src/scripting_ae.c
ifeq ($(shell uname),Darwin)
  # On macOS with GTK4 we use D-Bus (or could switch to AE if building with Carbon)
  SRC_EXCLUDE = src/scripting_ae.c
endif
SRC = $(filter-out $(SRC_EXCLUDE),$(SRC_ALL)) $(RESOURCE_C)
OBJ = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(filter src/%.c,$(SRC))) \
      $(patsubst resources/%.c,$(BUILD_DIR)/%.o,$(filter resources/%.c,$(SRC)))


# geditCtrl library
gedit_LIB = geditCtrl/libgedit.a
gedit_SRC = $(wildcard geditCtrl/*.c)
gedit_OBJ = $(gedit_SRC:.c=.o)

# CrispinIMAP library
crispin_LIB = CrispinIMAP/libc-client.a

all: $(BUILD_DIR) $(RESOURCE_C) $(gedit_LIB) $(crispin_LIB) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


# Compile GResource
$(RESOURCE_C): $(RESOURCE_XML) resources/features.xml resources/strings.json
	glib-compile-resources --target=$@ --sourcedir=resources --generate-source $(RESOURCE_XML)
	glib-compile-resources --target=$(RESOURCE_H) --sourcedir=resources --generate-header $(RESOURCE_XML)
	@echo "✓ Compiled GResource bundle"

# Build geditCtrl library
$(gedit_LIB): $(gedit_OBJ)
	ar rcs $@ $^
	@echo "✓ Built geditCtrl library"

# Build CrispinIMAP library
$(crispin_LIB):
	$(MAKE) -C CrispinIMAP
	@echo "✓ Built CrispinIMAP library"

# Build main application
STUB_OBJS = $(wildcard src/*.o.stub) src/mailbox_stub.o

$(TARGET): $(OBJ) $(gedit_LIB) $(crispin_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(if $(wildcard src/mailbox_stub.o),src/mailbox_stub.o,) $(gedit_LIB) $(crispin_LIB) $(LIBS)
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
	$(MAKE) -C CrispinIMAP clean
	@echo "✓ Cleaned build artifacts"


.PHONY: all clean
