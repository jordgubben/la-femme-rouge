/****
LFR Scripting "game" used to demonstrate how to integrate LFR with an existing application.
****/

// LIBC
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

// Render Nuklear with OpenGL
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

// LFR
#include "lfr.h"
#include "lfr_editor.h"


// Shader program

const char* vertex_shader_src =
	"#version 330 core\n"
	"layout (location = 0) in vec3 attr_pos;\n"
	"layout (location = 1) in vec3 attr_color;\n"
	"out vec3 var_color;\n"
	"uniform mat4 transform;"
	"void main()\n"
	"{\n"
	"   var_color = attr_color;\n"
	"   gl_Position = transform * vec4(attr_pos, 1.0);\n"
	"}\0";


const char *fragment_shader_src =
	"#version 330 core\n"
	"in vec3 var_color;\n"
	"out vec4 frag_color;\n"
	"void main()\n"
	"{\n"
	"   frag_color = vec4(var_color, 1.0);\n"
	"}\0";


// Geometry
typedef struct gl_mesh_ {
	GLuint vao;
	GLuint position_vbo, color_vbo;
	GLuint ebo;
	unsigned num_indecies;
} gl_mesh_t;

typedef struct vec2_ { float x,y; } vec2_t;
typedef struct vec3_ { float x,y,z; } vec3_t;
typedef struct mat4_ {float m[16]; } mat4_t;

bool create_mesh(
	const vec3_t [], const vec3_t [], unsigned num_verticies,
	const unsigned [], unsigned num_indecies,
	gl_mesh_t *);
void render_mesh(const gl_program_t *, const gl_mesh_t *, const mat4_t *);
void delete_mesh(gl_mesh_t *);

const vec3_t triangle_positions[3] = {
	{0, +1, 0},
	{+1, 0, 0},
	{-1, 0, 0},
};

const vec3_t triangle_colors[3] = {
	{1, 0, 0},
	{0, 1, 0},
	{0, 0, 1},
};

const unsigned triangle_indices[3] = {
	0, 1,2,
};


const vec3_t unit_quad_positions[4] = {
	{-0.5, +0.5, 0},
	{+0.5, +0.5, 0},
	{+0.5, -0.5, 0},
	{-0.5, -0.5, 0},
};
const vec3_t unit_quad_colors[4] = {
	{0, 0, 0},
	{1, 1, 0},
	{0, 1, 1},
	{1, 0, 1},
};

const unsigned unit_quad_indices[3*2] = {
	0, 1, 2,
	0, 2, 3,
};


//// Game world ////

enum {
	num_actors_in_world = 4,
};

typedef struct population_ {
	vec2_t actor_positions[num_actors_in_world];

} population_t;

//// Games custom instructions ////

typedef enum game_instructions_ {
	gi_set_actor_position,
	gi_get_actor_position,
	gi_no_instructions // Not an instruction
} game_instructions_e;

/**
Script instruction: Set position of the given actor.
**/
void set_actor_position_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		void *custom_data,
		const lfr_graph_t* graph) {
	assert(custom_data);
	population_t *pop = custom_data;

	// Find actor
	int actor_index = input[0].int_value;
	actor_index %= num_actors_in_world;

	// Get new position
	lfr_vec2_t in_pos = input[1].vec2_value;

	// Set new position
	pop->actor_positions[actor_index] = (vec2_t) { in_pos.x, in_pos.y };
}


/**
Script instruction: Get position of the given actor.
**/
void get_actor_position_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		void *custom_data,
		const lfr_graph_t* graph) {
	assert(custom_data);
	population_t *pop = custom_data;
	
	// Find actor position
	int actor_index = input[0].int_value;
	actor_index %= num_actors_in_world;
	vec2_t pos = pop->actor_positions[actor_index];

	// Output position
	output[0] = lfr_vec2_xy(pos.x, pos.y);
}


