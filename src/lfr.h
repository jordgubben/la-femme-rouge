/****
La Femme Rough - An minimal graph based scripting system for games.
****/
#ifndef LFR_H
#define LFR_H

//// LFR Base types ////
typedef struct lfr_vec2_ { float x,y; } lfr_vec2_t;

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
} lfr_node_t;

enum {lfr_node_table_max_rows = 16, lfr_node_table_id_range = 1024};
typedef struct lfr_node_table_ {
	// Meta fields
	unsigned sparse_id[lfr_node_table_id_range];
	lfr_node_id_t dense_id[lfr_node_table_max_rows];
	unsigned num_rows, next_id;

	// Data colums
	lfr_node_t node[lfr_node_table_max_rows];
	lfr_vec2_t position[lfr_node_table_max_rows];
} lfr_node_table_t;

lfr_node_id_t lfr_insert_node_into_table(lfr_instruction_e, lfr_node_table_t*);
unsigned lfr_get_node_index(lfr_node_id_t, const lfr_node_table_t *);
lfr_vec2_t lfr_get_node_position(lfr_node_id_t, const lfr_node_table_t *);
void lfr_set_node_position(lfr_node_id_t, lfr_vec2_t, lfr_node_table_t *);
int lfr_fprint_node_table(const lfr_node_table_t*, FILE * restrict);


/// LFR Graph ////
typedef struct lfr_flow_link_ {
	lfr_node_id_t source_node, target_node;
	unsigned slot;
} lfr_flow_link_t;

enum {lfr_graph_max_flow_links = 32};
typedef struct lfr_graph_ {
	// Nodes
	lfr_node_table_t nodes;
	lfr_vec2_t next_node_pos;

	// Flow links
	lfr_flow_link_t flow_links[lfr_graph_max_flow_links];
	unsigned num_flow_links;
} lfr_graph_t;

void lfr_init_graph(lfr_graph_t *);
void lfr_term_graph(lfr_graph_t *);
lfr_node_id_t lfr_add_node(lfr_instruction_e, lfr_graph_t *);
void lfr_link_nodes(lfr_node_id_t, unsigned, lfr_node_id_t, lfr_graph_t*);
void lfr_unlink_nodes(lfr_node_id_t, unsigned, lfr_node_id_t, lfr_graph_t*);
int lfr_fprint_graph(const lfr_graph_t*, FILE * restrict stream);


//// LFR script execution ////
enum { lfr_toil_max_queue  = 8};
typedef struct lfr_toil_ {
	lfr_node_id_t schedueled_nodes[lfr_toil_max_queue];
	unsigned num_schedueled_nodes;
} lfr_toil_t;

void lfr_schedule(lfr_node_id_t, const lfr_graph_t *, lfr_toil_t *);
int lfr_step(const lfr_graph_t *, lfr_toil_t *);

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

/* Compare two ids. True if they are the same. */
#define T_SAME_ID(a,b) \
	((a).id == (b).id)

/* Add a new row to the sparse table, returning the index of the row.*/
#define T_INSERT_ROW(t, id_type) \
	((t).dense_id[(t).num_rows] = (id_type){(t).next_id}, (t).sparse_id[(t).next_id++] = (t).num_rows++)

#define T_FOR_ROWS(r,t) \
	for (unsigned r = 0; r < (t).num_rows; r++)


//// LFR script execution ////

/**
Enqueue a node to process to the script executions todo-list (as long as there is space left in the queue).
**/
void lfr_schedule(lfr_node_id_t node_id, const lfr_graph_t *graph, lfr_toil_t *toil) {
	assert(T_HAS_ID(graph->nodes, node_id));
	if (toil->num_schedueled_nodes >= lfr_toil_max_queue) {
		fprintf(stderr, "%s(): Node queue is full!\n", __func__);
		return;
	}
	toil->schedueled_nodes[toil->num_schedueled_nodes++] = node_id;
}


/**
Execute topmost scheduled node (if any) from the script executions todo-list.

Returns number of scheduled nodes, including the one executed.
**/
int lfr_step(const lfr_graph_t *graph, lfr_toil_t *toil) {
	if (!toil->num_schedueled_nodes) { return 0; };

	// Find the right node
	const lfr_node_id_t node_id = toil->schedueled_nodes[0];
	const unsigned node_index = T_INDEX(graph->nodes, node_id);
	const lfr_node_t *head = &graph->nodes.node[node_index];

	// Process instruction
	switch (head->instruction) {
	case lfr_print_own_id: { printf("Node ID: [#%u|%u]\n", node_id.id, node_index); } break;
	default: { assert(0); } break;
	};

	// Enqueue nodes - Continue flow throgh graph
	for (int i =0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t * link = &graph->flow_links[i];
		if (T_SAME_ID(link->source_node, node_id) && link->slot == 0) {
			lfr_schedule(link->target_node, graph, toil);
		}
	}

	// Shuffle queue (inefficient)
	for (int i = 1; i < toil->num_schedueled_nodes; i++) {
		toil->schedueled_nodes[i-1] = toil->schedueled_nodes[i];
	}
	return toil->num_schedueled_nodes--;
}


