/****
LFR Scripting editor based on Nuklear.

Requirements:
 - libc
 - Nuklear
 - lfr.h
****/
#ifndef LFR_EDITOR_H
#define LFR_EDITOR_H

const char *bg_window_title = "Graph editor BG";
const unsigned node_window_w = 210, node_window_h = 330;

typedef enum editor_mode_ {
	em_normal,
	em_select_flow_prev,
	em_select_flow_next,
	em_select_data_link_input,
	em_select_data_link_output,
	no_em_modes // Not a mode :P
} editor_mode_e;

typedef struct lfr_editor_ {
	GLFWwindow* window;
	struct nk_context *ctx;

	// Interaction (mode based)
	editor_mode_e mode;
	lfr_node_id_t active_node_id;
	unsigned active_slot;

	// Removing nodes
	lfr_node_id_t removal_of_node_requested;

	// Layout
	float input_ys[lfr_node_table_max_rows][lfr_signature_size];
	lfr_vec2_t output_positions[lfr_node_table_max_rows][lfr_signature_size];
} lfr_editor_t;


// Editor
void lfr_show_editor(lfr_editor_t * app, const lfr_vm_t*, lfr_graph_t *, lfr_graph_state_t *);
void lfr_show_debug(struct nk_context*, lfr_graph_t *, lfr_graph_state_t *);

#endif // LFR_EDITOR_H

#ifdef LFR_EDITOR_IMPLEMENTATION

// Individual node windows
void show_individual_node_window(lfr_node_id_t, const lfr_vm_t*, lfr_graph_t*, lfr_graph_state_t*, lfr_editor_t*);
void show_node_main_flow_section(lfr_node_id_t, lfr_graph_t *, struct nk_context *);
void show_node_input_slots_group(lfr_node_id_t,
	const lfr_vm_t*, const lfr_graph_state_t*, lfr_graph_t*, lfr_editor_t*);
void show_node_output_slots_group(lfr_node_id_t,
	const lfr_vm_t *, const lfr_graph_state_t*, lfr_graph_t*, lfr_editor_t*);

// Lines between nodes
void show_editor_bg_window(lfr_graph_t *, const lfr_vm_t*, const lfr_editor_t *);
void draw_flow_link_lines(const lfr_editor_t *, const lfr_graph_t *, struct nk_command_buffer *);
void draw_data_link_lines(const lfr_editor_t *, const lfr_graph_t *, struct nk_command_buffer *);
void draw_link_selection_curve(const lfr_editor_t *, const lfr_graph_t *, struct nk_command_buffer *);
void show_node_creation_contextual_menu(const lfr_vm_t *, struct nk_context *, lfr_graph_t *);


/**
Show a script graph using Nuclear widgets.
**/
void lfr_show_editor(lfr_editor_t *app, const lfr_vm_t *vm, lfr_graph_t *graph, lfr_graph_state_t* state) {
	assert(app && graph && state);
	struct nk_context *ctx = app->ctx;

	// Show nodes as individual windows
	lfr_node_id_t *node_ids = &graph->nodes.dense_id[0];
	int num_nodes = graph->nodes.num_rows;
	for (int index = 0; index < num_nodes; index++) {
		lfr_node_id_t node_id = node_ids[index];
		show_individual_node_window(node_id, vm, graph, state, app);
	}

	// Show node flow et.al.
	show_editor_bg_window(graph, vm, app);

	// Remove node that has recently had it's window closed
	// (Avoid pain by waitning until after cycling through nodes before removing one)
	if (app->removal_of_node_requested.id) {
		lfr_remove_node(app->removal_of_node_requested, graph);
		app->removal_of_node_requested = (lfr_node_id_t) {0};
	}

	// Return editor to 'normal' mode by clicking on background
	if (nk_window_is_active(ctx, bg_window_title) && app->mode != em_normal) {
		app->mode = em_normal;
	}
}


