CC      = cc
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -Os $(shell pkg-config --cflags xcursor)
LDFLAGS = -lX11 -lXcursor
PREFIX  = /usr/local
SRC     = slide.c
BIN     = slide
all: $(BIN)
$(BIN): $(SRC) slide.h config.h
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)
install: $(BIN)
	install -Dm755 $(BIN) $(PREFIX)/bin/$(BIN)
uninstall:
	rm -f $(PREFIX)/bin/$(BIN)
clean:
	rm -f $(BIN)
.PHONY: all install uninstall clean
