# PCE emulator makefile
# Gilead Kutnick - Exophase

# Global definitions

CC        = gcc
STRIP     = strip

PREFIX    = /usr
OBJS      = main.o cpu.o memory.o irq.o io.o video.o timer.o psg.o disasm.o    \
            screen.o event.o audio.o menu.o debug.o cd.o adpcm.o netplay.o     \
            arcade_card.o bin_cue.o                                            \
            SDL_main.o SDL_screen.o SDL_audio.o SDL_event.o SDL_menu.o crc32.o
BIN       ?= temper 

# Platform specific definitions 

VPATH      += .. ../SDL ../linux
CFLAGS     += -DLINUX_X86_BUILD -O2 -pg -Wall
INCLUDES   = -I${PREFIX}/include `sdl-config --cflags`
LIBS       = -L${PREFIX}/lib `sdl-config --libs` -lz -lm -pthread -pg

# Compilation:

.SUFFIXES: .c .S

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

%.o: %.S
	${AS} -o $@ $<

all:	${OBJS}
	${CC} ${OBJS} ${LIBS} -o ${BIN}  

clean:
	rm -f *.o ${BIN} 