/**
Show the window for an individual graph node.
**/
void show_individual_node_window(
		lfr_node_id_t node_id,
		const lfr_vm_t *vm,
		lfr_graph_t *graph,
		lfr_graph_state_t *state,
		lfr_editor_t *app) {
	assert(graph && state && app);
	struct nk_context *ctx = app->ctx;

	unsigned index = lfr_get_node_index(node_id, &graph->nodes);

	// Window title
	char title[1024];
	lfr_instruction_e inst = graph->nodes.node[index].instruction;
	const char* inst_name = lfr_get_instruction_name(inst, vm);
	snprintf(title, 1024, "[#%u|%u] %s", node_id.id, index, inst_name);

	// Initial window rect
	lfr_vec2_t pos = lfr_get_node_position(node_id, &graph->nodes);
	struct nk_rect rect =  nk_rect(pos.x, pos.y, node_window_w, node_window_h);

	// Show the window
	bool highlight = (state->num_schedueled_nodes && node_id.id ==  state->schedueled_nodes[0].id);
	nk_flags flags = 0
		| NK_WINDOW_MOVABLE
		| NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE
		| (highlight ? NK_WINDOW_BORDER : 0);
		;
	if (nk_begin(ctx, title, rect, flags)) {
		if (nk_tree_push_id(ctx, NK_TREE_NODE, "Main flow", NK_MINIMIZED, node_id.id)){
			// Add new links
			if (app->mode == em_normal) {
				nk_layout_row_dynamic(ctx, 0, 2);
				if (nk_button_label(ctx, "Prev?")){
					app->mode = em_select_flow_prev;
					app->active_node_id = node_id;
					printf("Entered select prev mode for [#%u|%u]].\n", node_id.id, index);
				}
				if (nk_button_label(ctx, "Next?")) {
					app->mode = em_select_flow_next;
					app->active_node_id = node_id;
					printf("Entered select prev mode for [#%u|%u]].\n", node_id.id, index);
				}
			}

			show_node_main_flow_section(node_id, graph, ctx);

			nk_tree_pop(ctx);
		}

		// Show "Link with this" button?
		if (app->mode == em_select_flow_prev || app->mode == em_select_flow_next) {
			// Define involved nodes
			lfr_node_id_t source_id, target_id;
			if (app->mode == em_select_flow_prev) {
				source_id = node_id;
				target_id = app->active_node_id;
			} else { // app->mode == em_select_flow_next
				source_id = app->active_node_id;
				target_id = node_id;
			}

			// Show button only if nodes are not alrweady linked
			if (!lfr_has_link(source_id, target_id, graph)) {
				nk_layout_row_dynamic(ctx, 0, 1);
				if (nk_button_label(ctx, "Link with this!")){
					// Link
					lfr_link_nodes(source_id, target_id, graph);

					// Clear
					app->mode = em_normal;
					app->active_node_id = (lfr_node_id_t){ 0 };
				}
			}
		}

		// Data (input and output)
		{
			nk_layout_row_dynamic(ctx, 150, 2);
			show_node_input_slots_group(node_id, vm, state, graph, app);
			show_node_output_slots_group(node_id, vm, state, graph, app);
		}

		// Misc. management
		nk_layout_row_dynamic(ctx, 0, 2);
		if (nk_button_label(ctx, "Remove me")) {
			app->removal_of_node_requested = node_id;
		}
		if (nk_button_label(ctx, "Schedule me")) {
			lfr_schedule(node_id, graph, state);
		}

		// Update node position
		struct nk_vec2 p = nk_window_get_position(ctx);
		lfr_vec2_t node_pos = {p.x, p.y};
		lfr_set_node_position(node_id, node_pos, &graph->nodes);
	}
	nk_end(ctx);
}


