# 1. Variables
EXE      = main
SRCDIR   = src
OBJDIR   = obj
CC       = qcc 
PLATFORM = -Vgcc_ntoaarch64le

# Add -Isrc/include so we can find defs.h and cJSON.h
CFLAGS   = -Wall -Isrc -Isrc/include
LIBS     = -lsocket

# 2. Tell make to look in 'src' for any .c files
VPATH    = $(SRCDIR)

# 3. List your objects
_OBJS    = main.o client.o socket.o data_parser.o
OBJS     = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

# 4. Rules
all: $(OBJDIR) $(EXE)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(EXE): $(OBJS)
	$(CC) $(PLATFORM) -o $@ $^ $(LIBS)

# Simpler rule: look for the .c file using VPATH
$(OBJDIR)/%.o: %.c
	$(CC) $(PLATFORM) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(EXE) $(OBJDIR)

.PHONY: all clean clean