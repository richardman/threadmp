CC = clang++
CFLAGS = -O2 -g

OBJS = threadmp.o 

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

testmp:	$(OBJS) testmp.o 
	$(CC) $(LFLAGS) -o $@ $< testmp.o

threadmp.o: threadmp.cpp
	$(CC) $(CFLAGS) -c $< -o $@