/*
Show all links involved in the given nodes main flow.
*/
void show_node_main_flow_section(lfr_node_id_t node_id, lfr_graph_t *graph, struct nk_context *ctx) {
	unsigned sl_count = lfr_count_node_source_links(node_id, graph);
	unsigned tl_count = lfr_count_node_target_links(node_id, graph);
	unsigned max_links = (sl_count > tl_count ? sl_count : tl_count);
	unsigned flow_section_heigh = 10 + 20 * max_links;

	//  Existing flow link section
	nk_layout_row_dynamic(ctx, flow_section_heigh, 2);

	// Flow links where node is target
	if (nk_group_begin(ctx, "Flow link targets", NK_WINDOW_NO_SCROLLBAR)) {
		nk_layout_row_dynamic(ctx, 15, 1);

		// For each link targeting this node
		for (int i = 0; i < graph->num_flow_links; i++) {
			lfr_flow_link_t *link = &graph->flow_links[i];
			if (link->target_node.id != node_id.id) { continue; }

			// 'Remove link' button
			char label[128];
			snprintf(label, 128, "(x) [#%u]", link->source_node.id);
			if (nk_button_label(ctx, label)) {
				lfr_unlink_nodes(link->source_node, link->target_node, graph);
			}
		}

		nk_group_end(ctx);
	}

	// Flow links where node is source
	if (nk_group_begin(ctx, "Flow link source", NK_WINDOW_NO_SCROLLBAR)) {
		nk_layout_row_dynamic(ctx, 15, 1);

		// For each link source node
		for (int i = 0; i < graph->num_flow_links; i++) {
			lfr_flow_link_t *link = &graph->flow_links[i];
			if (link->source_node.id != node_id.id) { continue; }

			// 'Remove link' button
			char label[128];
			snprintf(label, 128, "[#%u] (x)", link->target_node.id);
			if (nk_button_label(ctx, label)) {
				lfr_unlink_nodes(link->source_node, link->target_node, graph);
			}
		}

		nk_group_end(ctx);
	}
}


/*
Show an UI group listing all *input* data slots of the given node.
*/
void show_node_input_slots_group(
		lfr_node_id_t node_id,
		const lfr_vm_t *vm,
		const lfr_graph_state_t* state,
		lfr_graph_t *graph,
		lfr_editor_t *app) {
	assert(state && graph && app && app->ctx);
	struct nk_context *ctx = app->ctx;

	// Start group - Early out if it is hidden
	const nk_flags group_flags = 0;
	if (!nk_group_begin(ctx, "Input data", group_flags)) { return; }
	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, "Input", NK_TEXT_LEFT);

	// Get node info
	unsigned node_index = lfr_get_node_index(node_id, &graph->nodes);
	lfr_node_t *node = &graph->nodes.node[node_index];

	// Go over all (real) input slots
	for (int slot = 0; slot < lfr_signature_size; slot++) {
		const char* name = lfr_get_instruction(node->instruction, vm)->input_signature[slot].name;
		if (!name) { continue; };

		// Name (+ dummy buttons)
		nk_layout_row_template_begin(ctx, 15);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_end(ctx);
		if (app->mode == em_select_data_link_input) {
			// Link output slot on active node to input slot on this node
			if (nk_button_label(ctx, "Link!")) {
				// Link
				lfr_link_data(
					app->active_node_id, app->active_slot,
					node_id, slot,
					graph);

				// Clear editor mode
				app->mode = em_normal;
			}
		} else if (node->input_data[slot].node.id == 0) {
			// Enter data linking mode on button press
			if (nk_button_label(ctx, "+")) {
				app->mode = em_select_data_link_output;
				app->active_node_id = node_id;
				app->active_slot = slot;
			}
		} else {
			// Clear (x) link with button
			if (nk_button_label(ctx, "x")) {
				lfr_unlink_input_data(node_id, slot, graph);
			}
		}
		nk_label(ctx, name, NK_TEXT_LEFT);

		// Get current y
		// (Use when rendering slot connections)
		struct nk_panel* panel = nk_window_get_panel(ctx);
		app->input_ys[node_index][slot] = panel->at_y + 15/2;

		// Current input value (linked or fixed)
		// (Set a new value if it's changed by the UI, and breaks any link))
		nk_layout_row_dynamic(ctx, 0, 1);
		const lfr_variant_t data = lfr_get_input_value(node_id, slot, vm, graph, state);
		switch(data.type) {
		case lfr_nil_type: {
			nk_label(ctx, "---", NK_TEXT_RIGHT);
		} break;
		case lfr_float_type: {
			float new_value = nk_propertyf(ctx, "#=", FLT_MIN, data.float_value, FLT_MAX, 1, 1);
			if (data.float_value != new_value) {
				lfr_set_fixed_input_value(node_id, slot, lfr_float(new_value), &graph->nodes);
			}
		} break;
		case lfr_vec2_type: {
			// Set a new value if it's changed by the UI (breaks link)
			float new_x = nk_propertyf(ctx, "#=", FLT_MIN, data.vec2_value.x, FLT_MAX, 1, 1);
			float new_y = nk_propertyf(ctx, "#=", FLT_MIN, data.vec2_value.y, FLT_MAX, 1, 1);
			if (data.vec2_value.x != new_x || data.vec2_value.y != new_y) {
				lfr_set_fixed_input_value(node_id, slot, lfr_vec2_xy(new_x, new_y), &graph->nodes);
			}
		} break;
		default:
			nk_label(ctx, "???", NK_TEXT_RIGHT);
		}
	}
	nk_group_end(ctx);
}


