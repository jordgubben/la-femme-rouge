/****
LFR Scripting demo app.
****/

// LIBC
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>

// OpenGL
#include "basic_gl.h"

// Nuklear
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#include "nuklear.h"
#include "demo/glfw_opengl3/nuklear_glfw_gl3.h"

// La femme rouge
#include "lfr.h"
#include "lfr_editor.h"

#define SHOW_EXAMPLE_WINDOW 0

// Editor
void run_gui(lfr_graph_t *, lfr_graph_state_t *);
void show_example_window(struct nk_context *);


/**
Starting pint for the demo application.
**/
int main( int argc, char** argv) {
	lfr_graph_t graph = {0};
	lfr_init_graph(&graph);

	// Optionally read graph script from file
	if (argc > 1) {
		printf("Loading graph from file: %s\n", argv[1]);
		FILE * fp = fopen(argv[1], "r");
		lfr_load_graph_from_file(fp, &graph);
		fclose(fp);
	} else {
		// Construct graph
		lfr_node_id_t n1 = lfr_add_node(lfr_print_own_id, &graph);

		// Randomizer
		lfr_node_id_t n2 = lfr_add_node(lfr_randomize_number, &graph);
		lfr_variant_t val = {lfr_float_type, .float_value = 0.5};
		lfr_set_default_output_value(n1, 0, val, &graph.nodes);

		// Addition
		lfr_node_id_t n3 = lfr_add_node(lfr_add, &graph);
		lfr_variant_t val_a = {lfr_float_type, .float_value = 1.5};
		lfr_set_fixed_input_value(n3, 0, val_a, &graph.nodes);
		lfr_variant_t val_b = {lfr_float_type, .float_value = 2.5};
		lfr_link_data(n2, 0, n3, 1, &graph);

		// Links
		lfr_link_nodes(n1, n2, &graph);
		lfr_link_nodes(n1, n3, &graph);
	}

	// Run graph in gui
	lfr_graph_state_t state = {0};
	run_gui(&graph, &state);

	// Optionally dump graph script to file
	if (argc > 1) {
		printf("Saving graph to file: %s\n", argv[1]);
		FILE * fp = fopen(argv[1], "w");
		lfr_save_graph_to_file(&graph, fp);
		fclose(fp);
	}

	lfr_term_graph(&graph);
	return 0; 
}


#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

