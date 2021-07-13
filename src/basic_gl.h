#ifndef BASIC_GL_H
#define BASIC_GL_H

// OpenGl et.al.
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Table of Content
bool init_gl_app(int, int, GLFWwindow **);
void term_gl_app(GLFWwindow *);
bool check_gl(const char* hint, int line);

#define CHECK_GL(hint) check_gl(hint, __LINE__)
#define CHECK_GL_OR(hint, bail) if(!check_gl(hint, __LINE__)) { bail; }

//// Content ////

/**
Init application, giving us a GLFW window to render to with fairly modern OpenGL.
**/
bool init_gl_app(int width, int heigt, GLFWwindow **win) {
	void error_callback_(int error, const char* description);
	void describe_gl_driver_();

	assert(win && *win == NULL);

	// Start up GLFW
	glfwSetErrorCallback(error_callback_);
	if (!glfwInit()) {
		fprintf(stderr, "Failed to init GLFW!\n");
		return false;
	}

	// Create window with (recent) OpenGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	*win = glfwCreateWindow(width, heigt, "Minimal GLFW window", NULL, NULL);
	if (!*win) {
		fprintf(stderr, "Failed to create GLFW window!\n");
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*win);
	glfwSwapInterval(1);
	glewInit();
	describe_gl_driver_();

	// All went well
	return true;
}


/**
GLFW Callback (internal) - Print GLFW errors to stderr.
**/
void error_callback_(int error, const char* description) {
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
		fprintf(stderr,
			"OpenGL failed to '%s' due to error code [0x%x] after line [%d].\n",
			hint, e, line);
		e = glGetError();
	}
	return ok;
}


/**
Print information about OpenGL to stdout.
**/
void describe_gl_driver_() {
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
void term_gl_app(GLFWwindow *win) {
	if (win) { glfwDestroyWindow(win); }
	glfwTerminate();
}


#else
// This header should only be included in the app main C-file.
#warn "Header included more than once."
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
