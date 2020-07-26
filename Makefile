P = spotlibdump
OBJS = main.o cJSON.o cJSON_helper.o hiredis_helper.c
CFLAGS = -g -Wall -Wstrict-prototypes
LDLIBS = -lb -lcurl -lhiredis

$(P): $(OBJS)
	$(CC) -o $(P) $(LDFLAGS) $(OBJS) $(LDLIBS)

clean:
	rm -f *o; rm -f $(P)
