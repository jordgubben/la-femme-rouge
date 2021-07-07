/****
La Femme Rough - An minimal graph based scripting system for games.
****/
#ifndef LFR_H
#define LFR_H

//// LFR Base types ////

typedef struct lfr_vec2_ { float x,y; } lfr_vec2_t;

typedef enum lfr_variant_type_ {
	lfr_nil_type,
	lfr_float_type,
	lfr_vec2_type,
	lfr_no_core_types
} lfr_variant_type_e;

typedef struct lfr_variant_ {
	lfr_variant_type_e type;
	union {
		int int_value;
		float floar_value;
		lfr_vec2_t vec2_value;
	};
} lfr_variant_t;


//// LFR Instructions ////

typedef enum lfr_instruction_ {
	lfr_print_own_id,
	lfr_no_core_instructions // Not an instruction :P
} lfr_instruction_e;

const char* lfr_get_instruction_name(lfr_instruction_e);
lfr_instruction_e lfr_find_instruction_from_name(const char* name);

//// LFR Node ////

typedef struct lfr_node_id_ { unsigned id; } lfr_node_id_t;

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

// Node CRUD
lfr_node_id_t lfr_insert_node_into_table(lfr_instruction_e, lfr_node_table_t*);
unsigned lfr_get_node_index(lfr_node_id_t, const lfr_node_table_t *);
lfr_vec2_t lfr_get_node_position(lfr_node_id_t, const lfr_node_table_t *);
void lfr_set_node_position(lfr_node_id_t, lfr_vec2_t, lfr_node_table_t *);

// Node serialization
int lfr_save_node_table_to_file(const lfr_node_table_t*, FILE * restrict stream);


/// LFR Graph ////
typedef struct lfr_flow_link_ {
	lfr_node_id_t source_node, target_node;
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

// Link CRUD
void lfr_link_nodes(lfr_node_id_t, lfr_node_id_t, lfr_graph_t*);
bool lfr_has_link(lfr_node_id_t, lfr_node_id_t, const lfr_graph_t*);
unsigned lfr_count_node_source_links(lfr_node_id_t, const lfr_graph_t*);
unsigned lfr_count_node_target_links(lfr_node_id_t, const lfr_graph_t*);
void lfr_unlink_nodes(lfr_node_id_t, lfr_node_id_t, lfr_graph_t*);

// Graph serialization
void lfr_load_graph_from_file(FILE * restrict stream, lfr_graph_t *graph);
int lfr_save_graph_to_file(const lfr_graph_t *, FILE * restrict stream);
int lfr_save_links_to_file(const lfr_graph_t *, FILE * restrict stream);


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


//// Utility macros ////
/* Log from a function, including the function name in the string. */
#define LFR_TRACE(m, ...) \
	printf("# %s():\t" m "\n", __func__, __VA_ARGS__);

//// Internals (defined further down) ////

/** Full definition of a single instruction. **/
typedef struct lfr_instruction_def_ {
	const char *name;
	void (*func)(lfr_node_id_t, lfr_variant_t input[], lfr_variant_t output[], const lfr_graph_t *);
} lfr_instruction_def_t;

const struct lfr_instruction_def_* lfr_get_instruction(lfr_instruction_e);

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
	if (head->instruction < lfr_no_core_instructions) {
		lfr_variant_t input[8], output[8];
		lfr_get_instruction(head->instruction)->func(node_id, input, output, graph);
	}

	// Enqueue nodes - Continue flow throgh graph
	for (int i =0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t * link = &graph->flow_links[i];
		if (T_SAME_ID(link->source_node, node_id)) {
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
void lfr_link_nodes(lfr_node_id_t source_node, lfr_node_id_t target_node, lfr_graph_t *graph) {
	assert(graph->num_flow_links < lfr_graph_max_flow_links);

	// Prevent duplicates
	if (lfr_has_link(source_node, target_node, graph)) { return; };

	// Add (non-duplicate link)
	graph->flow_links[graph->num_flow_links++] = (lfr_flow_link_t) {source_node, target_node};
}


/**
Is there a linke from one node to the other?
**/
bool lfr_has_link(lfr_node_id_t source_node, lfr_node_id_t target_node, const lfr_graph_t *graph) {
	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		if ( !T_SAME_ID(source_node, link->source_node)) { continue; }
		if ( !T_SAME_ID(target_node, link->target_node)) { continue; }

		return true;
	}

	return false;
}


/**
How many links have this node as source?
**/
unsigned lfr_count_node_source_links(lfr_node_id_t source_node, const lfr_graph_t *graph) {
	unsigned count = 0;

	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		if ( T_SAME_ID(source_node, link->source_node)) { count++; }
	}

	return count;
}


/**
How many links have this node as target?
**/
unsigned lfr_count_node_target_links(lfr_node_id_t target_node, const lfr_graph_t *graph) {
	unsigned count = 0;

	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		if ( T_SAME_ID(target_node, link->target_node)) { count++; }
	}

	return count;
}

