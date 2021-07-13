/****
LFR Scripting "game" used to demonstrate how to integrate LFR with an existing application.
****/

// LIBC
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

// OpenGL
#include "basic_gl.h"


const char* vertex_shader_src =
	"#version 330 core\n"
	"layout (location = 0) in vec3 attr_pos;\n"
	"layout (location = 1) in vec3 attr_color;\n"
	"out vec3 var_color;\n"
	"void main()\n"
	"{\n"
	"   var_color = attr_color;\n"
	"   gl_Position = vec4(attr_pos.x, attr_pos.y, attr_pos.z, 1.0);\n"
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

typedef struct vec3_ { float x,y,z; } vec3_t;

bool create_mesh(
	const vec3_t [], const vec3_t [], unsigned num_verticies,
	const unsigned [], unsigned num_indecies,
	gl_mesh_t *);
void render_mesh(const gl_program_t *, const gl_mesh_t *);
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

/**
Application starting point.
**/
int main( int argc, char** argv) {
	GLFWwindow *win;

	// Initialize window (Full HD)
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

	// Keep the motor runnin
	while(!glfwWindowShouldClose(win)) {
		// Prepare rendering
		int width, height;
		glfwGetWindowSize(win, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(.75f, .55f, .75f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		CHECK_GL_OR("Prepare rendering", goto quit);

		// Render world
		render_mesh(&program, &triangle);

		// Cooperate with OS
		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	// Terminate application
	quit:
	glDeleteBuffers(1, &triangle.position_vbo);
	glDeleteBuffers(1, &triangle.color_vbo);
	glDeleteVertexArrays(1, &triangle.vao);
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
void render_mesh(const gl_program_t *program, const gl_mesh_t *mesh) {
	assert(program && mesh);

	// Render geometry
	glUseProgram(program->shader_program);
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
