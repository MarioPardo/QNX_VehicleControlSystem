EXE = main
COBJ = main.o client.o socket.o data_parser.o
CC = gcc -Wall
LIBS = -lpthread -lcjson

all:	$(EXE)

client: $(COBJ)
	$(CC) -o client $(COBJ) $(LIBS)

client.o: client.c defs.h
	$(CC) -c client.c


# connect: $(COBJ)
# 	$(CC) -o connect $(COBJ) $(LIBS)

# connect.o: connect.c defs.h
# 	$(CC) -c connect.c


socket: $(COBJ)
	$(CC) -o socket $(COBJ) $(LIBS)

socket.o: socket.c defs.h
	$(CC) -c socket.c


data_parser: $(COBJ)
	$(CC) -o data_parser $(COBJ) $(LIBS)

data_parser.o: data_parser.c defs.h
	$(CC) -c data_parser.c


main: $(COBJ)
	$(CC) -o main $(COBJ) $(LIBS)

main.o: main.c defs.h
	$(CC) -c main.c



clean:
	rm -f $(EXE) $(COBJ)

clean:
	rm -f $(EXE) $(SOBJ) $(COBJ)

# DO NOT USE , NOT READY YET