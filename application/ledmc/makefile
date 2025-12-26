
OUTFILE = ledmc

ROOTDIR = ../..

ifndef CONFIGURATION
CONFIGURATION = Debug
endif

ifeq "$(CONFIGURATION)" "Debug"
LIBDIR = $(ROOTDIR)/lib/Debug
endif

ifeq "$(CONFIGURATION)" "Release"
LIBDIR = $(ROOTDIR)/lib/Release
endif

#OUTDIR = $(ROOTDIR)/bin
OUTDIR = .
OUT = $(OUTDIR)/$(OUTFILE)

MAKEFLAGS += --warn-undefined-variables
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.SUFFIXES:

SOURCES := $(wildcard *.cpp)

OBJDIR = $(ROOTDIR)/obj
OBJS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)

LIBRARYDIR = library
INCLUDEDIR = -I. \
	-I$(ROOTDIR)/$(LIBRARYDIR)

LIBDIRS = -L$(LIBDIR)
LIBSLOCAL = \
	$(LIBDIR)/pi_gpio.a \
	$(LIBDIR)/xml.a \
	$(LIBDIR)/quetzal.a
LIBS = \
	$(LIBSLOCAL) \
	-lpthread \
	-lwiringPi

CPPFLAGS = -std=c++14
CFLAGS = -Wall -g -c -Wl,-export-dynamic
ALL_CFLAGS = $(CPPFLAGS) $(CFLAGS) $(INCLUDEDIR)

LDFLAGS = $(LIBDIRS) $(LIBS)

CC = g++

COMPILE.cpp = $(CC) $(ALL_CFLAGS)

.PHONY: all
all: $(OUT)

$(OUT): $(OBJS) $(LIBSLOCAL)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	sudo chown root $(OUT)
	sudo chmod 4755 $(OUT)

DEPDIR = $(ROOTDIR)/dep
DEPFILEIN = $(OBJDIR)/$(*F)
DEPFILEOUT = $(DEPDIR)/$(*F)
DEPS = $(SOURCES:%.cpp=$(DEPDIR)/%.P)

$(OBJDIR)/%.o : %.cpp
	$(COMPILE.cpp) -MD -o $@ $<
	@cp $(DEPFILEIN).d $(DEPFILEOUT).P; \
        sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
            -e '/^$$/ d' -e 's/$$/ :/' < $(DEPFILEIN).d >> $(DEPFILEOUT).P; \
        rm -f $(DEPFILEIN).d

-include $(DEPS)

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(DEPS): | $(DEPDIR)

$(DEPDIR):
	mkdir -p $(DEPDIR)

$(OUT): | $(OUTDIR)

$(OUTDIR):
	mkdir -p $(OUTDIR)

.PHONY: clean
clean:
	rm -f $(DEPS) $(OBJS) $(OUT)

#.PHONY: install
#install:

.PHONY: run
run:
	./$(OUT)
