CC = g++
OPENCV =  `pkg-config --cflags --libs opencv`
PTHREAD = -pthread

CLIENT = client.cpp
SERVER = server.cpp
SRC = src/
BIN = bin/
CLI = client
SER = server

all: server client
  
server: $(SRC)$(SERVER)
	$(CC) $(SRC)$(SERVER) -o $(SER)  $(OPENCV) $(PTHREAD) 
client: $(SRC)$(CLIENT)
	$(CC) $(SRC)$(CLIENT) -o $(CLI)  $(OPENCV) $(PTHREAD)

.PHONY: clean

test: $(SRC)$(SERVER) $(SRC)$(CLIENT)
	$(CC) $(SRC)$(SERVER) -o $(BIN)$(SER)  $(OPENCV) $(PTHREAD) 
	$(CC) $(SRC)$(CLIENT) -o $(BIN)$(CLI)  $(OPENCV) $(PTHREAD)

clean:
	rm $(CLI) $(SER)
