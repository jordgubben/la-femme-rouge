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

void run_gui(lfr_graph_t*);
void show_graph(struct nk_context*, lfr_graph_t *);

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
	lfr_link_nodes(n1, 0, n2, &graph);
	lfr_fprint_graph(&graph, stdout);

	// Run graph (step by step)
	lfr_toil_t toil = {0};
	lfr_schedule(n1, &graph, &toil);
	while (lfr_step(&graph, &toil)) { /* Do nothing */ }

	run_gui(&graph);

	lfr_term_graph(&graph);
	return 0; 
}


#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

/**
Run a single window application, where a graph could be rendered.
**/
void run_gui(lfr_graph_t* graph) {
	// Initialize window application
	app_t app = {0};
	if(!init_gl_app(1024,768, &app)) {
		term_gl_app(&app);
		return;
	}

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

		show_graph(ctx, graph);

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
void show_graph(struct nk_context*  ctx, lfr_graph_t *graph) {
	// Show nodes as individual windows
	nk_flags node_window_flags = 0
		| NK_WINDOW_MOVABLE
		| NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE
		;
	lfr_node_id_t *node_ids = &graph->nodes.dense_id[0];
	int num_nodes = graph->nodes.num_rows;
	for (int index = 0; index < num_nodes; index++) {
		// Window title
		char title[1024];
		lfr_instruction_e inst = graph->nodes.node[index].instruction;
		const char* inst_name = lfr_get_instruction_name(inst);
		snprintf(title, 1024, "[#%u|%u] %s", node_ids[index].id, index, inst_name);

		// Initial window rect
		lfr_vec2_t pos = graph->nodes.position[index];
		struct nk_rect rect =  nk_rect(pos.x, pos.y, 250, 200);

		// Show the window
		if (nk_begin(ctx, title, rect, node_window_flags)) {
			nk_layout_row_dynamic(ctx, 0, 2);
			nk_label(ctx, "Example label", NK_TEXT_LEFT);
			if (nk_button_label(ctx, "Example button")) {
				printf("Button [#%u|%u]] pressed!\n", node_ids[index].id, index);
			}
		}
		nk_end(ctx);
	}
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
