CC=gcc
CFLAGS=-I. -std=c++0x -lstdc++
DEPS = # header file 
OBJ = serverFork.o

%.o: %.cc $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

serverFork: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)


