CC=mpicc
CFLAGS= -Wall -pthread -g 
LIBS= -lm
OBJS= main22.o

PROGS= main22

all: $(PROGS)

%.o : %.c
	$(CC) $(CFLAGS)  -c $(LIBS)$<

main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)


clean:
	rm -f $(PROGS) *.o *~