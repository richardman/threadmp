CC = clang++
CFLAGS = -O2 -g

OBJS = threadmp.o 
INCLUDES = threadmp.h status_codes.h

testmp:	$(OBJS) testmp.o 
	$(CC) $(LFLAGS) -o $@ $< testmp.o

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

threadmp.o:	$(INCLUDES)

testmp.o: $(INCLUDES)


