/****
La Femme Rough - An minimal graph based scripting system for games.
****/
#ifndef LFR_H
#define LFR_H

typedef struct lfr_node_id_ {
	unsigned id;
} lfr_node_id_t;

typedef enum lfr_instruction_ {
	lfr_print_own_id,
	lfr_no_core_instructions // Not an instruction :P
} lfr_instruction_e;

typedef struct lfr_graph_ {

} lfr_graph_t;

void lfr_init_graph(lfr_graph_t *);
void lfr_term_graph(lfr_graph_t *);
lfr_node_id_t lfr_add_node(lfr_instruction_e, lfr_graph_t *);
void lfr_link_nodes(lfr_node_id_t, unsigned, lfr_node_id_t, lfr_graph_t*);
int lfr_fprint_graph(const lfr_graph_t*, FILE * restrict stream);

#endif


/*	==============
	IMPLEMENTATION
	============== */
#ifdef LFR_IMPLEMENTATION
#undef LFR_IMPLEMENTATION

/**
Initialize an LFR graph.
**/
void lfr_init_graph(lfr_graph_t *graph) {
}


/**
Terminate an LFR graph.
**/
void lfr_term_graph(lfr_graph_t *graph) {
}


/**
Add a node with the given instruction to the graph.
**/
lfr_node_id_t lfr_add_node(lfr_instruction_e inst, lfr_graph_t *graph) {
	return (lfr_node_id_t){ 0 };
}


/**
Link execution of one node to another.
**/
void lfr_link_nodes(lfr_node_id_t from_node, unsigned slot, lfr_node_id_t to_node, lfr_graph_t *graph) {
}


/**
Print graph to file stream.
**/
int lfr_fprint_graph(const lfr_graph_t *graph, FILE * restrict stream) {
	return 0;
}
#endif


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
