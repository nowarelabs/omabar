CC      ?= clang
CFLAGS  := -std=c99 -Wall -O3 -ffast-math -fvisibility=hidden -fno-common
OBJCFLAGS := -x objective-c
ODIR    := bin

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

_C_SRC := main.c event_loop.c event.c bar_manager.c bar.c bar_item.c \
          window.c surface.c context.c text.c font.c color.c \
          background.c shadow.c image.c graph.c alias.c slider.c \
          group.c popup.c animation.c mouse.c display.c \
          volume.c power.c custom_events.c ipc.c plugin.c \
          hotload.c private_shim.c workspace.m display_nsscreen.m \
          media.m wifi.m app_windows.m

_OBJ := main.o event_loop.o event.o bar_manager.o bar.o bar_item.o \
        window.o surface.o context.o text.o font.o color.o \
        background.o shadow.o image.o graph.o alias.o slider.o \
        group.o popup.o animation.o mouse.o display.o \
        volume.o power.o custom_events.o ipc.o plugin.o \
        hotload.o private_shim.o app_windows.o workspace.o \
        display_nsscreen.o layer.o media.o wifi.o

ifndef NDEBUG
CFLAGS += -g -O0 -DDEBUG
else
CFLAGS += -DNDEBUG
endif

$(ODIR)/omabar: $(patsubst %.o, $(ODIR)/%.o, $(_OBJ))
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

$(ODIR)/%.o: $(SRC)/%.c | $(ODIR)
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

$(ODIR)/%.o: $(SRC)/%.m | $(ODIR)
	$(CC) $(OBJCFLAGS) $(CFLAGS) -I$(SRC) -c $< -o $@

$(ODIR):
	mkdir -p $(ODIR)

all: $(ODIR)/omabar

debug: all

clean:
	rm -rf $(ODIR)

leak: clean
	CFLAGS="-g -O0 -DDEBUG -fsanitize=address,undefined" $(MAKE)

.PHONY: all clean debug leak