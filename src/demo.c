/****
LFR Scripting demo.
****/
#include <assert.h>
#include <stdio.h>

#include "lfr.h"

int main( int argc, char** argv) {
	lfr_graph_t graph = {0};
	lfr_init_graph(&graph);

	// Construct graph
	lfr_node_id_t n1 = lfr_add_node(lfr_print_own_id, &graph);
	lfr_node_id_t n2 = lfr_add_node(lfr_print_own_id, &graph);
	lfr_link_nodes(n1, 0, n2, &graph);
	lfr_fprint_graph(&graph, stdout);
	lfr_term_graph(&graph);
	return 0; 
}


#define LFR_IMPLEMENTATION
#include "lfr.h"


/********************************************************************************
MIT License
===========

Copyright (c) 2021 Jakob Eklund

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
********************************************************************************/
