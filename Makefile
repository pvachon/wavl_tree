OBJ=wavltree.o wavltree_test.o

DEFINE=-D__WAVL_TEST__ -DDEBUG
OFLAGS=-O0 -ggdb

TARGET=wavl-test

CFLAGS=$(OFLAGS) -Wextra -Wall $(DEFINE) -std=c11
LDFLAGS=

.c.o:
	$(CC) $(CFLAGS) -c $<

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJ)

clean:
	$(RM) $(OBJ) $(TARGET)

.PHONY: all clean

