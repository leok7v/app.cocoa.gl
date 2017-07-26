#include "app.h"
#include "vc3d.h"
#include <OpenGL/gl3.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

BEGIN_C

typedef struct vc3d_s {
    app_t* app;
    GLuint program_id;
} vc3d_t_;

vc3d_t vc3d_create(app_t* app) {
    vc3d_t_* vc = (vc3d_t_*)calloc(sizeof(vc3d_t_), 1); // zeroed out
    if (vc != null) {
        vc->app = app;
    }
    return vc;
}

void vc3d_shape(vc3d_t vc, int x, int y, int w, int h) {
    glViewport(0, 0, w, h);
}

void vc3d_input(vc3d_t p, input_event_t* e) {
    // process mouse and keyboard input events here
}

const char* vertex_shader_code =
    "#version 330 core\n"
    "layout(location = 0) in vec3 vertexPosition_modelspace;\n"
    "void main() {\n"
    "    gl_Position.xyz = vertexPosition_modelspace;\n"
    "    gl_Position.w = 1.0;\n"
    "}";
const char* fragment_shader_code =
    "#version 330 core\n"
    "out vec3 color;\n"
    "void main() {\n"
    "    color = vec3(1,0,0);\n"
    "}";

void compile_shaders(vc3d_t_* vc) {
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    GLint Result = GL_FALSE;
    int InfoLogLength;
    glShaderSource(vertex_shader_id, 1, &vertex_shader_code , NULL);
    glCompileShader(vertex_shader_id);
    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        char message[InfoLogLength + 1];
        glGetShaderInfoLog(vertex_shader_id, InfoLogLength, NULL, message);
        printf("%s\n", message);
        exit(1);
    }
    glShaderSource(fragment_shader_id, 1, &fragment_shader_code , NULL);
    glCompileShader(fragment_shader_id);
    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        char message[InfoLogLength + 1];
        glGetShaderInfoLog(fragment_shader_id, InfoLogLength, NULL, message);
        printf("%s\n", message);
    }
    vc->program_id = glCreateProgram();
    glAttachShader(vc->program_id, vertex_shader_id);
    glAttachShader(vc->program_id, fragment_shader_id);
    glLinkProgram(vc->program_id);
    // Check the program
    glGetProgramiv(vc->program_id, GL_LINK_STATUS, &Result);
    glGetProgramiv(vc->program_id, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        char message[InfoLogLength+1];
        glGetProgramInfoLog(vc->program_id, InfoLogLength, NULL, &message[0]);
        printf("%s\n", message);
    }
    glDetachShader(vc->program_id, vertex_shader_id);
    glDetachShader(vc->program_id, fragment_shader_id);
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
}

static void build_scene() {
    GLuint vertex_array_id;
    glGenVertexArrays(1, &vertex_array_id);
    glBindVertexArray(vertex_array_id);
    // An array of 3 vectors which represents 3 vertices
    static const GLfloat g_vertex_buffer_data[] = {
       -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        0.0f,  1.0f, 0.0f,
    };
    // This will identify our vertex buffer
    GLuint vertex_buffer;
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertex_buffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glDisableVertexAttribArray(0);
}

void vc3d_paint(vc3d_t p, int x, int y, int w, int h) {
    vc3d_t_* vc = (vc3d_t_*)p;
    if (vc->program_id == 0) {
        compile_shaders(vc);
        build_scene();
    }
    glClearColor(0.3, 0.4, 0.4, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(vc->program_id);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,         // attribute 0 (must match the layout in the shader)
                          3,         // size
                          GL_FLOAT,  // type
                          GL_FALSE,  // not normalized into [-1.0..1.0] range
                          0,         // stride
                          (void*)0); // array buffer offset
    glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
    glDisableVertexAttribArray(0);
}

void vc3d_destroy(vc3d_t p) {
    vc3d_t_* vc = (vc3d_t_*)p;
    glDeleteProgram(vc->program_id);
    free(vc);
}

END_C
