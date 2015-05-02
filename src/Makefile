CC=gcc
CFLAGS=-I. -lstdc++
DEPS = # header file 
OBJ = serverFork.o

%.o: %.cc $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

serverFork: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)


