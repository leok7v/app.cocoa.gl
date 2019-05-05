#include "app.h"
#include "vc3d.h"
#include "math4x4.h"
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>

BEGIN_C

typedef struct vc3d_s {
    app_t* app;
    GLuint program_id;
    int w;
    int h;
    mat4x4f_t model;
    mat4x4f_t view;
    mat4x4f_t projection;
} vc3d_t_;

vc3d_t vc3d_create(app_t* app) {
    vc3d_t_* vc = (vc3d_t_*)calloc(sizeof(vc3d_t_), 1); // zeroed out
    if (vc != null) {
        vc->app = app;
        memcpy(vc->model, identity_4x4f, sizeof(vc->model));
        memcpy(vc->view, identity_4x4f, sizeof(vc->view));
        memcpy(vc->projection, identity_4x4f, sizeof(vc->projection));
    }
    return vc;
}

static void orthographic_projection(GLfloat* m, float left, float right, float bottom, float top, float near, float far) {
    // see: http://en.wikipedia.org/wiki/Orthographic_projection_(geometry)
    const float inv_z = 1.0f / (far - near);
    const float inv_y = 1.0f / (top - bottom);
    const float inv_x = 1.0f / (right - left);
    m[0] = 2 * inv_x; m[1] = 0; m[2] = 0; m[3] = 0;
    m[4] = 0; m[5] = 2 * inv_y; m[6] = 0; m[7] = 0;
    m[8] = 0; m[9] = 0; m[10] = -2 * inv_z; m[11] = 0;
    m[12] = -(right + left) * inv_x; m[13] = -(top + bottom) * inv_y; m[14] = -(far + near) * inv_z; m[15] = 1;
}

static void viewport(GLfloat* m, float w, float h, float near, float far) {
    memcpy(m, identity_4x4f, sizeof(identity_4x4f));
/*
    m[0] = w / 2;
    m[5] = h / 2;
    m[10] = (far - near) / 2;
    m[12] = w / 2;
    m[13] = h / 2;
    m[14] = (far + near) / 2;
*/
}

#define check_gl(call) call; { \
    int _gl_error_ = glGetError(); \
    if (_gl_error_ != 0) { printf("%s(%d): %s %s glError=%d\n", __FILE__, __LINE__, __func__, #call, _gl_error_); } \
}

void vc3d_shape(vc3d_t vc, int x, int y, int w, int h) {
    vc3d_t_* v = (vc3d_t_*)vc;
    v->w = w;
    v->h = h;
    const float far = -1;
    const float near = 1;
    orthographic_projection(v->projection, 0, w, h, 0, -1, 1);
    viewport(v->view, w, h, near, far);
    check_gl(glViewport(0, 0, w, h));
    printf("glViewport(0, 0, %d, %d)\n", w, h);
//  disabled because I failed to make it communicate with glClear()
//  glScissor(0, 0, w, h); // https://www.khronos.org/opengl/wiki/GLAPI/glScissor affects glClear
}

void vc3d_input(vc3d_t p, input_event_t* e) {
    // process mouse and keyboard input events here
}

// https://www.khronos.org/opengl/wiki/Vertex_Post-Processing
// The clip-space positions returned from the clipping stage are transformed into normalized device coordinates (NDC) via this equation:
// xyx_ndc  = xyz / w
// and now obsolete: http://www.glprogramming.com/red/chapter03.html#name4

const char* vertex_shader_code =
    "#version 330 core\n"
    "layout(location = 0) in vec3 vertex_position_in_model_space;\n"
//  "uniform mat4 model;\n"
//  "uniform mat4 view;\n"
//  "uniform mat4 projection;\n"
    "uniform mat4 mvp;\n" // premultiplied
    "void main() {\n"
//  "   // Order of multiplication is important. Forth coordinate is `w`\n"
//  "   gl_Position = projection * view * model * vec4(vertex_position_in_model_space, 1.0);\n"
    "   gl_Position = mvp * vec4(vertex_position_in_model_space, 1.0);\n"
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
    glShaderSource(vertex_shader_id, 1, &vertex_shader_code , null);
    glCompileShader(vertex_shader_id);
    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        char message[InfoLogLength + 1];
        glGetShaderInfoLog(vertex_shader_id, InfoLogLength, null, message);
        printf("%s\n", message);
        exit(1);
    }
    glShaderSource(fragment_shader_id, 1, &fragment_shader_code , null);
    glCompileShader(fragment_shader_id);
    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        char message[InfoLogLength + 1];
        glGetShaderInfoLog(fragment_shader_id, InfoLogLength, null, message);
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
        glGetProgramInfoLog(vc->program_id, InfoLogLength, null, &message[0]);
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
/*
    static const GLfloat g_vertex_buffer_data[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        0.0f,  1.0f, 0.0f,
    };
*/
    static const GLfloat g_vertex_buffer_data[] = {
       -1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
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
    check_gl(glViewport(0, 0, vc->w, vc->h))
    check_gl(glClearColor(0.3, 0.4, 0.4, 1.0))
    check_gl(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE))
    check_gl(glDepthMask(GL_TRUE))
    check_gl(glDrawBuffer(GL_BACK));
    check_gl(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
    if (vc->program_id == 0) {
        glDisable(GL_SCISSOR_TEST);
        compile_shaders(vc);
        build_scene();
    }
    check_gl(glUseProgram(vc->program_id))
    // create transformations
    mat4x4f_t mv;  // model * view
    multiply_4x4f(mv, vc->model, vc->view);
    mat4x4f_t mvp = IDENTITY_MATRIX_4x4F; // model * view * projection
    multiply_4x4f(mvp, mv, vc->projection);
    memcpy(mvp, identity_4x4f, sizeof(mvp)); // DEBUG
/*
    projection = perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    view       = translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    model      = translate(model, cubePositions[i]);
    static count;
    float angle = 20.0f * count++;
    rotate(model, radians(angle), vec3(1.0f, 0.3f, 0.5f));
*/
    check_gl(GLint mvp_matrix_uniform = glGetUniformLocation(vc->program_id, "mvp"))
    check_gl(glUniformMatrix4fv(mvp_matrix_uniform, 1, GL_FALSE, mvp));
    check_gl(glEnableVertexAttribArray(0))
    check_gl(glVertexAttribPointer(0,         // attribute 0 (must match the layout in the shader)
                          3,         // size
                          GL_FLOAT,  // type
                          GL_FALSE,  // not normalized into [-1.0..1.0] range
                          0,         // stride
                          (void*)0)) // array buffer offset
    check_gl(glDrawArrays(GL_TRIANGLES, 0, 3)) // Starting from vertex 0; 3 vertices total -> 1 triangle
    check_gl(glDisableVertexAttribArray(0))
}

void vc3d_destroy(vc3d_t p) {
    vc3d_t_* vc = (vc3d_t_*)p;
    glDeleteProgram(vc->program_id);
    free(vc);
}

END_C
