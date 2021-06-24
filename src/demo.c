/****
LFR Scripting demo.
****/

// LIBC
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

// OpenGl et.al.
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Nuklear
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include "nuklear.h"
#include "demo/glfw_opengl3/nuklear_glfw_gl3.h"

// La femme rouge
#include "lfr.h"

#define CHECK_GL(hint) check_gl(hint, __LINE__)
#define CHECK_GL_OR(hint, bail) if(!check_gl(hint, __LINE__)) { bail; }

void run_gui(lfr_graph_t *, lfr_toil_t *);
void show_graph(struct nk_context*, lfr_graph_t *, lfr_toil_t *);
void show_toil_queue(struct nk_context*, lfr_graph_t *, lfr_toil_t *);

//// GLWF Application
typedef struct app_ {
	GLFWwindow* window;
} app_t;
bool init_gl_app(int, int, app_t*);
void term_gl_app(app_t*);
bool check_gl(const char* hint, int line);

int main( int argc, char** argv) {
	lfr_graph_t graph = {0};
	lfr_init_graph(&graph);

	// Construct graph
	lfr_node_id_t n1 = lfr_add_node(lfr_print_own_id, &graph);
	lfr_node_id_t n2 = lfr_add_node(lfr_print_own_id, &graph);
	lfr_node_id_t n3 = lfr_add_node(lfr_print_own_id, &graph);
	lfr_link_nodes(n1, 0, n2, &graph);
	lfr_link_nodes(n1, 0, n3, &graph);
	lfr_fprint_graph(&graph, stdout);

	// Set a starting point in the graph
	lfr_toil_t toil = {0};
	lfr_schedule(n1, &graph, &toil);

	run_gui(&graph, &toil);

	lfr_term_graph(&graph);
	return 0; 
}


#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

/**
Run a single window application, where a graph could be rendered.
**/
void run_gui(lfr_graph_t* graph, lfr_toil_t *toil) {
	// Initialize window application
	app_t app = {0};
	if(!init_gl_app(1024,768, &app)) {
		term_gl_app(&app);
		return;
	}

	double last_step_time = glfwGetTime();
	const double time_between_steps = 1.f;

	// Init Nuklear
	struct nk_glfw glfw = {0};
	struct nk_context *ctx = nk_glfw3_init(&glfw, app.window, NK_GLFW3_INSTALL_CALLBACKS);
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
			lfr_step(graph, toil);
		}

		// Prep UI
		nk_glfw3_new_frame(&glfw);

		// Example window
		nk_flags window_flags = 0
			| NK_WINDOW_MOVABLE
			| NK_WINDOW_SCALABLE
			| NK_WINDOW_TITLE
			;
		if (nk_begin(ctx, "Example window", nk_rect(100,500, 300, 200), window_flags)) {
			nk_layout_row_dynamic(ctx, 0, 2);
			nk_label(ctx, "Example label", NK_TEXT_LEFT);
			if (nk_button_label(ctx, "Example button")) {
				printf("Button pressed!\n");
			}
		}
		nk_end(ctx);

		show_graph(ctx, graph, toil);
		show_toil_queue(ctx, graph, toil);

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
	term_gl_app(&app);
}


