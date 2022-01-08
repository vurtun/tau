platform=$(shell uname -s)

BIN = tau
CC = clang

# MAC OS
ifeq ($(platform),Darwin)

CFLAGS = -std=c99 -pedantic -DUSE_SIMD_128
OBJCFLAGS = $(CFLAGS) -ObjC -fobjc-arc

.PHONY: debug
debug: CFLAGS += -g -Weverything -Wno-missing-noreturn -Wno-covered-switch-default
debug: CFLAGS += -Wno-padded -Wno-comma -Wno-missing-field-initializers
debug: CFLAGS += -Wno-double-promotion -Wno-float-equal -Wno-switch -Wno-switch-enum
debug: CFLAGS += -Wno-unused-macros -Wno-unused-local-typedef -Wno-format-nonliteral
debug: CFLAGS += -Wc++-compat -Wno-unused-function
debug: CFLAGS += -Wimplicit-int-conversion -Wimplicit-fallthrough
debug: CFLAGS += -Wno-atomic-implicit-seq-cst
debug: CFLAGS += -DDEBUG_MODE
debug: OBJCFLAGS = -g -Weverything -Wno-missing-noreturn -Wno-covered-switch-default
debug: OBJCFLAGS += -Wno-padded -Wno-comma -Wno-missing-field-initializers
debug: OBJCFLAGS += -Wno-double-promotion -Wno-float-equal -Wno-switch -Wno-switch-enum
debug: OBJCFLAGS += -Wno-unused-macros -Wno-unused-local-typedef -Wno-format-nonliteral
debug: OBJCFLAGS += -Wno-unused-function -Wimplicit-int-conversion -Wimplicit-fallthrough
debug: OBJCFLAGS += -Wno-atomic-implicit-seq-cst -Wno-deprecated-declarations
debug: OBJCFLAGS += -DDEBUG_MODE
debug: CC = clang
debug: bin/tau
debug: bin/dbg.so
debug: bin/ren.so
debug: bin/app.so
debug: bin/res.so
debug: bin/gui.so
debug: bin/pck.so
debug: bin/dbs.so

.PHONY: release
release: CFLAGS += -Wall -Wextra -O2 -fwhole-program -flto
release: CFLAGS += -DRELEASE_MODE
release: OBJCFLAGS = -Wall -Wextra -O2 -fwhole-program -flto
release: OBJCFLAGS += -DRELEASE_MODE
release: CC = clang
release: $(BIN)

.PHONY: clean
clean:
	rm bin/tau src/sys/dbg.o src/sys/ren.o src/app.o src/res.o
	rm src/gui.o src/pck.o src/dbs.o
	rm bin/dbg.so bin/ren.so bin/app.so bin/res.so bin/gui.so bin/pck.so bin/dbs.so

bin/tau: src/sys/sys_mac.o
	@mkdir -p bin
	$(CC) $(OBJCFLAGS) -o bin/tau src/sys/sys_mac.m -framework Cocoa

bin/dbg.so: src/sys/dbg.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/dbg.so src/sys/dbg.c

bin/ren.so: src/sys/ren.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/ren.so src/sys/ren.c

bin/app.so: src/app.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/app.so src/app.c

bin/res.so: src/res.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/res.so src/res.c

bin/gui.so: src/gui.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/gui.so src/gui.c

bin/pck.so: src/pck.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/pck.so src/pck.c

bin/dbs.so: src/dbs.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/dbs.so src/dbs.c

$(BIN): src/sys/sys_mac.o src/app.o
	rm -r -f bin
	@mkdir -p bin
	$(CC) $(OBJCFLAGS) -c src/sys/sys_mac.m -o src/sys/sys_mac.o -framework Cocoa
	$(CC) $(CFLAGS) -o bin/$(BIN) src/app.c src/sys/sys_mac.o -framework Cocoa

else # UNIX

CFLAGS = -std=c99 -pedantic -DUSE_SIMD_256 -D_DEFAULT_SOURCE -msse4.1 -mavx2
CFLAGS += -D_POSIX_C_SOURCE=200809L

SYSLIBS = -L /usr/X11/lib -L /usr/local/lib -lX11 -ldl -lXext
SYSINCL = -I /usr/X11/include -I /usr/local/include
SYSSRC = src/sys/sys_x11.c

.PHONY: debug
debug: CFLAGS += -g -Weverything -Wno-missing-noreturn -Wno-covered-switch-default
debug: CFLAGS += -Wno-padded -Wno-comma -Wno-missing-field-initializers
debug: CFLAGS += -Wno-double-promotion -Wno-float-equal -Wno-switch -Wno-switch-enum
debug: CFLAGS += -Wno-unused-macros -Wno-unused-local-typedef -Wno-format-nonliteral
debug: CFLAGS += -Wc++-compat -Wno-unused-function
debug: CFLAGS += -Wimplicit-int-conversion -Wimplicit-fallthrough
debug: CFLAGS += -Wno-atomic-implicit-seq-cst
debug: CC = clang
debug: bin/tau
debug: bin/dbg.so
debug: bin/ren.so
debug: bin/app.so
debug: bin/res.so
debug: bin/gui.so
debug: bin/pck.so
debug: bin/dbs.so
debug: $(BIN)

.PHONY: release
release: CFLAGS += -Wall -Wextra -O2 -fwhole-program -flto -DRELEASE_MODE
release: CC = clang
release: $(BIN)

bin/tau: src/sys/sys_x11.o
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SYSINCL) -o bin/$(BIN) src/sys/sys_x11.c $(SYSLIBS)

bin/dbg.so: src/sys/dbg.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/dbg.so src/sys/dbg.c

bin/ren.so: src/sys/ren.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/ren.so src/sys/ren.c

bin/app.so: src/app.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/app.so src/app.c

bin/res.so: src/res.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/res.so src/res.c

bin/gui.so: src/gui.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/gui.so src/gui.c

bin/pck.so: src/pck.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/pck.so src/pck.c

bin/dbs.so: src/dbs.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -shared -o bin/dbs.so src/dbs.c

$(BIN): $(APPOBJ) $(SYSOBJ) $(RENOBJ) $(RESOBJ) $(GUIOBJ) $(DBGOBJ) $(PCKOBJ) $(DBSOBJ)
	rm -r -f bin
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SYSINCL) -o bin/$(BIN) $(SYSSRC) $(SYSLIBS)
	$(CC) $(CFLAGS) $(DBGINCL) -shared $(INCL) -o bin/dbg.so $(DBGSRC) $(DBGLIBS)
	$(CC) $(CFLAGS) $(RENINCL) -shared $(INCL) -o bin/ren.so $(RENSRC) $(RENLIBS)
	$(CC) $(CFLAGS) $(APPINCL) -shared $(INCL) -o bin/app.so $(APPSRC) $(APPLIBS)
	$(CC) $(CFLAGS) $(RESINCL) -shared $(INCL) -o bin/res.so $(RESSRC) $(RESLIBS)
	$(CC) $(CFLAGS) $(GUIINCL) -shared $(INCL) -o bin/gui.so $(GUISRC) $(GUILIBS)
	$(CC) $(CFLAGS) $(PCKINCL) -shared $(INCL) -o bin/pck.so $(PCKSRC) $(PCKLIBS)
	$(CC) $(CFLAGS) $(DBSINCL) -shared $(INCL) -o bin/dbs.so $(DBSSRC) $(DBSLIBS)

endif

