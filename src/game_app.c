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
typedef struct vertex_ {
	float position[3];
	float color[3];
} vertex_t;

vertex_t triangle[3] = {
	{{0, +1, 0},{1,1,1}},
	{{+1, 0, 0},{1,1,1}},
	{{-1, 0, 0},{1,1,1}},
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
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	CHECK_GL_OR("Create and bind VAO", term_gl_app(win); return -20);

	// Put Position and Color in a buffer
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
	CHECK_GL_OR("Create geometry position Vertex Buffer Object", term_gl_app(win); return -21);

	// Define how position is stored
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)(offsetof(vertex_t, position)));
	glEnableVertexAttribArray(0);
	CHECK_GL_OR("Link position attribut (VAO->VBO)", term_gl_app(win); return -22);

	// Define how color is stored
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)(offsetof(vertex_t, color)));
	glEnableVertexAttribArray(1);
	CHECK_GL_OR("Link color attribut (VAO->VBO)", term_gl_app(win); return -23);

	// Keep the motor runnin
	while(!glfwWindowShouldClose(win)) {
		// Prepare rendering
		int width, height;
		glfwGetWindowSize(win, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(.75f, .55f, .75f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		CHECK_GL_OR("Prepare rendering", goto quit);

		// Render geometry
		glUseProgram(program.shader_program);
		CHECK_GL_OR("Use shader program", term_gl_app(win); return -110);
		glBindVertexArray(vao);
		CHECK_GL_OR("Bind VAO", term_gl_app(win); return -111);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		CHECK_GL_OR("Render triangle", term_gl_app(win); return -112);

		// Cooperate with OS
		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	// Terminate application
	quit:
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	term_gl_app(win);
	return 0;
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