lfr_instruction_def_t game_instructions[gi_no_instructions] = {
	{"set_actor_position", set_actor_position_proc,
		{
			{"ACTOR", {lfr_int_type, .int_value = 0 }},
			{"POS", (lfr_variant_t) { lfr_vec2_type, .vec2_value = { 0,0}}},
		},
		{},
	},
	{"get_actor_position", get_actor_position_proc,
		{{"ACTOR", {lfr_int_type, .int_value = 0 }}},
		{{"POS", (lfr_variant_t) { lfr_vec2_type, .vec2_value = { 0,0}}},},
	},
};


/**
Application starting point.
**/
int main( int argc, char** argv) {
	printf("gi_set_actor_position = %u\n" , gi_set_actor_position);
	printf("gi_get_actor_position = %u\n" , gi_get_actor_position);

	// Init world population
	population_t pop = {0};
	for (int i = 0; i < num_actors_in_world; i++) {
		pop.actor_positions[i] = (vec2_t) {-0.75 + 0.5 * i, 0.0};
	}

	// Initialize window (Full HD)
	GLFWwindow *win;
	if(!init_gl_app("Full HD game", 1920, 1080, &win)) {
		term_gl_app(win);
		return -1;
	}

	// Prepare basic shader program
	gl_program_t program = {0};
	if(!init_shader_program(vertex_shader_src, fragment_shader_src, &program)) {
		term_gl_app(win);
		return -10;
	}

	// Create a VAO to associate with some geometry to render
	gl_mesh_t triangle = {0};
	if (!create_mesh(triangle_positions, triangle_colors, 3, triangle_indices, 3, &triangle)) {
		term_gl_app(win);
		return -20;
	}

	gl_mesh_t unit_quad = {0};
	if (!create_mesh(unit_quad_positions, unit_quad_colors, 4, unit_quad_indices, 6, &unit_quad)) {
		term_gl_app(win);
		return -20;
	}

	// Init Nuklear
	struct nk_glfw glfw = {0};
	struct nk_context *ctx = nk_glfw3_init(&glfw, win, NK_GLFW3_INSTALL_CALLBACKS);
	{
		struct nk_font_atlas *atlas;
		nk_glfw3_font_stash_begin(&glfw, &atlas);
		nk_glfw3_font_stash_end(&glfw);
	}

	// Setup LFR
	lfr_vm_t vm = {game_instructions, gi_no_instructions, &pop};
	lfr_graph_t graph = {0};
	lfr_init_graph(&graph);
	lfr_graph_state_t graph_state = {0};
	lfr_editor_t editor = {0};
	lfr_init_editor(nk_rect(0,0, 1920, 1080/2), win, ctx, &editor);

	// Set up LFR example
	// (keep things simmple by skipping load from file)
	lfr_node_id_t n1 = lfr_add_custom_node(gi_get_actor_position, &graph);
	lfr_node_id_t n2 = lfr_add_custom_node(gi_set_actor_position, &graph);
	lfr_link_nodes(n1, n2, &graph);
	lfr_link_data(n1, 0, n2, 1, &graph);

	// Script stepping timer
	double last_step_time = glfwGetTime();
	const double time_between_steps = 1.f;

	// Keep the motor runnin
	while(!glfwWindowShouldClose(win)) {
		// Take a step through the graph now and then
		double now = glfwGetTime();
		while (now  > last_step_time + time_between_steps) {
			last_step_time += time_between_steps;
			lfr_step(&vm, &graph, &graph_state);
		}

		// Prep UI
		nk_glfw3_new_frame(&glfw);

		// LFR editor
		lfr_show_editor(&editor, &vm, &graph, &graph_state);

		// Prepare rendering
		int width, height;
		glfwGetWindowSize(win, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(.75f, .55f, .75f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		CHECK_GL_OR("Prepare rendering", goto quit);

		// Render actors to the lower half of the screen
		glViewport(0, 0, width, height/2);

		// Render some actors
		// (So that the custom controls have something to manipulate)
		for (int i = 0; i < num_actors_in_world; i++) {
			vec2_t pos = pop.actor_positions[i];
			const float r = ((float) height *0.5)/ ((float)width);
			const float s = 0.2;
			const mat4_t transform = {
				s, 0, 0, pos.x,
				0, s/r, 0, pos.y/r,
				0, 0,.5, 0,
				0, 0, 0, 1
			};

			// Render world
			render_mesh(&program, &unit_quad, &transform);
		}

		// Render UI
		nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);

		// Cooperate with OS
		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	// Terminate application
	quit:
	lfr_term_graph(&graph);
	nk_glfw3_shutdown(&glfw);
	delete_mesh(&triangle);
	delete_mesh(&unit_quad);
	term_gl_app(win);
	return 0;
}


/**
Create an OpenGL mesh from vertices and indices.
**/
bool create_mesh(
		const vec3_t positions[], const vec3_t colors[], unsigned num_verticies,
		const unsigned indices[], unsigned num_indecies,
		gl_mesh_t *mesh) {
	assert(mesh);
	bool ok = true;

	glGenVertexArrays(1, &mesh->vao);
	glBindVertexArray(mesh->vao);
	CHECK_GL_OR("Create and bind mesh VAO", ok = false; goto exit;)

	// Put Position in a buffer
	glGenBuffers(1, &mesh->position_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->position_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3_t) * num_verticies, positions, GL_STATIC_DRAW);
	CHECK_GL_OR("Create mesh VBO for positions", ok = false; goto exit;)

	// Define how position is stored
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_t), (void*)(0));
	glEnableVertexAttribArray(0);
	CHECK_GL_OR("Assign position attribute (VAO->VBO)", ok = false; goto exit;)

	// Put Color in a buffer
	glGenBuffers(1, &mesh->color_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->color_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3_t) * num_verticies, colors, GL_STATIC_DRAW);
	CHECK_GL_OR("Create geometry color VBO", ok = false; goto exit;)

	// Define how color is stored
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_t), (void*)(0));
	glEnableVertexAttribArray(1);
	CHECK_GL_OR("Assign color attribute (VAO->VBO)", ok = false; goto exit;)

	// Put indices in an EBO
	mesh->num_indecies = num_indecies;
	glGenBuffers(1, &mesh->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * num_indecies, indices, GL_STATIC_DRAW);
	CHECK_GL_OR("Create geometry EBO (for indices)", ok = false; goto exit;)

	exit:
	if (!ok) {
		delete_mesh(mesh);
	}
	return ok;
}


