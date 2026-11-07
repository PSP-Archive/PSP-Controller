TARGET = PSPController
APPOBJS= main.o nlh.o loadutil.o stubs.o

OBJS = $(APPOBJS)

INCDIR = 
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS) -c

LIBDIR =
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSPController 0.1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

PSP_MOUNT = /cygdrive/l

kx-install: kxploit
ifeq ($(PSP_MOUNT),)
	@echo '*** Error: $$(PSP_MOUNT) undefined. Please set it to for example /cygdrive/e'
	@echo if your PSP is mounted to E: in cygwin.
else
	@echo copying to $(KXDIR) and $(KXDUMMY)...
	cp -r $(KXDIR) $(PSP_MOUNT)/PSP/GAME/
	cp -r $(KXDUMMY) $(PSP_MOUNT)/PSP/GAME/
	@echo copying data...
	cp -r settings.txt $(PSP_MOUNT)/PSP/GAME/$(KXDIR)
endif