/*
Show an UI group listing all *output* data slots of the given node.
*/
void show_node_output_slots_group(
		lfr_node_id_t node_id,
		const lfr_vm_t *vm,
		const lfr_graph_state_t* state,
		lfr_graph_t *graph,
		lfr_editor_t *app) {
	assert(state && graph && app && app->ctx);
	struct nk_context *ctx = app->ctx;

	// Start group - Eary out if it is hidden
	const nk_flags group_flags = 0;
	if (!nk_group_begin(ctx, "Output data", group_flags)) { return; }
	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, "Output", NK_TEXT_RIGHT);

	// Get node info
	unsigned node_index = lfr_get_node_index(node_id, &graph->nodes);
	lfr_instruction_e inst = graph->nodes.node[node_index].instruction;

	// Go over all (real) output slots
	for (int slot = 0; slot < lfr_signature_size; slot++) {
		const char* name = lfr_get_instruction(inst, vm)->output_signature[slot].name;
		lfr_variant_t data = lfr_get_output_value(node_id, slot, vm, graph, state);
		if (!name) { continue; }

		// Name (+ dummy buttons)
		nk_layout_row_template_begin(ctx, 15);
		nk_layout_row_template_push_dynamic(ctx);
		if (app->mode == em_select_data_link_output) {
			nk_layout_row_template_push_static(ctx, 40);
		} else {
			nk_layout_row_template_push_static(ctx, 20);
			nk_layout_row_template_push_static(ctx, 20);
		}
		nk_layout_row_template_end(ctx);
		nk_label(ctx, name, NK_TEXT_LEFT);
		if (app->mode == em_select_data_link_output) {
			// Link output slot on this node to input slot on active node
			if (nk_button_label(ctx, "Link!")) {
				// Link
				lfr_link_data(
					node_id, slot,
					app->active_node_id, app->active_slot,
					graph);

				// Clear mode
				app->mode = em_normal;
			}
		} else {
			// Clear (x) links with buttons
			if (nk_button_label(ctx, "x")) {
				lfr_unlink_output_data(node_id, slot, graph);
			}

			// Enter data linkin mode on button press
			if (nk_button_label(ctx, "+")) {
				app->mode = em_select_data_link_input;
				app->active_node_id = node_id;
				app->active_slot = slot;
			}
		}

		// Get current y
		// (Use when rendering slot connections)
		struct nk_panel* panel = nk_window_get_panel(ctx);
		app->output_positions[node_index][slot] =
			(lfr_vec2_t) {panel->bounds.x + panel->bounds.w + 30, panel->at_y + 15/2};

		// Value
		nk_layout_row_dynamic(ctx, 0, 1);
		char label_buf[32];
		switch (data.type) {
		case lfr_nil_type: {
			snprintf(label_buf, 32, "---");
		} break;
		case lfr_vec2_type: {
			snprintf(label_buf, 32, "%.1f,%.1f", data.vec2_value.x, data.vec2_value.y);
		} break;
		case lfr_float_type: {
			snprintf(label_buf, 32, "(%.3f)", data.float_value);
		} break;
		default:
			snprintf(label_buf, 32, "???");
		}
		nk_label(ctx, label_buf, NK_TEXT_RIGHT);
	}
	nk_group_end(ctx);
}


