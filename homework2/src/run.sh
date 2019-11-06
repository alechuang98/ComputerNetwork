g++ -o ../ser/server server.cpp `pkg-config --cflags --libs opencv`
g++ -o ../cli/client client.cpp `pkg-config --cflags --libs opencv`
