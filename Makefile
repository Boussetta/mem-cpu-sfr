# ------------------------------------------------
# Generic Makefile
#
# Author: wissem.boussetta@outlook.com
# Date  : 2019-07-27
#
# Changelog :
#   2019-07-27 - first version
# ------------------------------------------------

# project name (generate executable with this name)
TARGET   = mem-cpu-sfr

CC       = gcc
LINKER   = gcc

# change these to proper directories where each file should be
SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin
INCDIR   = include

# compiling flags here
CFLAGS   = -std=c99 -Wall -I$(INCDIR)
# linking flags here
LFLAGS   = -Wall -I$(SRCDIR) -lm

LDFLAGS  = -lpthread

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm       = rm -f

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(LINKER) -o $@ $(LFLAGS) $(OBJECTS) $(LDFLAGS)
	@echo "Linking complete!"

OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"