/**
Render the given mesh with OpenGL.
**/
void render_mesh(const gl_program_t *program, const gl_mesh_t *mesh, const mat4_t *transform) {
	assert(program && mesh && transform);

	// Render mesh with the given program and the provided transform
	glUseProgram(program->shader_program);
	GLuint transform_loc = glGetUniformLocation(program->shader_program, "transform");
	glUniformMatrix4fv(transform_loc, 1, GL_TRUE, &transform->m[0]);
	glBindVertexArray(mesh->vao);
	glDrawElements(GL_TRIANGLES, mesh->num_indecies, GL_UNSIGNED_INT, 0);
	CHECK_GL("Render mesh");
}


/**
Delete (all existing parts of) given mesh.
**/
void delete_mesh(gl_mesh_t *mesh) {
	assert(mesh);

	if (mesh->ebo) { glDeleteBuffers(1, &mesh->ebo); mesh->ebo = 0;}
	if (mesh->position_vbo) { glDeleteBuffers(1, &mesh->position_vbo); mesh->position_vbo = 0;}
	if (mesh->color_vbo) { glDeleteBuffers(1, &mesh->color_vbo); mesh->color_vbo = 0;}
	if (mesh->vao) { glDeleteVertexArrays(1, &mesh->vao); mesh->vao = 0;}
	CHECK_GL("Delete mesh");
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
