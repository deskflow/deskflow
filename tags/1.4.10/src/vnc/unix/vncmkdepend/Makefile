SRCS = include.c main.c parse.c pr.c cppsetup.c ifparser.c
OBJS = $(SRCS:.c=.o)
vncmkdepend: $(OBJS)
	$(CC) $^ -o $@
