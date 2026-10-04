platform = $(shell uname -s)

BIN    = tau
BINEMC = tau.html
EMCC   = emcc
CC     = clang
DXC   ?= dxc

# Strict Safety & Real-Time Warning Flags
WARN_FLAGS  = -Weverything -Wno-covered-switch-default -Wno-padded
WARN_FLAGS += -Wno-declaration-after-statement -Wno-double-promotion
WARN_FLAGS += -Wno-float-equal -Wno-switch-default -Wno-unused-macros
WARN_FLAGS += -Wno-unused-local-typedef -Wc++-compat -Wno-unused-function
WARN_FLAGS += -Wsign-conversion -Wimplicit-int-conversion -Wimplicit-fallthrough
WARN_FLAGS += -Wno-unsafe-buffer-usage -Wno-c11-extensions -Wno-pre-c11-compat
WARN_FLAGS += -Wno-deprecated-declarations -Wno-reserved-macro-identifier
WARN_FLAGS += -Wno-reserved-identifier

# ============================================================================
#                                MAC OS (Darwin)
# ============================================================================
ifeq ($(platform),Darwin)

CFLAGS    = -std=c11 -pedantic -DUSE_SIMD_128 -Isrc -Ibin
OBJCFLAGS = $(CFLAGS) -ObjC -fobjc-arc

.PHONY: debug
debug: CFLAGS += -g $(WARN_FLAGS) -DDEBUG_MODE -DDEBUG -DSQLITE_DEBUG
debug: CFLAGS += -fsanitize=address,undefined,leak,integer
debug: CFLAGS += -Wno-nullability-completeness -Wno-nullability-extension -Wno-direct-ivar-access
debug: OBJCFLAGS = -g $(WARN_FLAGS) -std=c99 -pedantic
debug: OBJCFLAGS += -DDEBUG_MODE -DDEBUG -DSQLITE_DEBUG
debug: OBJCFLAGS += -Wno-nullability-completeness -Wno-nullability-extension
debug: OBJCFLAGS += -Wno-direct-ivar-access -Wno-atomic-implicit-seq-cst -Wno-comma
debug: CC = clang
debug: $(BIN)

.PHONY: release
release: CFLAGS += -Wall -Wextra -O2 -s -flto -fwrapv -Wno-unused-function
release: CFLAGS += -DRELEASE_MODE -fstack-protector-all -Werror -std=c11
release: OBJCFLAGS = -Wall -Wextra -O2 -s -flto -fwrapv -Werror -Wno-unused-function
release: OBJCFLAGS += -DRELEASE_MODE -std=c11 -pedantic -fstack-protector-all
release: CC = clang
release: $(BIN)

# Metal Compilation & C Header Generation
src/sys/mac/gfx_mtl.h: src/sys/mac/gfx.metal
	@mkdir -p bin
	xcrun -sdk macosx metal -c $< -o bin/gfx.air
	xcrun -sdk macosx metallib bin/gfx.air -o bin/gfx.metallib
	@echo "/* Auto-generated from gfx.metal - DO NOT EDIT */" > $@
	@(cd bin && xxd -i gfx.metallib) | sed -e 's/^unsigned char/static const unsigned char/' -e '/^unsigned int/d' >> $@
	@rm -f bin/gfx.air bin/gfx.metallib

src/sys/mac/sys.o: src/sys/mac/sys.m src/sys/mac/gfx_mtl.h
	$(CC) $(OBJCFLAGS) -c $< -o $@

$(BIN): src/sys/mac/sys.o src/app.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/$(BIN) src/app.c src/sys/mac/sys.o -framework Cocoa -framework Metal -framework MetalKit

# ============================================================================
#                                UNIX / Linux
# ============================================================================
else

CFLAGS  = -std=c11 -pedantic -DUSE_SIMD_128 -D_GNU_SOURCE -DGFX_EMBED_SPV
CFLAGS += -Isrc -Ibin
LDLIBS  = -lX11 -lX11-xcb -lvulkan -lm -lpthread -ldl

.PHONY: debug
debug: CFLAGS += -g $(WARN_FLAGS)
debug: CFLAGS += -fsanitize=address,undefined,leak,integer
debug: CFLAGS += -DDEBUG_MODE -DDEBUG -DSQLITE_DEBUG
debug: CC = clang
debug: $(BIN)

.PHONY: release
release: CFLAGS += -Wall -Wextra -O2 -s -flto -fwrapv -Wno-unused-function
release: CFLAGS += -DRELEASE_MODE -fstack-protector-all -Werror -std=c11
release: CC = clang
release: $(BIN)

# Shader Compilation & C Header Generation
src/sys/x11/gfx_spv.h: src/sys/x11/gfx.hlsl
	@mkdir -p bin
	$(DXC) -spirv -T vs_6_0 -E ren_vtx $< -Fo bin/ren_vtx.spv
	$(DXC) -spirv -T ps_6_0 -E ren_frag $< -Fo bin/ren_frag.spv
	@echo "/* Auto-generated from gfx.hlsl - DO NOT EDIT */" > $@
	@echo "#include <stdint.h>" >> $@
	@(cd bin && xxd -i ren_vtx.spv)  | sed -e 's/^unsigned char/static const _Alignas(4) unsigned char/' -e '/^unsigned int/d' >> $@
	@(cd bin && xxd -i ren_frag.spv) | sed -e 's/^unsigned char/static const _Alignas(4) unsigned char/' -e '/^unsigned int/d' >> $@
	@rm -f bin/ren_vtx.spv bin/ren_frag.spv

src/sys/x11/sys.o: src/sys/x11/sys.c src/sys/x11/gfx_spv.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): src/sys/x11/sys.o src/app.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/$(BIN) src/app.c src/sys/x11/sys.o $(LDLIBS)

endif

.PHONY: clean
clean:
	rm -rf bin src/sys/x11/*.o src/sys/mac/*.o src/*.o