/**
Show background window with flow lines, link selection and context menu for creating new nodes.
**/
void show_editor_bg_window(lfr_graph_t *graph, const lfr_vm_t *vm, const lfr_editor_t *app) {
	assert(graph && app);
	struct nk_context *ctx = app->ctx;

	if (nk_begin(ctx, bg_window_title, nk_rect(0,0, 1024, 768), NK_WINDOW_BACKGROUND)) {
		struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

		nk_fill_rect(canvas, nk_window_get_bounds(ctx), 0.f, nk_rgb(20,20,20));
		draw_flow_link_lines(app, graph, canvas);
		draw_data_link_lines(app, graph, canvas);
		draw_link_selection_curve(app, graph, canvas);
		show_node_creation_contextual_menu(vm, ctx, graph);
	}
	nk_end(ctx);
}


/*
Draw main flow link lines in various orange colors.
*/
void draw_flow_link_lines(const lfr_editor_t *app, const lfr_graph_t *graph, struct nk_command_buffer *canvas) {
	assert(app && graph && canvas);

	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		// Source end
		lfr_vec2_t p1 = lfr_get_node_position(link->source_node, &graph->nodes);
		p1.x += node_window_w;
		p1.y += 20;

		// Target end
		lfr_vec2_t p2 = lfr_get_node_position(link->target_node, &graph->nodes);
		p2.y += 20;

		// Draw curve
		const float ex = 75;
		int r = 150 + (link->source_node.id * 11) % 100;
		int g = 100 + (link->target_node.id * 11) % 100;
		nk_stroke_curve(canvas,
			p1.x, p1.y, p1.x + ex, p1.y, p2.x - ex, p2.y, p2.x, p2.y,
			2.f, nk_rgb(r, g, 50));
	}
}


/*
Draw data lines in various green colors.
*/
void draw_data_link_lines(const lfr_editor_t *app, const lfr_graph_t *graph, struct nk_command_buffer *canvas) {
	assert(app && graph && canvas);

	for (int node_index = 0; node_index < graph->nodes.num_rows; node_index++) {
		lfr_node_id_t in_node_id = graph->nodes.dense_id[node_index];
		const lfr_node_t *node = &graph->nodes.node[node_index];
		const lfr_vec2_t node_win_pos = lfr_get_node_position(in_node_id, &graph->nodes);

		// For every linked pair of slots
		for (int slot = 0; slot < lfr_signature_size; slot++) {
			lfr_node_id_t out_node_id = node->input_data[slot].node;
			if (!out_node_id.id) { continue; }

			// Input slot height (on this node)
			lfr_vec2_t in_pos = {node_win_pos.x, app->input_ys[node_index][slot]};

			// Output slot position (on other node)
			unsigned output_index = lfr_get_node_index(out_node_id, &graph->nodes);
			unsigned output_slot = node->input_data[slot].slot;
			lfr_vec2_t out_pos = app->output_positions[output_index][output_slot];

			// Draw curve
			const float ex = 100;
			int g = 150 + (out_node_id.id * 11) % 100;
			int b = 100 + (in_node_id.id * 11) % 100;
			nk_stroke_curve(canvas,
				out_pos.x, out_pos.y, out_pos.x + ex, out_pos.y,
				in_pos.x - ex, in_pos.y, in_pos.x, in_pos.y,
				2.f, nk_rgb(50, g, b));
		}
	}
}

