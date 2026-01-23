# Sources for the library
SRCS = imap4r1.c mail.c misc.c netmsg.c rfc822.c smanager.c os_unix.c
OBJS = $(SRCS:.c=.o)
LIB = libc-client.a

CFLAGS = -std=gnu89 -O2 -IInclude -I../include

all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $^

mtest: mtest.o $(LIB)
	$(CC) $(CFLAGS) -o $@ mtest.o $(LIB) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) mtest.o $(LIB) mtest
