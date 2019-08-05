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
#~ CC       = /home/g361165/workspace/BP_trunk-dciwm344-v2_prod-3-4-X_nc-dciwm344-v2/output/bcm_dciwm344_7428b0_thinclient_nc_lxc_flash/host/usr/bin/mipsel-linux-gcc
#~ CC       = /home/g361165/workspace/BP_cosmos-broadband-rev1_nc4k-5-7-X_delivery-altice-rev1/output/m384nc_nc4k_ursr15.3_debug/host/usr/bin/arm-linux-gcc
LINKER   = gcc
#~ LINKER   = /home/g361165/workspace/BP_trunk-dciwm344-v2_prod-3-4-X_nc-dciwm344-v2/output/bcm_dciwm344_7428b0_thinclient_nc_lxc_flash/host/usr/bin/mipsel-linux-gcc
#~ LINKER   = /home/g361165/workspace/BP_cosmos-broadband-rev1_nc4k-5-7-X_delivery-altice-rev1/output/m384nc_nc4k_ursr15.3_debug/host/usr/bin/arm-linux-gcc

# change these to proper directories where each file should be
SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin
INCDIR   = include

# compiling flags here
CFLAGS   = -std=c99 -Wall -I$(INCDIR) -D _GNU_SOURCE
# linking flags here
LFLAGS   = -Wall -I$(SRCDIR) -lm

LDFLAGS  = -lpthread -lsqlite3

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