/**
Run a single window application, where a graph could be rendered.
**/
void run_gui(lfr_graph_t* graph, lfr_graph_state_t *state) {
	// Initialize window application
	lfr_editor_t app = {0};
	if(!init_gl_app("LFR Editor example", 1024,768, &app.window)) {
		term_gl_app(app.window);
		return;
	}

	double last_step_time = glfwGetTime();
	const double time_between_steps = 1.f;

	// Init Nuklear
	struct nk_glfw glfw = {0};
	struct nk_context *ctx = nk_glfw3_init(&glfw, app.window, NK_GLFW3_INSTALL_CALLBACKS);
	app.ctx = ctx;
	{
		struct nk_font_atlas *atlas;
		nk_glfw3_font_stash_begin(&glfw, &atlas);
		nk_glfw3_font_stash_end(&glfw);
	}

	// Keep the motor runnin
	while(!glfwWindowShouldClose(app.window)) {
		// Take a step through the graph now and then
		double now = glfwGetTime();
		while (now  > last_step_time + time_between_steps) {
			last_step_time += time_between_steps;
			lfr_step(graph, state);
		}

		// Prep UI
		nk_glfw3_new_frame(&glfw);

#if SHOW_EXAMPLE_WINDOW
		show_example_window(ctx);
#endif // SHOW_EXAMPLE_WINDOW

		lfr_show_editor(&app, graph, state);
		lfr_show_debug(ctx, graph, state);

		// Prepare rendering
		int width, height;
		glfwGetWindowSize(app.window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(.75f, .95f, .75f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		CHECK_GL_OR("Prepare rendering", goto quit);

		// Render UI
		nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);

		// Cooperate with OS
		glfwSwapBuffers(app.window);
		glfwPollEvents();
	}

	// Terminate application
	quit:
	nk_glfw3_shutdown(&glfw);
	term_gl_app(app.window);
}


/**
Show Nuklear example window.
**/
void show_example_window(struct nk_context *ctx) {
	// Example window
	nk_flags window_flags = 0
		| NK_WINDOW_MOVABLE
		| NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE
		| NK_WINDOW_MINIMIZABLE
		;
	if (nk_begin(ctx, "Example window", nk_rect(25, 300, 500, 500), window_flags)) {
		nk_layout_row_dynamic(ctx, 0, 2);
		nk_label(ctx, "Example label", NK_TEXT_LEFT);
		if (nk_button_label(ctx, "Example button")) {
			printf("Button pressed!\n");
		}

		// Simple group inside window
		nk_layout_row_dynamic(ctx, 75, 1);
		if (nk_group_begin(ctx, "Example group", NK_WINDOW_TITLE)) {
			nk_layout_row_dynamic(ctx, 25, 3);
			nk_label(ctx, "Label L", NK_TEXT_LEFT);
			nk_label(ctx, "Label C", NK_TEXT_CENTERED);
			nk_label(ctx, "Label R", NK_TEXT_RIGHT);
			nk_group_end(ctx);
		}

		// Group positioned with space layout
		nk_layout_space_begin(ctx, NK_STATIC, 200, 1);
		{
			static struct nk_rect group_placement = {0,0, 200, 200};
			struct nk_panel *group_panel;
			nk_layout_space_push(ctx, group_placement);
			if (nk_group_begin(ctx, "Space group", NK_WINDOW_TITLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
				group_panel = nk_window_get_panel(ctx);

				// Group placement
				nk_layout_row_dynamic(ctx, 10, 1);
				nk_label(ctx, "Group placement", NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, 15, 2);
				nk_property_float(ctx, "x", 0, &group_placement.x, 1000, 1,1);
				nk_property_float(ctx, "y", 0, &group_placement.y, 1000, 1,1);
				nk_property_float(ctx, "w", 0, &group_placement.w, 1000, 1,1);
				nk_property_float(ctx, "h", 0, &group_placement.h, 1000, 1,1);

				// Panel offset
				nk_layout_row_dynamic(ctx, 10, 1);
				nk_label(ctx, "Panel offset", NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, 15, 2);
				int ox =  *group_panel->offset_x,  oy =  *group_panel->offset_y;
				nk_property_int(ctx, "x", 0, &ox, 1000, 1,1);
				nk_property_int(ctx, "y", 0, &oy, 1000, 1,1);

				// Panel at
				nk_layout_row_dynamic(ctx, 10, 1);
				nk_label(ctx, "Panel at", NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, 15, 2);
				nk_property_float(ctx, "x", 0, &group_panel->at_x, 1000, 1,1);
				nk_property_float(ctx, "y", 0, &group_panel->at_y, 1000, 1,1);

				// Group panel bounds
				nk_layout_row_dynamic(ctx, 10, 1);
				nk_label(ctx, "Panel bounds", NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, 15, 2);
				nk_property_float(ctx, "x", 0, &group_panel->bounds.x, 1000, 1,1);
				nk_property_float(ctx, "y", 0, &group_panel->bounds.y, 1000, 1,1);
				nk_property_float(ctx, "w", 0, &group_panel->bounds.w, 1000, 1,1);
				nk_property_float(ctx, "h", 0, &group_panel->bounds.h, 1000, 1,1);
				nk_group_end(ctx);
			}
			group_placement = nk_layout_space_rect_to_local(ctx, group_panel->bounds);
		}
		nk_layout_space_end(ctx);

		// Label after group(s)
		nk_layout_row_dynamic(ctx, 0, 1);
		nk_label(ctx, "~ After group(s) ~", NK_TEXT_CENTERED);
	}
	nk_end(ctx);
}

#define LFR_IMPLEMENTATION
#include "lfr.h"

#define LFR_EDITOR_IMPLEMENTATION
#include "lfr_editor.h"

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