/**
Break execution link from one node to another.
**/
void lfr_unlink_nodes(lfr_node_id_t source_node, lfr_node_id_t target_node, lfr_graph_t *graph) {
	for (int i = 0; i < graph->num_flow_links; i++) {
		lfr_flow_link_t *link = &graph->flow_links[i];
		if ( !T_SAME_ID(source_node, link->source_node)) { continue; }
		if ( !T_SAME_ID(target_node, link->target_node)) { continue; }

		// Remove (breaking order)
		graph->flow_links[i] = graph->flow_links[--graph->num_flow_links];
		return;
	}
}


/**
Load graph content from (tab separated) file.
**/
void lfr_load_graph_from_file(FILE * restrict stream, lfr_graph_t *graph) {
	char line_buf[1024];
	while (fgets(line_buf, 1024, stream)) {
		// Get line type
		char type_buf[8];
		sscanf(line_buf, "%s", type_buf);

		// Parse various line types
		if (strlen(type_buf) == 0) {
			/* Skip if empty */
		} else if (strcmp(type_buf, "node") == 0) {
			// Parse instruction
			lfr_node_id_t expected_id;
			char inst_buf[32];
			lfr_vec2_t pos;
			sscanf(line_buf, "node #%u %s (%f,%f)", &expected_id.id, inst_buf, &pos.x, &pos.y);

			// Add instruction
			lfr_instruction_e instruction = lfr_find_instruction_from_name(inst_buf);
			lfr_node_id_t new_id = lfr_insert_node_into_table(instruction, &graph->nodes);
			assert(T_SAME_ID(new_id, expected_id));
			lfr_set_node_position(new_id, pos, &graph->nodes);

		} else if (strcmp(type_buf, "link") == 0) {
			// Parse and create link
			lfr_node_id_t source, target;
			sscanf(line_buf, "link #%u -> #%u", &source.id, &target.id);
			lfr_link_nodes(source, target, graph);

		} else {
			fprintf(stderr, "Unknown type '%s'\n", type_buf);
		}
	}
}

/**
Dump graph to file in a parsable (tab-separated) format.
**/
int lfr_save_graph_to_file(const lfr_graph_t *graph, FILE * restrict stream) {
	int char_count = 0;

	// Dump things
	char_count += lfr_save_node_table_to_file(&graph->nodes, stream);
	char_count += lfr_save_links_to_file(graph, stream);

	return char_count;
}


/**
Dump main flow links in a parsable (tab-separated) format.
**/
int lfr_save_links_to_file(const lfr_graph_t *graph, FILE * restrict stream) {
	int char_count = 0;

	// Flow links in graph
	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		char_count += fprintf(stream, "link\t");
		char_count += fprintf(stream, "#%u -> #%u\t", link->source_node.id, link->target_node.id);
		char_count += fprintf(stream, "\n");
	}

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
Print node table content onto file stream in a parser friendly (tab separated) format.
**/
int lfr_save_node_table_to_file(const lfr_node_table_t *table, FILE * restrict stream) {
	int char_count = 0;

	T_FOR_ROWS(index, *table) {
		char_count += fprintf(stream, "node\t");

		// ID
		lfr_node_id_t node_id = T_ID(*table,index);
		char_count += fprintf(stream, "#%u\t", node_id.id);

		// Instruction
		char_count += fprintf(stream, "%s\t", lfr_get_instruction_name(table->node[index].instruction));

		// Position
		lfr_vec2_t pos = lfr_get_node_position(node_id, table);
		char_count += fprintf(stream, "(%f, %f)\t", pos.x, pos.y);

		char_count += fprintf(stream, "\n");
	}

	return char_count;
}


//// LFR Instructions ////

void lfr_print_own_id_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		const lfr_graph_t* graph) {

	unsigned node_index = T_INDEX(graph->nodes, node_id);
	printf("Node ID: [#%u|%u]\n", node_id.id, node_index);
}


/**
Look up table of all core instructions.
**/
static const lfr_instruction_def_t lfr_core_instructions_[lfr_no_core_instructions] = {
	{"print_own_id", lfr_print_own_id_proc},
};


/**
Get entire (code) instruction definition.
**/
const lfr_instruction_def_t* lfr_get_instruction(lfr_instruction_e inst) {
	assert(inst < lfr_no_core_instructions);
	return &lfr_core_instructions_[inst];
}


/**
Get name of (core) instruction.
**/
const char* lfr_get_instruction_name(lfr_instruction_e inst) {
	return lfr_get_instruction(inst)->name;
}


/**
Get (core) instruction code from name.
**/
lfr_instruction_e lfr_find_instruction_from_name(const char* name) {
	for (int i = 0; i < lfr_no_core_instructions; i++) {
		if (strcmp(name, lfr_get_instruction_name(i)) == 0) {
			return i;
		}
	}

	// Warn, then fallback to intruction that does not do much at all
	const lfr_instruction_e fallback = lfr_print_own_id;
	printf("Unknown instruction '%s' substituted by '%s'\n", name, lfr_get_instruction_name(fallback));
	return fallback;
}


#undef T_HAS_ID
#undef T_INDEX
#undef T_ID
#undef T_FOR_ROWS
#undef LFR_TRACE
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
