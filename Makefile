# SPDX-LICENSE-IDENTIFIER: GPL-3.0-or-later

all: label

.PHONY: all clean

CFLAGS ?= -O2 -pipe
CFLAGS += -g -std=c2x -Wall -Wextra -Wpedantic -Wshadow -Werror

PKGCONFIG ?= pkg-config

SOURCES = main.c

LIBS = librsvg-2.0 pangocairo popt

$(foreach LIB,$(LIBS), \
	$(eval CFLAGS += $(shell $(PKGCONFIG) --cflags $(LIB))) \
	$(eval LDLIBS += $(shell $(PKGCONFIG) --libs   $(LIB))))

label: $(SOURCES:.c=.o) $(SOURCES:.c=.d)
	$(LINK.c) -o $@ $(SOURCES:.c=.o) $(LDLIBS)

clean:
	rm -f *.o *.d

distclean: clean
	rm -f *~

%.o: %.c
	$(COMPILE.c) -o $@ $<

%.d: %.c
	$(COMPILE.c) -MM -o $@ $<

-include $(SOURCES:.c=.d)
