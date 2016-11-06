EXEC	= uart_89501_rx
OBJS    = uart_89501_rx.o serial.o

CROSS =
CFLAGS	= -g -O2

all:  clean $(EXEC)

$(EXEC):$(OBJS)
	$(CC) $(CFLAGS)	-o $@ $(OBJS)  -lpthread

install:
	install svapi_test $(EXEC_DIR)/entry_navi/usr/bin/

clean:
	-rm -f $(EXEC) *.o

