EXEC	= uart_89501
OBJS    = uart_89501.o serial.o

CROSS =
#CC	= $(CROSS)gcc

#STRIP	= $(CROSS)strip
CFLAGS	= -g -O2

all:  clean $(EXEC)

$(EXEC):$(OBJS)
	$(CC) $(CFLAGS)	-o $@ $(OBJS)  -lpthread

clean:
	-rm -f $(EXEC) *.o

