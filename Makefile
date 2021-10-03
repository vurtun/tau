BIN = muon
CC = clang
DCC = clang
CFLAGS = -std=c99 -pedantic -DUSE_SIMD_128
CFRAMEWORKS = -framework Cocoa
OBJCFLAGS = $(CFLAGS) -ObjC -fobjc-arc

SYSLIBS = #-L -lpthread
APPLIBS = #-L /usr/X11/lib -L /usr/local/lib -lX11 -lGL -lXext
INCL = #-I /usr/X11/include -I /usr/local/include

SYSSRC = src/sys_mac.m
SYSOBJ = $(SYSSRC:.m=.o)

DBGSRC = src/dbg.c
DBGOBJ = $(DBGSRC:.c=.o)

APPSRC = src/app.c
APPOBJ = $(APPSRC:.c=.o)

RENSRC = src/ren.c
RENOBJ = $(RENSRC:.c=.o)

RESSRC = src/res.c
RESOBJ = $(RESSRC:.c=.o)

GUISRC = src/gui.c
GUIOBJ = $(GUISRC:.c=.o)

.PHONY: clang
clang: CFLAGS += -g -Weverything -Wno-missing-noreturn -Wno-covered-switch-default
clang: CFLAGS += -Wno-padded -Wno-comma -Wno-missing-field-initializers
clang: CFLAGS += -Wno-double-promotion -Wno-float-equal -Wno-switch -Wno-switch-enum
clang: CFLAGS += -Wno-unused-macros -Wno-unused-local-typedef -Wno-format-nonliteral
clang: CFLAGS += -Wc++-compat -Wno-unused-function
clang: CFLAGS += -Wimplicit-int-conversion -Wimplicit-fallthrough
clang: CFLAGS += -Wno-atomic-implicit-seq-cst
clang: OBJCFLAGS = -g -Weverything -Wno-missing-noreturn -Wno-covered-switch-default
clang: OBJCFLAGS += -Wno-padded -Wno-comma -Wno-missing-field-initializers
clang: OBJCFLAGS += -Wno-double-promotion -Wno-float-equal -Wno-switch -Wno-switch-enum
clang: OBJCFLAGS += -Wno-unused-macros -Wno-unused-local-typedef -Wno-format-nonliteral
clang: OBJCFLAGS += -Wno-unused-function -Wimplicit-int-conversion -Wimplicit-fallthrough
clang: OBJCFLAGS += -Wno-atomic-implicit-seq-cst -Wno-deprecated-declarations
clang: CC = clang
clang: $(BIN)

.PHONY: release
release: CFLAGS += -g -Wall -Wextra -O2
release: OBJCFLAGS = -g -Wall -Wextra -O2
release: CC = clang
release: $(BIN)

$(BIN): $(APPOBJ) $(SYSOBJ) $(RENOBJ) $(RESOBJ) $(GUIOBJ) $(DBGOBJ)
	@mkdir -p bin
	rm -f bin/$(BIN) $(APPOBJ) $(SYSOBJ) $(RENOBJ)
	rm -f $(RESOBJ) $(GUIOBJ) $(DBGOBJ)
	$(CC) $(OBJCFLAGS) $(INCL) -o bin/$(BIN) $(SYSSRC) $(CFRAMEWORKS) $(SYSLIBS)
	$(CC) $(CFLAGS) -shared $(INCL) -o bin/dbg.so $(DBGSRC) $(DBGLIBS)
	$(CC) $(CFLAGS) -shared $(INCL) -o bin/ren.so $(RENSRC) $(RENLIBS)
	$(CC) $(CFLAGS) -shared $(INCL) -o bin/app.so $(APPSRC) $(APPLIBS)
	$(CC) $(CFLAGS) -shared $(INCL) -o bin/res.so $(RESSRC) $(RESLIBS)
	$(CC) $(CFLAGS) -shared $(INCL) -o bin/gui.so $(GUISRC) $(GUILIBS)

