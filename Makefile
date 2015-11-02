VERSION=0.0.1

TARGET_CLIENT=statbar
TARGET_DAEMON=statd

SRCDIR = src
OBJDIR = build

CC ?= gcc
CFLAGS += -Wall -Wextra
CFLAGS += -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align
CFLAGS += -Wstrict-prototypes -Wwrite-strings -ftrapv -Wpadded
CFLAGS += -fsanitize=address
#CFLAGS += -march=native
CFLAGS += -DVERSION=\"$(VERSION)\"
SFLAGS = -std=c99 -pedantic -D_XOPEN_SOURCE=600
#_XOPEN_SOURCE >=500 to use ftruncate in C99, >=600 for setenv
INCLUDES = -I.
LIBS = -lrt -lpthread

SRC_C = $(SRCDIR)/client.c $(SRCDIR)/common.c
SRC_D = $(SRCDIR)/daemon.c $(SRCDIR)/common.c
OBJ_C=$(SRC_C:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJ_D=$(SRC_D:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

PREFIX ?= /usr
BINDIR = $(DESTDIR)$(PREFIX)/bin
MANDIR = $(DESTDIR)$(PREFIX)/share/man/man1
BSHDIR = $(DESTDIR)$(PREFIX)/share/bash-completion/completions
ZSHDIR = $(DESTDIR)$(PREFIX)/share/zsh/site-functions


all: CFLAGS += -O2
all: $(TARGET_CLIENT) $(TARGET_DAEMON)

debug: CFLAGS += -O0 -g -DDEBUG
debug: $(TARGET_CLIENT) $(TARGET_DAEMON)


#automatic recompile when makefile changes
$(OBJS): Makefile

#automatically creates build directory if it doesn't exist
dummy := $(shell test -d $(OBJDIR) || mkdir -p $(OBJDIR))


$(TARGET_CLIENT): $(OBJ_C)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJ_C) $(LIBS)

$(TARGET_DAEMON): $(OBJ_D)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJ_D) $(LIBS)


$(OBJ_C): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(SFLAGS) $(INCLUDES) -c -o $@ $< $(LIBS)

$(OBJ_D): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(SFLAGS) $(INCLUDES) -c -o $@ $< $(LIBS)

install:
	install -D -m 755 $(TARGET) "$(BINDIR)/$(TARGET)"
	install -D -m 644 doc/$(TARGET).1 "$(MANDIR)/$(TARGET).1"
	install -D -m 644 extra/$(RULES) "$(UDVDIR)/$(RULES)"
	install -D -m 644 extra/bash_completion "$(BSHDIR)/$(TARGET)"
	#install -D -m 644 extra/zsh_completion "$(ZSHDIR)/_$(TARGET)"

uninstall:
	$(RM) "$(BINDIR)/$(TARGET)"
	$(RM) "$(MANDIR)/$(TARGET).1"
	$(RM) "$(BSHDIR)/$(TARGET)"
	#$(RM) "$(ZSHDIR)/_$(TARGET)"

test:
	$(CC) -o test test/*.c

doc:
	a2x -v -d manpage -f manpage -a revnumber=$(VERSION) doc/$(TARGET).1.txt

clean:
	$(RM) $(OBJDIR)/* $(TARGET_DAEMON) $(TARGET_CLIENT)

.PHONY: all debug clean install uninstall test doc
