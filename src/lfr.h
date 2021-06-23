/****
La Femme Rough - An minimal graph based scripting system for games.
****/
#ifndef LFR_H
#define LFR_H

//// LFR Instructions ////

typedef enum lfr_instruction_ {
	lfr_print_own_id,
	lfr_no_core_instructions // Not an instruction :P
} lfr_instruction_e;

const char* lfr_get_instruction_name(lfr_instruction_e);

//// LFR Node ////

typedef struct lfr_node_id_ { unsigned id; } lfr_node_id_t;

enum {lfr_node_max_flow_slots = 4};
typedef struct lfr_node_ {
	lfr_instruction_e instruction;
	lfr_node_id_t flow_slots[lfr_node_max_flow_slots];
} lfr_node_t;

enum {lfr_node_table_max_rows = 16, lfr_node_table_id_range = 1024};
typedef struct lfr_node_table_ {
	// Meta fields
	unsigned sparse_id[lfr_node_table_id_range];
	lfr_node_id_t dense_id[lfr_node_table_max_rows];
	unsigned num_rows, next_id;

	// Data colums
	lfr_node_t nodes[lfr_node_table_max_rows];
} lfr_node_table_t;

lfr_node_id_t lfr_insert_node_into_table(lfr_instruction_e, lfr_node_table_t*);
int lfr_fprint_node_table(const lfr_node_table_t*, FILE * restrict);


/// LFR Graph ////
typedef struct lfr_graph_ {
	lfr_node_table_t nodes;
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

//// Sparce table macros ////
#define T_HAS_ID(t, r) \
	((t).sparse_id[(r).id] < (t).num_rows && (t).dense_id[(t).sparse_id[(r).id]].id == (r).id)

#define T_INDEX(t,r) \
	(assert(T_HAS_ID((t), (r))), (t).sparse_id[(r).id])

#define T_ID(table, index) \
	(assert( (index) < (table).num_rows), (table).dense_id[index])

#define T_FOR_ROWS(r,t) \
	for (unsigned r = 0; r < (t).num_rows; r++)


//// LFR Graph ////

/**
Initialize an LFR graph.
**/
void lfr_init_graph(lfr_graph_t *graph) {
	graph->nodes.next_id = 1;
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
	return lfr_insert_node_into_table(inst, &graph->nodes);
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
	int char_count = 0;

	// Nodes in graph
	char_count += fprintf(stream, "[[ LFR Nodes ]]\n");
	char_count += lfr_fprint_node_table(&graph->nodes, stream);
	char_count += fprintf(stream, "\n");

	return char_count;
}


//// LFR Node table ////
/**
Insert a new node at the end of the table.
**/
lfr_node_id_t lfr_insert_node_into_table(lfr_instruction_e inst, lfr_node_table_t *table) {
	assert(table->num_rows < lfr_node_table_max_rows);

	// Add row to sparse set
	lfr_node_id_t node_id = {table->next_id++};
	int index = table->num_rows++;
	table->sparse_id[node_id.id] = index;
	table->dense_id[index] = node_id;

	// Set row data
	table->nodes[index].instruction = inst;
	for (int i = 0; i < lfr_node_max_flow_slots; i++) {
		table->nodes[index].flow_slots[i] = (lfr_node_id_t){0};
	}

	return table->dense_id[index];
}


/**
Print node table content onto file stream.
**/
int lfr_fprint_node_table(const lfr_node_table_t *table, FILE * restrict stream) {
	int char_count = 0;

	char_count += fprintf(stream, "|ID\t|LFR Instruction\t|Flow slots\t|\n");
	T_FOR_ROWS(index, *table) {
		char_count += fprintf(stream, "|[#%u|%u]\t|", T_ID(*table,index).id, index);
		char_count += fprintf(stream, "%s\t|", lfr_get_instruction_name(table->nodes[index].instruction));
		char_count += fprintf(stream, "? ? ? ?\t|");
		char_count += fprintf(stream, "\n");
	}

	return char_count;
}


//// LFR Instructions ////

const char* lfr_get_instruction_name(lfr_instruction_e inst) {
	switch(inst) {
	case lfr_print_own_id: { return "lfr_print_own_id";}
	default: { assert(0); return "Unknown LFR Instruction"; }
	}
}

#undef T_HAS_ID
#undef T_INDEX
#undef T_ID
#undef T_FOR_ROWS
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
