platform=$(shell uname -s)

BIN = tau
BINEMC = tau.html
EMCC = emcc
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
debug: CFLAGS += -Wc++-compat -Wno-unused-function -Wsign-conversion
debug: CFLAGS += -Wimplicit-int-conversion -Wimplicit-fallthrough
debug: CFLAGS += -Wno-atomic-implicit-seq-cst -Wno-vla -mfma
debug: CFLAGS += -DDEBUG_MODE
debug: OBJCFLAGS = -g -Weverything -Wno-missing-noreturn -Wno-covered-switch-default
debug: OBJCFLAGS += -Wno-padded -Wno-comma -Wno-missing-field-initializers -mfma
debug: OBJCFLAGS += -Wno-double-promotion -Wno-float-equal -Wno-switch -Wno-switch-enum
debug: OBJCFLAGS += -Wno-unused-macros -Wno-unused-local-typedef -Wno-format-nonliteral
debug: OBJCFLAGS += -Wno-unused-function -Wimplicit-int-conversion -Wimplicit-fallthrough
debug: OBJCFLAGS += -Wno-atomic-implicit-seq-cst -Wno-deprecated-declarations -Wno-vla
debug: OBJCFLAGS += -DDEBUG_MODE
debug: CC = clang
debug: $(BIN)

.PHONY: release
release: CFLAGS += -Wall -Wextra -O2 -fwhole-program -flto -fwrapv -mfma
release: CFLAGS += -DRELEASE_MODE
release: OBJCFLAGS = -Wall -Wextra -O2 -fwhole-program -flto -fwrapv -mfma
release: OBJCFLAGS += -DRELEASE_MODE
release: CC = clang
release: $(BIN)

.PHONY: emsc
emsc: ($BINEMC)

($BINEMC): src/sys/sys_ems.c src/app.c
	$(EMCC) --emrun src/app.c src/sys/sys_ems.c -sFULL_ES2 -o bin/tau.html

.PHONY: clean
clean:
	rm bin/tau src/sys/ren.o src/app.o src/res.o src/sys/sys_mac.o
	rm src/gui.o src/pck.o src/dbs.o
	rm bin/dbg.so bin/ren.so bin/app.so bin/res.so bin/gui.so bin/pck.so bin/dbs.so

$(BIN): src/sys/sys_mac.o src/app.o
	rm -r -f bin
	@mkdir -p bin
	$(CC) $(OBJCFLAGS) -c src/sys/sys_mac.m -o src/sys/sys_mac.o -framework Cocoa -framework Metal -framework MetalKit
	$(CC) $(CFLAGS) -o bin/$(BIN) src/app.c src/sys/sys_mac.o -framework Cocoa -framework Metal -framework MetalKit

else # UNIX

CFLAGS = -std=c99 -pedantic -DUSE_SIMD_256 -D_DEFAULT_SOURCE -msse4.1 -mavx2
CFLAGS += -D_POSIX_C_SOURCE=200809L

SYSLIBS = -L /usr/X11/lib -L /usr/local/lib -lX11 -ldl -lXext
SYSINCL = -I /usr/X11/include -I /usr/local/include

.PHONY: debug
debug: CFLAGS += -g -Weverything -Wno-missing-noreturn -Wno-covered-switch-default
debug: CFLAGS += -Wno-padded -Wno-comma -Wno-missing-field-initializers
debug: CFLAGS += -Wno-double-promotion -Wno-float-equal -Wno-switch -Wno-switch-enum
debug: CFLAGS += -Wno-unused-macros -Wno-unused-local-typedef -Wno-format-nonliteral
debug: CFLAGS += -Wc++-compat -Wno-unused-function
debug: CFLAGS += -Wimplicit-int-conversion -Wimplicit-fallthrough
debug: CFLAGS += -Wno-atomic-implicit-seq-cst
debug: CC = clang
debug: $(BIN)

.PHONY: release
release: CFLAGS += -Wall -Wextra -O2 -fwhole-program -flto -DRELEASE_MODE
release: CC = clang
release: $(BIN)

$(BIN): src/sys/sys_x11.o src/app.o
	rm -r -f bin
	@mkdir -p bin
	$(CC) $(CFLAGS) -c src/sys/sys_x11.m -o src/sys/sys_x11.o $(SYSLIBS) $(SYSINCL)
	$(CC) $(CFLAGS) -o bin/$(BIN) src/app.c src/sys/sys_x11.o $(SYSLIBS) $(SYSINCL)

endif

