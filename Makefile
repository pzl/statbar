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
SFLAGS = -std=c99 -pedantic -D_XOPEN_SOURCE=700
#_XOPEN_SOURCE >=500 to use ftruncate in C99, >=600 for setenv, >= 700 for strndup
INCLUDES = -I.
LIBS = -lrt -lpthread

SRC_C = $(SRCDIR)/client.c $(SRCDIR)/common.c
SRC_D = $(SRCDIR)/daemon.c $(SRCDIR)/common.c
OBJ_C=$(SRC_C:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJ_D=$(SRC_D:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

PREFIX ?= /usr
BINDIR = $(DESTDIR)$(PREFIX)/bin
SHRDIR = $(DESTDIR)$(PREFIX)/share/statbar/
MANDIR = $(DESTDIR)$(PREFIX)/share/man/man1


all: CFLAGS += -O2
all: $(TARGET_CLIENT) $(TARGET_DAEMON)

debug: CFLAGS += -O0 -g -DDEBUG
debug: $(TARGET_CLIENT) $(TARGET_DAEMON)


#automatic recompile when makefile changes
$(OBJ_C): Makefile $(SRCDIR)/icons-in-terminal.h

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
	install -Dm 755 $(TARGET_CLIENT) "$(BINDIR)/$(TARGET_CLIENT)"
	install -Dm 755 $(TARGET_DAEMON) "$(BINDIR)/$(TARGET_DAEMON)"
	#install -D -m 644 doc/$(TARGET).1 "$(MANDIR)/$(TARGET).1"
	install -d "$(SHRDIR)"
	cp -r extra "$(SHRDIR)"
	cp -r modules "$(SHRDIR)"

uninstall:
	$(RM) "$(BINDIR)/$(TARGET_CLIENT)"
	$(RM) "$(BINDIR)/$(TARGET_DAEMON)"
	#$(RM) "$(MANDIR)/$(TARGET).1"
	$(RM) -r "$(SHRDIR)"

doc:
	a2x -v -d manpage -f manpage -a revnumber=$(VERSION) doc/$(TARGET).1.txt

clean:
	$(RM) $(OBJDIR)/* $(TARGET_DAEMON) $(TARGET_CLIENT)

.PHONY: all debug clean install uninstall doc
