CC=mpicc
CFLAGS=-Wall -pthread -g
LIBS=
OBJS=main.o

PROGS= main

all: $(PROGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)


clean:
	rm -f $(PROGS) *.o *~