/**
Show a script graph using Nuclear widgets.
**/
void show_graph(struct nk_context*  ctx, lfr_graph_t *graph, lfr_toil_t* toil) {
	assert(ctx && graph && toil);

	// Show nodes as individual windows
	nk_flags node_window_flags = 0
		| NK_WINDOW_MOVABLE
		| NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE
		;
	lfr_node_id_t *node_ids = &graph->nodes.dense_id[0];
	int num_nodes = graph->nodes.num_rows;
	for (int index = 0; index < num_nodes; index++) {
		lfr_node_id_t node_id = node_ids[index];

		// Window title
		char title[1024];
		lfr_instruction_e inst = graph->nodes.node[index].instruction;
		const char* inst_name = lfr_get_instruction_name(inst);
		snprintf(title, 1024, "[#%u|%u] %s", node_id.id, index, inst_name);

		// Initial window rect
		lfr_vec2_t pos = lfr_get_node_position(node_id, &graph->nodes);
		struct nk_rect rect =  nk_rect(pos.x, pos.y, 250, 200);

		// Show the window
		bool highlight = (toil->num_schedueled_nodes && node_id.id ==  toil->schedueled_nodes[0].id);
		nk_flags highlight_flag = highlight ? NK_WINDOW_BORDER : 0;
		if (nk_begin(ctx, title, rect, node_window_flags | highlight_flag)) {
			nk_layout_row_dynamic(ctx, 0, 2);
			nk_label(ctx, "Example label", NK_TEXT_LEFT);
			if (nk_button_label(ctx, "Schedule me")) {
				printf("Scheduling node [#%u|%u]].\n", node_id.id, index);
				lfr_schedule(node_id, graph, toil);
			}

			// Update node position
			struct nk_vec2 p = nk_window_get_position(ctx);
			lfr_vec2_t node_pos = {p.x, p.y};
			lfr_set_node_position(node_id, node_pos, &graph->nodes);
		}
		nk_end(ctx);
	}

	// Show node flow
	if (nk_begin(ctx, "Node editor BG", nk_rect(0,0, 512, 768), NK_WINDOW_BACKGROUND)) {
		struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
		nk_fill_rect(canvas, nk_window_get_bounds(ctx), 0.f, nk_rgb(20,20,20));
		for (int i = 0; i < graph->num_flow_links; i++) {
			const lfr_flow_link_t *link = &graph->flow_links[i];
			// Source end
			lfr_vec2_t p1 = lfr_get_node_position(link->source_node, &graph->nodes);
			p1.x += 250;
			p1.y += 10;

			// Target end
			lfr_vec2_t p2 = lfr_get_node_position(link->target_node, &graph->nodes);
			p2.y += 10;

			// Draw line
			nk_stroke_line(canvas, p1.x, p1.y, p2.x,p2.y, 2.f, nk_rgb(100,100,100));
		}
	}
	nk_end(ctx);
}


/**
Show queued nodes.
**/
void show_toil_queue(struct nk_context *ctx, lfr_graph_t *graph, lfr_toil_t *toil) {
	assert(ctx && graph && toil);
	// Example window
	nk_flags window_flags = 0
		| NK_WINDOW_MOVABLE
		| NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE
		;
	if (nk_begin(ctx, "Queued nodes", nk_rect(500,500, 300, 200), window_flags)) {
		nk_layout_row_dynamic(ctx, 0, 1);
		for (int i = 0 ; i < toil->num_schedueled_nodes; i++) {
			lfr_node_id_t node_id = toil->schedueled_nodes[i];
			unsigned index = graph->nodes.sparse_id[node_id.id];
			char label[1024];
			snprintf(label, 1024, "Node [#%u|%u]", node_id.id, index);
			nk_label(ctx, label, NK_TEXT_LEFT);
		}
	}
	nk_end(ctx);
}

//// Application ////

/**
Init application, giving us a GLFW window to render to with OpenGL.
**/
bool init_gl_app(int width, int heigt, app_t* app) {
	void error_callback(int error, const char* description);
	void describe_gl_driver();

	assert(app);
	app->window = NULL;

	// Start up GLFW
	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		fprintf(stderr, "Failed to init GLFW!\n");
		return false;
	}

	// Create window with (recent) OpenGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	app->window = glfwCreateWindow(width, heigt, "Minimal GLFW window", NULL, NULL);
	if (!app->window) {
		fprintf(stderr, "Failed to create GLFW window!\n");
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(app->window);
	glfwSwapInterval(1);
	glewInit();
	describe_gl_driver();

	// All went well
	return true;
}


/**
GLFW Callback - Print GLFW errors to stderr.
**/
void error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW Error #%0d '%s'\n", error, description);
}


/**
Check that there are no OpenGL errors.
**/
bool check_gl(const char* hint, int line) {
	bool ok = true;
	GLenum e = glGetError();
	while( e != GL_NO_ERROR)  {
		ok = false;
		fprintf(stderr, "OpenGL failed to '%s' due to error code [0x%x] after line [%d].\n", hint, e, line);
		e = glGetError();
	}
	return ok;
}


/**
Print information about OpenGL to stdout.
**/
void describe_gl_driver() {
	printf("OpenGL\n======\n");
	printf("Vendor: %s\n", glGetString( GL_VENDOR ));
	printf("Renderer: %s\n", glGetString( GL_RENDERER ));
	printf("OpenGL Version: %s\n", glGetString( GL_VERSION ));
	printf("GLSL Version: %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ));
	printf("Extention  #0: %s\n", glGetStringi( GL_EXTENSIONS, 0 ));
	printf("=======\n\n");
	CHECK_GL("Describe connections");
}


/**
Terminate application.
**/
void term_gl_app(app_t* app) {
	if (app->window) { glfwDestroyWindow(app->window); }
	glfwTerminate();
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
