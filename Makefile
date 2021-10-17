platform=$(shell uname -s)

BIN = muon
CC = clang
DCC = clang

# MAC OS
ifeq ($(platform),Darwin)

CFLAGS = -std=c99 -pedantic -DUSE_SIMD_128
OBJCFLAGS = $(CFLAGS) -ObjC -fobjc-arc

SYSLIBS = -framework Cocoa
RENLIBS =
RESLIBS =
GUILIBS =
APPLIBS =
PCKLIBS =

SYSINCL =
DBGINCL =
RENINCL =
RESINCL =
GUIINCL =
APPINCL =
PCKINCL =

SYSSRC = src/sys/sys_mac.m
SYSOBJ = $(SYSSRC:.m=.o)

DBGSRC = src/sys/dbg.c
DBGOBJ = $(DBGSRC:.c=.o)

RENSRC = src/sys/ren.c
RENOBJ = $(RENSRC:.c=.o)

RESSRC = src/res.c
RESOBJ = $(RESSRC:.c=.o)

GUISRC = src/gui.c
GUIOBJ = $(GUISRC:.c=.o)

APPSRC = src/app.c
APPOBJ = $(APPSRC:.c=.o)

PCKSRC = src/pck.c
PCKOBJ = $(PCKSRC:.c=.o)

DBSSRC = src/dbs.c
DBSOBJ = $(DBSSRC:.c=.o)

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

$(BIN): $(APPOBJ) $(SYSOBJ) $(RENOBJ) $(RESOBJ) $(GUIOBJ) $(DBGOBJ) $(PCKOBJ) $(DBSOBJ)
	@mkdir -p bin
	rm -f bin/$(BIN) $(APPOBJ) $(SYSOBJ) $(RENOBJ)
	rm -f $(RESOBJ) $(GUIOBJ) $(DBGOBJ) $(PCKOBJ) $(DBSOBJ)
	$(CC) $(OBJCFLAGS) $(SYSINCL) -o bin/$(BIN) $(SYSSRC) $(SYSLIBS)
	$(CC) $(CFLAGS) $(DBGINCL) -shared $(INCL) -o bin/dbg.so $(DBGSRC) $(DBGLIBS)
	$(CC) $(CFLAGS) $(RENINCL) -shared $(INCL) -o bin/ren.so $(RENSRC) $(RENLIBS)
	$(CC) $(CFLAGS) $(APPINCL) -shared $(INCL) -o bin/app.so $(APPSRC) $(APPLIBS)
	$(CC) $(CFLAGS) $(RESINCL) -shared $(INCL) -o bin/res.so $(RESSRC) $(RESLIBS)
	$(CC) $(CFLAGS) $(GUIINCL) -shared $(INCL) -o bin/gui.so $(GUISRC) $(GUILIBS)
	$(CC) $(CFLAGS) $(PCKINCL) -shared $(INCL) -o bin/pck.so $(PCKSRC) $(PCKLIBS)
	$(CC) $(CFLAGS) $(DBSINCL) -shared $(INCL) -o bin/dbs.so $(DBSSRC) $(DBSLIBS)

else # UNIX

CFLAGS = -std=c99 -pedantic -DUSE_SIMD_256 -D_DEFAULT_SOURCE -msse4.1 -mavx2
CFLAGS += -D_POSIX_C_SOURCE=200809L

SYSLIBS = -L /usr/X11/lib -L /usr/local/lib -lX11 -ldl -lXext
RENLIBS =
DBGLIBS =
RESLIBS =
GUILIBS =
APPLIBS =
PCKLIBS =

SYSINCL = -I /usr/X11/include -I /usr/local/include
DBGINCL =
RENINCL =
RESINCL =
GUIINCL =
APPINCL =
PCKINCL =

SYSSRC = src/sys/sys_x11.c
SYSOBJ = $(SYSSRC:.c=.o)

DBGSRC = src/sys/dbg.c
DBGOBJ = $(DBGSRC:.c=.o)

RENSRC = src/sys/ren.c
RENOBJ = $(RENSRC:.c=.o)

RESSRC = src/res.c
RESOBJ = $(RESSRC:.c=.o)

GUISRC = src/gui.c
GUIOBJ = $(GUISRC:.c=.o)

APPSRC = src/app.c
APPOBJ = $(APPSRC:.c=.o)

PCKSRC = src/pck.c
PCKOBJ = $(PCKSRC:.c=.o)

DBSSRC = src/dbs.c
DBSOBJ = $(DBSSRC:.c=.o)

.PHONY: clang
clang: CFLAGS += -g -Weverything -Wno-missing-noreturn -Wno-covered-switch-default
clang: CFLAGS += -Wno-padded -Wno-comma -Wno-missing-field-initializers
clang: CFLAGS += -Wno-double-promotion -Wno-float-equal -Wno-switch -Wno-switch-enum
clang: CFLAGS += -Wno-unused-macros -Wno-unused-local-typedef -Wno-format-nonliteral
clang: CFLAGS += -Wc++-compat -Wno-unused-function
clang: CFLAGS += -Wimplicit-int-conversion -Wimplicit-fallthrough
clang: CFLAGS += -Wno-atomic-implicit-seq-cst
clang: CC = clang
clang: $(BIN)

.PHONY: release
release: CFLAGS += -g -Wall -Wextra -O2
release: CC = clang
release: $(BIN)

$(BIN): $(APPOBJ) $(SYSOBJ) $(RENOBJ) $(RESOBJ) $(GUIOBJ) $(DBGOBJ) $(PCKOBJ) $(DBSOBJ)
	@mkdir -p bin
	rm -f bin/$(BIN) $(APPOBJ) $(SYSOBJ) $(RENOBJ)
	rm -f $(RESOBJ) $(GUIOBJ) $(DBGOBJ) $(PCKOBJ) $(DBSOBJ)
	$(CC) $(CFLAGS) $(SYSINCL) -o bin/$(BIN) $(SYSSRC) $(SYSLIBS)
	$(CC) $(CFLAGS) $(DBGINCL) -shared $(INCL) -o bin/dbg.so $(DBGSRC) $(DBGLIBS)
	$(CC) $(CFLAGS) $(RENINCL) -shared $(INCL) -o bin/ren.so $(RENSRC) $(RENLIBS)
	$(CC) $(CFLAGS) $(APPINCL) -shared $(INCL) -o bin/app.so $(APPSRC) $(APPLIBS)
	$(CC) $(CFLAGS) $(RESINCL) -shared $(INCL) -o bin/res.so $(RESSRC) $(RESLIBS)
	$(CC) $(CFLAGS) $(GUIINCL) -shared $(INCL) -o bin/gui.so $(GUISRC) $(GUILIBS)
	$(CC) $(CFLAGS) $(PCKINCL) -shared $(INCL) -o bin/pck.so $(PCKSRC) $(PCKLIBS)
	$(CC) $(CFLAGS) $(DBSINCL) -shared $(INCL) -o bin/dbs.so $(DBSSRC) $(DBSLIBS)

endif

