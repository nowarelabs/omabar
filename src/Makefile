CC      ?= clang
ODIR    := bin

CFLAGS  := -std=c99 -Wall -Wextra -Wno-unused-parameter \
           -ffast-math -fvisibility=hidden -fno-common \
           -D_DARWIN_C_SOURCE
OBJCFLAGS := -x objective-c

LIBS := -framework Carbon \
        -framework AppKit \
        -framework QuartzCore \
        -framework CoreAudio \
        -framework CoreWLAN \
        -framework CoreVideo \
        -framework IOKit \
        -framework CoreText \
        -framework ImageIO \
        -framework CoreServices \
        -F/System/Library/PrivateFrameworks \
        -framework SkyLight \
        -framework DisplayServices \
        -framework MediaRemote

SRC := .

_C_SRC := main.c config.c event_loop.c event.c bar_manager.c bar.c bar_item.c \
          window.c surface.c context.c text.c font.c color.c \
          background.c shadow.c image.c graph.c alias.c slider.c \
          group.c popup.c animation.c mouse.c display.c \
          volume.c power.c custom_events.c ipc.c ipc_client.c plugin.c \
          hotload.c private_shim.c workspace.m display_nsscreen.m \
          media.m wifi.m app_windows.m mach.c dnd.c

_OBJ := main.o config.o event_loop.o event.o bar_manager.o bar.o bar_item.o \
        window.o surface.o context.o text.o font.o color.o \
        background.o shadow.o image.o graph.o alias.o slider.o \
        group.o popup.o animation.o mouse.o display.o \
        volume.o power.o custom_events.o ipc.o ipc_client.o plugin.o \
        hotload.o private_shim.o app_windows.o workspace.o \
        display_nsscreen.o layer.o media.o wifi.o mach.o dnd.o

ifeq ($(NDEBUG),1)
CFLAGS += -DNDEBUG -O3
else
CFLAGS += -DDEBUG -g -O0
endif

# $(SANITIZE) is empty by default; asan/leak pass
# "-fsanitize=address,undefined" through it so the flags reach both
# the compile steps and the final link.
SANITIZE :=

$(ODIR)/omabar: $(patsubst %.o, $(ODIR)/%.o, $(_OBJ))
	$(CC) $(CFLAGS) $(SANITIZE) $^ -o $@ $(LIBS)

$(ODIR)/%.o: $(SRC)/%.c $(wildcard $(SRC)/*.h $(SRC)/misc/*.h) | $(ODIR)
	$(CC) $(CFLAGS) $(SANITIZE) -I$(SRC) -c $< -o $@

$(ODIR)/%.o: $(SRC)/%.m $(wildcard $(SRC)/*.h $(SRC)/misc/*.h) | $(ODIR)
	$(CC) $(OBJCFLAGS) $(CFLAGS) $(SANITIZE) -I$(SRC) -c $< -o $@

$(ODIR):
	mkdir -p $(ODIR)

all: $(ODIR)/omabar

debug: all

release: clean
	$(MAKE) NDEBUG=1 all

# AddressSanitizer + UBSan build — instrumented objects and link.
# Run the resulting binary with ASAN_OPTIONS=detect_leaks=1 to also
# surface leaks.
asan: clean
	$(MAKE) SANITIZE="-fsanitize=address,undefined" all

# Same instrumentation; convenience alias kept identical to `asan`.
leak: clean
	$(MAKE) SANITIZE="-fsanitize=address,undefined" all

# O3 + frame pointers, no sanitizer — useful for profiling with
# Instruments/spindump.
perf: clean
	$(MAKE) CFLAGS="$(CFLAGS) -O3 -fno-omit-frame-pointer" all

# Best-effort universal binary. Private frameworks (SkyLight,
# DisplayServices, MediaRemote) are not always fat; on Apple Silicon
# the x86_64 slice may fail to link.
universal: clean
	$(MAKE) CFLAGS="$(CFLAGS) -arch arm64 -arch x86_64" all

clean:
	rm -rf $(ODIR)

.PHONY: all debug release asan leak perf universal clean