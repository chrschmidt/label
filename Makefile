# SPDX-LICENSE-IDENTIFIER: GPL-3.0-or-later

all: label

.PHONY: all clean

CFLAGS ?= -O2 -pipe
CFLAGS += -g -std=c23 -Wall -Wextra -Wpedantic -Wshadow -Werror

PKGCONFIG ?= pkg-config

SOURCES = main.c

LIBS = librsvg-2.0 pangocairo popt

TIDYCHECKS = -*,clang-analyzer-*,-clang-analyzer-cplusplus*,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling

$(foreach LIB,$(LIBS), \
	$(eval LIBCFLAGS += $(shell $(PKGCONFIG) --cflags $(LIB))) \
	$(eval LIBLDLIBS += $(shell $(PKGCONFIG) --libs   $(LIB))))

CFLAGS += $(LIBCFLAGS)
LDLIBS += $(LIBLDLIBS)

label: $(SOURCES:.c=.o) $(SOURCES:.c=.d)
	$(LINK.c) -o $@ $(SOURCES:.c=.o) $(LDLIBS)

tidy: $(SOURCES:.c=.tidy)

clean:
	rm -f *.o *.d *.tidy

distclean: clean
	rm -f *~

%.tidy: %.c
	clang-tidy $< -checks=$(TIDYCHECKS) -- $(LIBCFLAGS)2>&1 | tee $@

%.o: %.c
	$(COMPILE.c) -o $@ $<

%.d: %.c
	$(COMPILE.c) -MM -o $@ $<

-include $(SOURCES:.c=.d)