/*
Draw selection curve (if in one of the appropriate modes).
*/
void draw_link_selection_curve(const lfr_editor_t *app, const lfr_graph_t *graph, struct nk_command_buffer *canvas) {
	assert(app && graph && canvas);

	// Select source node in flow
	if (app->mode == em_select_flow_prev && app->active_node_id.id) {
		// Connected end
		lfr_vec2_t target_p = lfr_get_node_position(app->active_node_id, &graph->nodes);
		target_p.y += 20;

		// Mouse end
		double mouse_x, mouse_y;
		glfwGetCursorPos(app->window, &mouse_x, &mouse_y);

		// Signify mode
		nk_stroke_line(canvas, mouse_x, mouse_y, target_p.x, target_p.y, 5.f, nk_rgb(200,150,100));
	}

	// Select target node in flow
	if (app->mode == em_select_flow_next && app->active_node_id.id) {
		// Connected end
		lfr_vec2_t source_p = lfr_get_node_position(app->active_node_id, &graph->nodes);
		source_p.x += node_window_w;
		source_p.y += 20;

		// Mouse end
		double mouse_x, mouse_y;
		glfwGetCursorPos(app->window, &mouse_x, &mouse_y);

		// Signify mode
		nk_stroke_line(canvas, source_p.x, source_p.y, mouse_x, mouse_y, 5.f, nk_rgb(150,200,100));
	}
}


/*
Show contexual menu for creating new nodes.
*/
void show_node_creation_contextual_menu(const lfr_vm_t *vm, struct nk_context* ctx, lfr_graph_t *graph) {
	assert(ctx && graph);

	// Open context menu - or early out if it can't be opened
	struct nk_vec2 size = nk_vec2(100,200);
	struct nk_rect trigger_bounds = nk_window_get_bounds(ctx);
	if (!nk_contextual_begin(ctx, 0, size, trigger_bounds)) { return; }

	// List all available options
	nk_layout_row_dynamic(ctx, 25, 1);
	for (int i = 0; i < lfr_no_core_instructions; i++) {
		const char* name = lfr_get_instruction_name(i, vm);
		if ( nk_contextual_item_label(ctx, name, NK_TEXT_LEFT)) {
			// Create node
			lfr_node_id_t id = lfr_add_node(i, graph);

			// Position at cursor
			struct nk_vec2 mouse_pos = ctx->input.mouse.pos;
			lfr_vec2_t node_pos = {mouse_pos.x, mouse_pos.y};
			lfr_set_node_position(id, node_pos, &graph->nodes);
		}
	}
	nk_contextual_end(ctx);
}


/**
Show queued nodes.
**/
void lfr_show_debug(struct nk_context *ctx, lfr_graph_t *graph, lfr_graph_state_t *state) {
	assert(ctx && graph && state);
	// Example window
	nk_flags window_flags = 0
		| NK_WINDOW_MOVABLE
		| NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE
		;
	if (nk_begin(ctx, "Queued nodes", nk_rect(500,500, 300, 200), window_flags)) {
		nk_layout_row_dynamic(ctx, 0, 1);
		for (int i = 0 ; i < state->num_schedueled_nodes; i++) {
			lfr_node_id_t node_id = state->schedueled_nodes[i];
			unsigned index = graph->nodes.sparse_id[node_id.id];
			char label[1024];
			snprintf(label, 1024, "Node [#%u|%u]", node_id.id, index);
			nk_label(ctx, label, NK_TEXT_LEFT);
		}
	}
	nk_end(ctx);
}
#endif // LFR_EDITOR_IMPLEMENTATION


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
