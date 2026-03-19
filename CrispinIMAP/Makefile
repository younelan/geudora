# Sources for the library
SRCS = imap4r1.c mail.c misc.c netmsg.c rfc822.c smanager.c os_unix.c imap_callbacks.c TransStream.c tcp.c ssl.c pop.c sendmail.c uudecode.c binhex.c
OBJS = $(SRCS:.c=.o)
LIB = libc-client.a

# Build standalone — only need our own headers + minimal system headers
# ../include still needed for now (MAILSTREAM has Eudora fields), will be removed later
OPENSSL_CFLAGS = $(shell pkg-config --cflags openssl 2>/dev/null)
CFLAGS = -std=gnu89 -O2 -IInclude -I../include -I../gEditCtrl -I../crispy -DIMAP -DESSL $(OPENSSL_CFLAGS) $(shell pkg-config --cflags gtk4)

all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $^

mtest: mtest.o $(LIB)
	$(CC) $(CFLAGS) -o $@ mtest.o $(LIB) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) mtest.o $(LIB) mtest
