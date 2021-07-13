# General settings #
# ==================
CFLAGS=-std=c11 -g
GLFLAGS=-lglfw -lglew -framework OpenGL
NKFLAGS=-I../friends/nuklear/
BIN_DIR=../bin/


# Build & run things #
# ================== #
.phony: main run
main: $(BIN_DIR)demo $(BIN_DIR)game tags

run: $(BIN_DIR)game
	cd $(BIN_DIR) && ./game

# Build demo application
$(BIN_DIR)demo: demo_app.c *.h $(BIN_DIR) _nk.o
	$(CC) $(CFLAGS) $<  _nk.o $(GLFLAGS) $(NKFLAGS) -o $@

$(BIN_DIR)game: game_app.c *.h $(BIN_DIR) _nk.o
	$(CC) $(CFLAGS) $< _nk.o $(GLFLAGS) $(NKFLAGS) -o $@

# Compile nuklear implementation separately
_nk.o: impl_nk.c
	$(CC) -c $(CFLAGS) $(NKFLAGS) $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)


# Clean up #
# ======== #

clean: $(BIN_DIR)
	rm _*.o
	rm -rv $(BIN_DIR)*


# Utils #
# ===== #
tags: *.c *.h
	ctags *.c *.h


# MIT License
# ===========
#
# Copyright (c) 2021 Jakob Eklund
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
