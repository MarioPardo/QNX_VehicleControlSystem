# 1. Variables
EXE_MAIN     = main_supervisor
EXE_WATCHDOG = watchdog
EXE_BRAKING  = braking_system

SRCDIR   = src
OBJDIR   = obj
CC       = qcc 
PLATFORM = -Vgcc_ntoaarch64le

# ADDED: -g and -O0 to LDFLAGS to ensure symbols survive the link stage
# ADDED: -Werror-implicit-function-declaration to catch the source of your crashes
CFLAGS   = -Wall -g -O0 -Isrc -Isrc/include -Werror-implicit-function-declaration
LDFLAGS  = $(PLATFORM) -g -O0
LIBS_MAIN = -lsocket 

# 2. Tell make to look in 'src' for any .c files
VPATH    = $(SRCDIR)

# 3. List your objects per executable
_OBJS_MAIN = main.o client.o socket.o data_parser.o
OBJS_MAIN  = $(patsubst %,$(OBJDIR)/%,$(_OBJS_MAIN))

_OBJS_WD   = watchdog.o
OBJS_WD    = $(patsubst %,$(OBJDIR)/%,$(_OBJS_WD))

_OBJS_BRK  = braking_system.o
OBJS_BRK   = $(patsubst %,$(OBJDIR)/%,$(_OBJS_BRK))

# 4. Rules
all: $(OBJDIR) $(EXE_MAIN) $(EXE_WATCHDOG) $(EXE_BRAKING)

$(OBJDIR):
	mkdir -p $(OBJDIR)

# Build Main Supervisor - Use $(LDFLAGS) to preserve symbols
$(EXE_MAIN): $(OBJS_MAIN)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS_MAIN)

# Build Watchdog - Use $(LDFLAGS) to preserve symbols
$(EXE_WATCHDOG): $(OBJS_WD)
	$(CC) $(LDFLAGS) -o $@ $^

# Build Braking System - Use $(LDFLAGS) to preserve symbols
$(EXE_BRAKING): $(OBJS_BRK)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile .c to .o
$(OBJDIR)/%.o: %.c
	$(CC) $(PLATFORM) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(EXE_MAIN) $(EXE_WATCHDOG) $(EXE_BRAKING) $(OBJDIR)

.PHONY: all clean