# Zeke - User space base makefile
#
# Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Configuration files ##########################################################
# Generic config
include $(ROOT_DIR)/makefiles/buildconf.mk
include $(ROOT_DIR)/makefiles/dist.mk
################################################################################
include $(ROOT_DIR)/makefiles/user_idir.mk
################################################################################
# We use suffixes because it's fun
.SUFFIXES:                      # Delete the default suffixes
.SUFFIXES: .c .bc .o .h .S ._S  # Define our suffix list

CCFLAGS += $(subst $\",,$(configUSER_CCFLAGS))

# Available selections for source code files:
# SRC-    =# C sources
# SRC-n   =#
# SRC-y   =#
# ASRC-   =# Assembly sources
# ASRC-n  =#
# ASRC-y  =#
# (A)SRC- and (A)SRC-n won't be compiled

# Automatically generated list of sources
SRC-y = $(foreach var,$(BIN-y), $($(var)-SRC-y))
ASRC-y = $(foreach var,$(BIN-y), $($(var)-ASRC-y))

# Assembly Obj files
ASOBJS = $(patsubst %.S, %.o, $(ASRC-y))

# C Obj files
BCS = $(patsubst %.c, %.bc, $(SRC-y))
OBJS = $(patsubst %.c, %.o, $(SRC-y))
