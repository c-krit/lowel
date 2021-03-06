#
# Copyright (c) 2021 jdeokkim
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

.PHONY: all clean

PROJ_PATH := lowel

LIB_PATH := $(PROJ_PATH)/lib
SRC_PATH := $(PROJ_PATH)/src

INPUT_OBJ := $(SRC_PATH)/json.o $(SRC_PATH)/lowel.o
OUTPUT_LIB := $(LIB_PATH)/lib$(PROJ_PATH).a

CC := gcc
AR := ar
CFLAGS := -c -D_DEFAULT_SOURCE -g -std=c99 -O2

$(SRC_PATH)/%.o: $(SRC_PATH)/%.c
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

$(OUTPUT_LIB): $(INPUT_OBJ)
	mkdir -p $(LIB_PATH)
	$(AR) rcs $(OUTPUT_LIB) $(INPUT_OBJ)

clean:
	rm -rf $(OUTPUT_LIB)
	rm -rf $(SRC_PATH)/*.o
