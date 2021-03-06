/**
 * OpenGL 4 - Example 30
 *
 * @author	Norbert Nopper norbert@nopper.tv
 *
 * Homepage: http://nopper.tv
 *
 * Copyright Norbert Nopper
 */

#include <stdio.h>

#include "GL/glus.h"

#define WIDTH 640

#define HEIGHT 480

#define PADDING 1

/**
 * The used shader program.
 */
static GLUSshaderprogram g_program;

/**
 * The location of the texture in the shader program.
 */
static GLint g_textureLocation;

/**
 * The empty VAO.
 */
static GLuint g_vao;

/**
 * The texture.
 */
static GLuint g_texture;

/**
 * Width of the image.
 */
static GLuint g_imageWidth = 640;

/**
 * Height of the image.
 */
static GLuint g_imageHeight = 480;

/**
 * Size of the work goups.
 */
static GLuint g_localSize = 16;

//

/**
 * The used compute shader program.
 */
static GLUSshaderprogram g_computeProgram;

//

static GLuint g_directionSSBO;

static GLfloat g_directionBuffer[WIDTH * HEIGHT * (3 + PADDING)];

//

static GLuint g_positionSSBO;

static GLfloat g_positionBuffer[WIDTH * HEIGHT * 4];

GLUSboolean init(GLUSvoid)
{
	GLUStextfile vertexSource;
	GLUStextfile fragmentSource;
	GLUStextfile computeSource;

	glusLoadTextFile("../Example30/shader/fullscreen.vert.glsl", &vertexSource);
	glusLoadTextFile("../Example30/shader/texture.frag.glsl", &fragmentSource);

	glusBuildProgramFromSource(&g_program, (const GLchar**)&vertexSource.text, 0, 0, 0, (const GLchar**)&fragmentSource.text);

	glusDestroyTextFile(&vertexSource);
	glusDestroyTextFile(&fragmentSource);

	glusLoadTextFile("../Example30/shader/raytrace.comp.glsl", &computeSource);

	glusBuildComputeProgramFromSource(&g_computeProgram, (const GLchar**)&computeSource.text);

	glusDestroyTextFile(&computeSource);

	//

	// Retrieve the uniform locations in the program.
	g_textureLocation = glGetUniformLocation(g_program.program, "u_texture");

	//

	// Generate and bind a texture.
	glGenTextures(1, &g_texture);
	glBindTexture(GL_TEXTURE_2D, g_texture);

	// Create an empty image.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	// Setting the texture parameters.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);

	//
	//

	glUseProgram(g_program.program);

	glGenVertexArrays(1, &g_vao);
	glBindVertexArray(g_vao);

	//

	// Also bind created texture ...
	glBindTexture(GL_TEXTURE_2D, g_texture);

	// ... and bind this texture as an image, as we will write to it. see binding = 0 in shader.
	glBindImageTexture(0, g_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

	// ... and as this is texture number 0, bind the uniform to the program.
	glUniform1i(g_textureLocation, 0);

	//
	//

	glUseProgram(g_computeProgram.program);

	//

	// Generate the ray directions depending on FOV, width and height.
	if (!glusRaytracePerspectivef(g_directionBuffer, PADDING, 30.0f, WIDTH, HEIGHT))
	{
		printf("Error: Could not create direction buffer.\n");

		return GLUS_FALSE;
	}

	// Compute shader will use these textures just for input.
	glusRaytraceLookAtf(g_positionBuffer, g_directionBuffer, g_directionBuffer, PADDING, WIDTH, HEIGHT, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);

	//
	// Buffers with the initial ray position and direction.
	//

	glGenBuffers(1, &g_directionSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_directionSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, WIDTH * HEIGHT * (3 + PADDING) * sizeof(GLfloat), g_directionBuffer, GL_STATIC_READ);
	// see binding = 1 in the shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_directionSSBO);

	//

	glGenBuffers(1, &g_positionSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_positionSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, WIDTH * HEIGHT * 4 * sizeof(GLfloat), g_positionBuffer, GL_STATIC_READ);
	// see binding = 2 in the shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_positionSSBO);

	return GLUS_TRUE;
}

GLUSvoid reshape(GLUSint width, GLUSint height)
{
	glViewport(0, 0, width, height);
}

GLUSboolean update(GLUSfloat time)
{
	// Switch to the compute shader.
	glUseProgram(g_computeProgram.program);

	// Create threads depending on width, height and block size. In this case we have 1200 threads.
	glDispatchCompute(g_imageWidth / g_localSize, g_imageHeight / g_localSize, 1);

	// Switch back to the render program.
	glUseProgram(g_program.program);

	// Here we full screen the output of the compute shader.
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	return GLUS_TRUE;
}

GLUSvoid terminate(GLUSvoid)
{
	glBindTexture(GL_TEXTURE_2D, 0);

	if (g_texture)
	{
		glDeleteTextures(1, &g_texture);

		g_texture = 0;
	}

	//

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	if (g_directionSSBO)
	{
		glDeleteBuffers(1, &g_directionSSBO);

		g_directionSSBO = 0;
	}

	if (g_positionSSBO)
	{
		glDeleteBuffers(1, &g_positionSSBO);

		g_positionSSBO = 0;
	}

	//

	glBindVertexArray(0);

	if (g_vao)
	{
		glDeleteVertexArrays(1, &g_vao);

		g_vao = 0;
	}

	glUseProgram(0);

	glusDestroyProgram(&g_program);

	glusDestroyProgram(&g_computeProgram);
}

int main(int argc, char* argv[])
{
	glusInitFunc(init);

	glusReshapeFunc(reshape);

	glusUpdateFunc(update);

	glusTerminateFunc(terminate);

	glusPrepareContext(4, 3, GLUS_FORWARD_COMPATIBLE_BIT);

	// Again, makes programming for this example easier.
	glusPrepareNoResize(GLUS_TRUE);

	if (!glusCreateWindow("GLUS Example Window", WIDTH, HEIGHT, 0, 0, GLUS_FALSE))
	{
		printf("Could not create window!\n");
		return -1;
	}

	glusRun();

	return 0;
}
