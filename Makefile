
CC = gcc

HEADERS = \
	absthr.h \
	ath.h \
	audio_read.h \
	availbits.h \
	bitstream.h \
	common.h \
	crc.h \
	critband.h \
	encode.h \
	encode_new.h \
	encoder.h \
	enwindow.h \
	fft.h \
	freqtable.h \
	ieeefloat.h \
	mem.h \
	musicin.h \
	options.h \
	portableio.h \
	psycho_0.h \
	psycho_1.h \
	psycho_1_priv.h \
	psycho_2.h \
	psycho_3.h \
	psycho_3priv.h \
	psycho_4.h \
	psycho_n1.h \
	subband.h \
	tables.h \
	toolame.h \
	utils.h \
	xpad.h \
	zmqoutput.h \
	vlc_input.h

c_sources = \
	common.c \
	encode.c \
	ieeefloat.c \
	toolame.c \
	portableio.c \
	psycho_n1.c \
	psycho_0.c \
	psycho_1.c \
	psycho_2.c \
	psycho_3.c \
	psycho_4.c \
	fft.c \
	subband.c \
	audio_read.c \
	bitstream.c \
	mem.c \
	crc.c \
	tables.c \
	availbits.c \
	ath.c \
	encode_new.c \
	zmqoutput.c \
	utils.c \
	xpad.c \
	vlc_input.c

OBJ = $(c_sources:.c=.o)

GIT_VER = -DGIT_VERSION="\"`sh git-version.sh`\""

#Uncomment this if you want to do some profiling/debugging
#PG = -g -pg
PG = -g -fomit-frame-pointer

# Optimize flag.
OPTIM = -O2

# These flags are pretty much mandatory
REQUIRED = -DINLINE= ${GIT_VER}

#pick your architecture
ARCH = -march=native
#Possible x86 architectures
#gcc3.2 => i386, i486, i586, i686, pentium, pentium-mmx
#          pentiumpro, pentium2, pentium3, pentium4, k6, k6-2, k6-3,
#          athlon, athlon-tbird, athlon-4, athlon-xp and athlon-mp.

#TWEAK the hell out of the compile. Some of these are real dodgy
# and will cause program instability
#TWEAKS = -finline-functions -fexpensive-optimizations -ffast-math \
#	-malign-double \
#	-mfancy-math-387 -funroll-loops -funroll-all-loops -pipe \
#	-fschedule-insns2 -fno-strength-reduce

#Set a stack of warnings to overcome my atrocious coding style . MFC.
WARNINGS = -Wall
WARNINGS2 = -Wstrict-prototypes -Wmissing-prototypes -Wunused -Wunused-function -Wunused-label -Wunused-parameter -Wunused-variable -Wunused-value -Wredundant-decls

NEW_02L_FIXES = -DNEWENCODE

CC_SWITCHES = $(OPTIM) $(REQUIRED) $(ARCH) $(PG) $(TWEAKS) $(WARNINGS) $(NEW_02L_FIXES)

PGM = toolame

LIBS =  -lm -lzmq -ljack -lpthread -lvlc

#nick burch's OS/2 fix  gagravarr@SoftHome.net
UNAME = $(shell uname)
ifeq ($(UNAME),OS/2)
   SHELL=sh     
   PGM = toolame.exe
   PG = -Zcrtdll -Zexe
   LIBS =
endif

%.o: %.c $(HEADERS) Makefile
	$(CC) $(CC_SWITCHES) -c $< -o $@

$(PGM):	$(OBJ) $(HEADERS) Makefile
	$(CC) $(PG) -o $(PGM) $(OBJ) $(LIBS)

clean:
	-rm $(OBJ) $(DEP) $(PGM)

megaclean:
	-rm $(OBJ) $(DEP) $(PGM) \#*\# *~

distclean:
	-rm $(OBJ) $(DEP) $(PGM) \#* *~ gmon.out gprof* core *shit* *.wav *.mp2 *.c.* *.mp2.* *.da *.h.* *.d *.mp3 *.pcm *.wav logfile

tags: TAGS

TAGS: ${c_sources}
	etags -T ${c_sources}

