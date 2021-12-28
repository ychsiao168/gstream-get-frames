#===============================================================================
#  Description:
#  Author:
#===============================================================================
#===============================================================================
#  TOOL CHAIN and SHELL TOOL
#===============================================================================
CC = gcc
LD = $(CC)
AR = ar rc
RM = rm -f
#===============================================================================
#  CFLAGS, LDFLAGS
#===============================================================================
OPT= -O2
CFLAGS=-Wall -I$(srcdir) $(OPT) $(shell pkg-config --cflags gstreamer-1.0)
LDFLAGS=-s
LDLIBS=$(shell pkg-config --libs gstreamer-1.0 gstreamer-video-1.0)
#===============================================================================
#  OBJS and Output
#===============================================================================
OUTPUT1 = test
OBJS = \
          test.o
#===============================================================================
#  Compiling rules
#===============================================================================
%.o : %.c
	@echo Compiling $< ...
	@$(CC) $(CFLAGS) -c -o $@ $<
%.o : %.c %.h
	@echo Compiling $< ...
	@$(CC) $(CFLAGS) -c -o $@ $<
#===============================================================================
#  Targets
#===============================================================================
all: all-before $(OUTPUT1) all-after
$(OUTPUT1): $(OBJS)
	@echo Linking $@
	@$(LD) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	@$(RM) $(OBJS) $(OUTPUT1)

all-before:
	@clear

all-after:
	@echo Done @ $$(date)