//// LFR Graph ////

/**
Initialize an LFR graph.
**/
void lfr_init_graph(lfr_graph_t *graph) {
	graph->nodes.next_id = 1;
	graph->num_flow_links = 0;
	graph->next_node_pos = (lfr_vec2_t) {100,100};
}


/**
Terminate an LFR graph.
**/
void lfr_term_graph(lfr_graph_t *graph) {
}


/**
Add a node with the given instruction to the graph.

Node is positioned to the right of the last node in the table.
**/
lfr_node_id_t lfr_add_node(lfr_instruction_e inst, lfr_graph_t *graph) {
	lfr_node_id_t id = lfr_insert_node_into_table(inst, &graph->nodes);
	graph->nodes.position[graph->nodes.num_rows - 1] = graph->next_node_pos;
	graph->next_node_pos.x += 260;
	return id;
}


/**
Link execution of one node to another.
**/
void lfr_link_nodes(lfr_node_id_t source_node, unsigned slot, lfr_node_id_t target_node, lfr_graph_t *graph) {
	assert(graph->num_flow_links < lfr_graph_max_flow_links);

	// Prevent duplicates
	for (int i = 0; i < graph->num_flow_links; i++) {
		lfr_flow_link_t *link = &graph->flow_links[i];
		if ( !T_SAME_ID(source_node, link->source_node)) { continue; }
		if ( slot != link->slot) { continue; }
		if ( !T_SAME_ID(target_node, link->target_node)) { continue; }

		return;
	}

	// Add (non-duplicate link)
	graph->flow_links[graph->num_flow_links++] = (lfr_flow_link_t) {source_node, target_node, slot};
}


/**
Break execution link from one node to another.
**/
void lfr_unlink_nodes(lfr_node_id_t source_node, unsigned slot, lfr_node_id_t target_node, lfr_graph_t *graph) {
	for (int i = 0; i < graph->num_flow_links; i++) {
		lfr_flow_link_t *link = &graph->flow_links[i];
		if ( !T_SAME_ID(source_node, link->source_node)) { continue; }
		if ( slot != link->slot) { continue; }
		if ( !T_SAME_ID(target_node, link->target_node)) { continue; }

		// Remove (breaking order)
		graph->flow_links[i] = graph->flow_links[--graph->num_flow_links];
	}
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

	// Flow links in graph
	char_count += fprintf(stream, "[[ LFR Flow links ]]\n");
	char_count += fprintf(stream, "|Source\t|Slot \t|=>|Target\t|\n");
	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		fprintf(stream, "|[#%u|%u]\t|Slot %u\t|=>|[#%u|%u]\t|\n"
			, link->source_node.id, T_INDEX(graph->nodes, link->source_node)
			, link->slot
			, link->target_node.id, T_INDEX(graph->nodes, link->target_node)
			);
	}
	char_count += fprintf(stream, "\n");

	return char_count;
}


//// LFR Node table ////
/**
Insert a new node at the end of the table.
**/
lfr_node_id_t lfr_insert_node_into_table(lfr_instruction_e inst, lfr_node_table_t *table) {
	assert(table->num_rows < lfr_node_table_max_rows);

	// Insert row into sparse table
	int index = T_INSERT_ROW(*table, lfr_node_id_t);

	// Set row data
	table->node[index].instruction = inst;
	table->position[index] = (lfr_vec2_t) { 0, 0};

	return table->dense_id[index];
}


/**
Get node index for the given id.
**/
unsigned lfr_get_node_index(lfr_node_id_t id, const lfr_node_table_t *table) {
	return T_INDEX(*table, id);
}

/**
Get position of a node in the table.
**/
lfr_vec2_t lfr_get_node_position(lfr_node_id_t id, const lfr_node_table_t *table) {
	return table->position[T_INDEX(*table, id)];
}


/**
Set position of a node in the table.
**/
void lfr_set_node_position(lfr_node_id_t id, lfr_vec2_t pos, lfr_node_table_t *table) {
	table->position[T_INDEX(*table, id)] = pos;
}


/**
Print node table content onto file stream.
**/
int lfr_fprint_node_table(const lfr_node_table_t *table, FILE * restrict stream) {
	int char_count = 0;

	char_count += fprintf(stream, "|ID\t|LFR Instruction\t|Flow slots\t|\n");
	T_FOR_ROWS(index, *table) {
		char_count += fprintf(stream, "|[#%u|%u]\t|", T_ID(*table,index).id, index);
		char_count += fprintf(stream, "%s\t|", lfr_get_instruction_name(table->node[index].instruction));
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
