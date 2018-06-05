#include "test.h"

#define CORE_UTILS_IMPLEMENTATION
#include "core_utils.h"

#include "opengl.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>

#include "sdl.hpp"

#include <iostream>
#include <string>
//#include <array>
//#include <vector>

#include "shader.hpp"
#include "texture.hpp"
#include "camera.hpp"

#define WINDOW_HEADER ("")

#define SCREEN_WIDTH  (1280.0f)
#define SCREEN_HEIGHT (720.0f)
#define MS_PER_S (1000.0)
#define FRAMES_PER_SECOND (60.0)

//#define DISPLAY_FPS

#define GRID

int ignore_mouse_movement(void* unused, SDL_Event* event)
{
    return (event->type == SDL_MOUSEMOTION) ? 0 : 1;
}

//#define DEBUG_PRINT

void debug_print(const char* const in);
void debug_print(const char* const in) 
{
    #ifdef DEBUG_PRINT
    puts(in);
    #endif
}

#define FP_CAM
// #define FREE_CAM

#define MOUSE_ON


SDL_Window* window = NULL;

typedef void* (*Fn_MemoryAllocator)(size_t bytes);


// TEXTURES, GEOMETRY
typedef struct TextureData {
    Texture* ids;
    size_t count;
} TextureData;

void TextureData_init(TextureData* t, const size_t id_count) 
{
    t->ids = (Texture*)xmalloc(id_count * sizeof(t->ids));
    t->count = id_count;
    glGenTextures(t->count, t->ids);
}

void TextureData_init_inplace(TextureData* t, const size_t id_count, Texture* buffer)
{
    t->ids = buffer;
    t->count = id_count;
    glGenTextures(id_count, t->ids);
}

void TextureData_delete(TextureData* t)
{
    glDeleteTextures(t->count, t->ids);
    free(t);
}
void TextureData_delete_inplace(TextureData* t)
{
    glDeleteTextures(t->count, t->ids);
}

struct sceneData {
    glm::mat4 m_model;
    glm::mat4 m_view;
    glm::mat4 m_projection;
} scene;

template <typename T>
struct CappedArray {
    size_t cap;
    size_t count;
    T*  array;

    operator T*()
    {
        return this->array;
    }

    T& operator[](size_t i)
    {
        return this->array[i];
    }
     
    const T& operator[](size_t i) const 
    {
        return this->array[i];
    }

    inline size_t element_count() const
    {
        return this->count;
    }

    inline size_t element_size() const
    {
        return sizeof(T);
    }

    inline size_t size() const
    {
        return this->cap;
    }


    typedef CappedArray* iterator;
    typedef const CappedArray* const_iterator;
    iterator begin() { return &this->array[0]; }
    iterator end() { return &this->array[this->cap]; }
};

template <typename T>
void CappedArray_init(CappedArray<T>* arr, size_t cap, Fn_MemoryAllocator alloc) {
    arr->array = (T*)alloc(sizeof(T) * cap);
    arr->count = 0;
    arr->cap = cap;
}

template <typename T, size_t N>
struct CappedArrayStatic {
    size_t cap;
    size_t count;
    T array[N];

    operator T*()
    {
        return this->array;
    }

    T& operator[](size_t i)
    {
        return this->array[i];
    }
     
    const T& operator[](size_t i) const 
    {
        return this->array[i];
    }

    inline size_t element_count() const
    {
        return this->count;
    }

    inline size_t element_size() const
    {
        return sizeof(T);
    }

    inline size_t size() const
    {
        return N;
    }

    typedef CappedArrayStatic* iterator;
    typedef const CappedArrayStatic* const_iterator;
    iterator begin() { return &this->array[0]; }
    iterator end() { return &this->array[N]; }
};

// ATTRIBUTES AND VERTEX ARRAYS

typedef struct AttributeData {
    GLuint    index;
    GLint     size;
    GLenum    type;
    GLboolean normalized;
    GLsizei   stride;
    GLvoid*   pointer;
    GLchar*   name;
} AttributeData;

void AttributeData_init(
    AttributeData* a,
    GLuint index,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    GLvoid* pointer,
    GLchar* name    
) {
    a->index = index;
    a->size = size;
    a->type = type;
    a->normalized = normalized;
    a->stride = stride;
    a->pointer = pointer;
    a->name = name;
}

// #define VAO_ATTRIBUTE_MAIN_TEST_COUNT 3
// typedef struct _VertexArrayData {
//     VertexArray vao;
//     size_t attribute_count;
//     AttributeData attributes[VAO_ATTRIBUTE_MAIN_TEST_COUNT];
// } VertexArrayData;

// #define VAO_ATTRIBUTE_LIGHT_DAT_COUNT 3
// typedef struct _VertexArrayData {
//     VertexArray vao;
//     size_t attribute_count;
//     AttributeData attributes[VAO_ATTRIBUTE_MAIN_TEST_COUNT];
// } VertexArrayData;

// VERTEX BUFFERS


// struct MemoryAllocator {
//     void* type;
//     Fn_MemoryAllocator fn_alloc;
    
//     void* alloc(size_t bytes)
//     {
//         return Fn_MemoryAllocator(type, bytes);
//     }
// };

// void MemoryAllocator_init(MemoryAllocator* ma, void* type, Fn_MemoryAllocator* fn_alloc, Fn_MemoryAllocatorType_init fn_init, void* args)
// {
//     ma->fn_alloc = fn_alloc;
//     ma->type = alloc;
//     fn_init(fn_alloc, args);
// }

template <typename T>
struct Array {
    T* memory;
    size_t length;

    operator T*(void)
    {
        return this->memory;
    }

    inline T& operator[](size_t i)
    {
        return this->memory[i];
    }
     
    inline const T& operator[](size_t i) const 
    {
        return this->memory[i];
    }

    inline size_t size_buffer(void) const
    {
        return sizeof(T) * this->length;
    }

    inline size_t size_element(void) const
    {
        return sizeof(T);
    }

    typedef Array* iterator;
    typedef const Array* const_iterator;
    iterator begin() { return &this->memory[0]; }
    iterator end() { return &this->memory[this->length]; }
};


struct VertexBufferData {
    VertexBuffer vbo;
    ElementBuffer ebo;
    size_t    v_cap;
    size_t    v_count;
    size_t    i_cap;
    size_t    i_count;
    GLfloat*  vertices;
    GLuint*   indices;
};

struct Open_GL_Data {
    VertexBuffer vbo;
    ElementBuffer ebo;
    Array<GLfloat> vertices;
    Array<GLuint> indices;
};

struct Entity {
    glm::vec3 position;
    GLfloat rotation;
};

struct Player {
    Entity* base;
};

// struct VertexBufferDataAlt {
//     VertexBuffer vbo;
//     ElementBuffer ebo;
//     Buffer<GLfloat> vertices_lines;
//     Buffer<GLuint>  indices;
// } VertexBufferDataAlt;

// typedef VertexBufferData VBData;



void VertexBufferData_init(
    VertexBufferData* g,
    const size_t v_cap,
    const size_t i_cap,
    Fn_MemoryAllocator alloc_v,
    Fn_MemoryAllocator alloc_i
) {
    glGenBuffers(1, (GLBuffer*)&g->vbo);
    glGenBuffers(1, (GLBuffer*)&g->ebo);

    g->v_cap = v_cap;
    g->i_cap = i_cap;

    g->vertices = (GLfloat*)alloc_v(sizeof(GLfloat) * g->v_cap);
    g->indices  = (GLuint*)alloc_i(sizeof(GLuint) * g->i_cap);

    g->v_count = v_cap;
    g->i_count = i_cap;
}


#define STATIC_ARRAY_COUNT(array) (sizeof(array) / sizeof(*array))

void VertexBufferData_init_inplace(
    VertexBufferData* g,
    const size_t v_cap,
    GLfloat* vertices,
    const size_t i_cap,
    GLuint* indices
) {
    glGenBuffers(1, (GLBuffer*)&g->vbo);
    glGenBuffers(1, (GLBuffer*)&g->ebo);

    g->v_cap = v_cap;
    g->i_cap = i_cap;

    g->vertices = vertices;
    g->indices  = indices;

    g->v_count = v_cap;
    g->i_count = i_cap;
}

void VertexBufferData_delete(VertexBufferData* g)
{
    glDeleteBuffers(1, (GLBuffer*)&g->vbo);
    glDeleteBuffers(1, (GLBuffer*)&g->ebo);
    free(g->vertices);
    free(g->indices);
}
void VertexBufferData_delete_inplace(VertexBufferData* g)
{
    glDeleteBuffers(1, (GLBuffer*)&g->vbo);
    glDeleteBuffers(1, (GLBuffer*)&g->ebo);
}

void VertexBufferData_init_with_arenas(ArenaAllocator* v_arena, ArenaAllocator* i_arena, VertexBufferData* vbd, size_t v_count_elements, size_t i_count_elements) 
{
    VertexBufferData_init_inplace(
        vbd, 
        v_count_elements,
        (GLfloat*)ArenaAllocator_allocate(v_arena, v_count_elements * sizeof(GLfloat)),
        i_count_elements,
        (GLuint*)ArenaAllocator_allocate(i_arena, i_count_elements * sizeof(GLuint))
    );
}

void* GlobalArenaAlloc_vertex_attribute_data(size_t count) 
{
    return xmalloc(count * sizeof(GLfloat));
}
void* GlobalArenaAlloc_index_data(size_t count) 
{
    return xmalloc(count * sizeof(GLuint));    
}


typedef struct VertexAttributeArray {
    VertexArray vao;
    size_t stride;

    operator GLuint() { return vao; }

} VertexAttributeArray;

typedef VertexAttributeArray VAttribArr;

void VertexAttributeArray_init(VertexAttributeArray* vao, size_t stride) 
{
    glGenVertexArrays(1, &vao->vao);
    vao->stride = stride;
}
void VertexAttributeArray_delete(VertexAttributeArray* vao) 
{
    glDeleteVertexArrays(1, &vao->vao);
}

#define attribute_offsetof(offset) (GLvoid*)(offset * sizeof(GLfloat))

#define attribute_sizeof(vao) vao->stride * sizeof(GLfloat)

void gl_set_and_enable_vertex_attrib_ptr(GLuint index, GLint size, GLenum type, GLboolean normalized, size_t offset, VertexAttributeArray* va)
{
    glVertexAttribPointer(index, size, type, normalized, attribute_sizeof(va), attribute_offsetof(offset));            
    glEnableVertexAttribArray(index);
}

void gl_bind_buffers_and_upload_data(VertexBufferData* vbd, GLenum usage, size_t v_cap, size_t i_cap, GLintptr v_begin_offset = 0, GLintptr i_begin_offset = 0)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbd->vbo);
    glBufferData(GL_ARRAY_BUFFER, v_cap * sizeof(GLfloat), vbd->vertices + v_begin_offset, usage);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbd->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, i_cap * sizeof(GLuint), vbd->indices + i_begin_offset, usage);            
}

void gl_bind_buffers_and_upload_data(VertexBufferData* vbd, GLenum usage, GLintptr v_begin_offset = 0, GLintptr i_begin_offset = 0)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbd->vbo);
    glBufferData(GL_ARRAY_BUFFER, vbd->v_cap * sizeof(GLfloat), vbd->vertices + v_begin_offset, usage);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbd->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vbd->i_cap * sizeof(GLuint), vbd->indices + i_begin_offset, usage);           
}

void gl_bind_buffers_and_upload_sub_data(VertexBufferData* vbd)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbd->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vbd->v_count * sizeof(GLfloat), vbd->vertices);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbd->ebo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vbd->i_count * sizeof(GLuint), vbd->indices);
}


// COLLISION INFO
typedef struct CollisionStatus {
    bool      collided;
    glm::vec3 point;
} CollisionStatus;

void CollisionStatus_init(CollisionStatus* cs, const bool collided, glm::vec3 point)
{
    cs->collided = collided;
    cs->point = point;
}

typedef CollisionStatus (*Fn_CollisionHandler)(glm::vec3 incoming);

typedef struct Collider {
    glm::vec3 a;
    glm::vec3 b;
    Fn_CollisionHandler handler;
} Collider; 


struct GLData {
    VertexAttributeArray vao;
    VertexBufferData vbd;
};

void GLData_init(GLData* gl_data, size_t attribute_stride, const size_t v_cap, const size_t i_cap, Fn_MemoryAllocator alloc_v, Fn_MemoryAllocator alloc_i) 
{
    VertexAttributeArray_init(&gl_data->vao, attribute_stride);
    VertexBufferData_init(&gl_data->vbd, v_cap, i_cap, alloc_v, alloc_i);
}

void GLData_init_inplace(GLData* gl_data, size_t attribute_stride, const size_t v_cap, GLfloat* vertices, const size_t i_cap, GLuint* indices) 
{
    VertexAttributeArray_init(&gl_data->vao, attribute_stride);
    VertexBufferData_init_inplace(&gl_data->vbd, v_cap, vertices, i_cap, indices);    
}

inline void GLData_advance(GLData* const gl_data, const size_t i)
{
    gl_data->vbd.v_count += (i * gl_data->vao.stride);
    gl_data->vbd.i_count += i;
}

void GLData_delete(GLData* gl_data)
{
    VertexAttributeArray_delete(&gl_data->vao);
    VertexBufferData_delete(&gl_data->vbd);
}

void GLData_delete_inplace(GLData* gl_data)
{
    VertexAttributeArray_delete(&gl_data->vao);
    VertexBufferData_delete_inplace(&gl_data->vbd);    
}

// WORLD STATE
struct Room {
    VertexBufferData  geometry;
    Collider* collision_data;
    glm::mat4 matrix;
};

struct World {
    Room* rooms;
    glm::mat4 m_view;
    glm::mat4 m_projection;
};

struct GlobalData {
    SDL_GLContext context;
    TextureData textures;
};

GlobalData program_data;

#define GL_DRAW2D

#ifdef GL_DRAW2D
#define GL_DRAW2D_SIZE 2048
#include "gl_draw2d.h"
#endif

int main(int argc, char* argv[])
{
    c_func();

    // ArenaAllocator aa;

    // ArenaAllocator_init(&aa);
    // ArenaAllocator_delete(&aa);


    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "%s\n", "SDL could not initialize");
        return EXIT_FAILURE;
    }
    
    // MOUSE ///////////////////////////////////////////// 
    // hide the cursor
    SDL_ShowCursor(SDL_DISABLE);
    
    // ignore mouse movement events
    #ifndef MOUSE_ON
    SDL_SetEventFilter(ignore_mouse_movement, NULL); ///////////////////////////
    #endif
    // openGL initialization ///////////////////////////////////////////////////
    
    // if (argc >= 3) {
    //     SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, atoi(argv[1]));
    //     SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, atoi(argv[2]));
    // } else {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);        
    // }
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    // SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    // create the window
    if (NULL == (window = SDL_CreateWindow(
        WINDOW_HEADER,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI)))
    {
        fprintf(stderr, "Window could not be created\n");
        return EXIT_FAILURE;
    }

    //SDL_SetWindowFullscreen(window, true);
    
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if(!(IMG_Init(imgFlags) & imgFlags)) {
     	fprintf(stderr, "SDL_image could not initialize, SDL_image Error: %s\n", IMG_GetError());
        SDL_DestroyWindow(window);
        return EXIT_FAILURE;
    }
    
	program_data.context = SDL_GL_CreateContext(window);

	glewExperimental = GL_TRUE;
    glewInit();


    bool status;
    std::string glsl_perlin_noise = Shader::retrieve_src_from_file("shaders/perlin_noise.glsl", &status);
    if (!status) {
        return false;
    } 

    // SHADERS
    Shader shader_2d;
    shader_2d.load_from_file(
        "shaders/parallax/parallax_v2_vrt.glsl",
        "shaders/parallax/parallax_v2_frg.glsl",
        glsl_perlin_noise,
        glsl_perlin_noise
    );
    if (!shader_2d.is_valid()) {
        fprintf(stderr, "ERROR: shader_2d\n");
        return EXIT_FAILURE;
    }


    Shader shader_grid;
    shader_grid.load_from_file(
        "shaders/default_2d/grid_vrt.glsl",
        "shaders/default_2d/grid_frg.glsl"
    );
    if (!shader_grid.is_valid()) {
        fprintf(stderr, "ERROR: shader_grid\n");
        return EXIT_FAILURE;
    }
///////////////

    const GLfloat ASPECT = (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT;

    size_t STRIDE = 5;

// QUADS
    size_t POINTS_PER_QUAD = 4;
    size_t POINTS_PER_TRI = 3;
    size_t TRIS_PER_QUAD = 2;

    GLfloat wf = 1.0f;
    GLfloat hf = 1.0f * (512.0 / 360.0);
    const GLfloat OFF = 0.0f * wf * ASPECT;

    const GLfloat y_off_left = (16.0f / 45.0f);
    const GLfloat x_off_right = (512.0f / 640.0f);

    glm::vec2 tex_res(2048.0f, 1024.0f);

    glm::vec3 world_bguv_factor = glm::vec3(glm::vec2(1.0f) / tex_res, 1.0f);

    GLuint layers_per_row = (GLuint)(tex_res.x / SCREEN_WIDTH);
    // GLfloat x_off = (GLfloat)(GLdouble)(SCREEN_WIDTH / tex_res.x);
    // GLfloat y_off = (GLfloat)(GLdouble)(SCREEN_HEIGHT / tex_res.x);

    GLfloat x_ratio = SCREEN_WIDTH / tex_res.x;
    GLfloat y_ratio = SCREEN_HEIGHT / tex_res.y;

    GLfloat X_OFF = (tex_res.x - SCREEN_WIDTH) / 2.0f;
    GLfloat Y_OFF = (tex_res.y - SCREEN_HEIGHT) / 2.0f;

    GLfloat T[] = {
       0.0f - X_OFF,      tex_res.y - Y_OFF, 0.0f,    0.0f, 1.0f,    // top left
       0.0f - X_OFF,      0.0f - Y_OFF,      0.0f,    0.0f, 0.0f,    // bottom left
       tex_res.x - X_OFF, 0.0f - Y_OFF,      0.0f,    1.0f, 0.0f,    // bottom right
       tex_res.x - X_OFF, tex_res.y - Y_OFF, 0.0f,    1.0f, 1.0f,    // top right
    };

    GLuint TI[] = {
        0, 1, 2,
        2, 3, 0,
    };

// TOTAL ALLOCATION
    // const size_t BATCH_COUNT = 1024;
    // const size_t GUESS_VERTS_PER_DRAW = 4;

//////////////////////////////////////////////////

    VertexAttributeArray vao_2d2;
    VertexBufferData tri_data;

    VertexAttributeArray_init(&vao_2d2, STRIDE);

    glBindVertexArray(vao_2d2.vao);

        VertexBufferData_init_inplace(
            &tri_data, 
            STATIC_ARRAY_COUNT(T),
            T,
            STATIC_ARRAY_COUNT(TI),
            TI
        );


        gl_bind_buffers_and_upload_data(&tri_data, GL_STATIC_DRAW);
        // POSITION
        gl_set_and_enable_vertex_attrib_ptr(0, 3, GL_FLOAT, GL_FALSE, 0, &vao_2d2);
        // UV
        gl_set_and_enable_vertex_attrib_ptr(1, 2, GL_FLOAT, GL_FALSE, 3, &vao_2d2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    // GLData grid;
    // GLData_init_inplace(&grid, STRIDE, STATIC_ARRAY_COUNT(T) / 15, T, STATIC_ARRAY_COUNT(TI) / 15, TI);
    // glBindVertexArray(grid.vao);

    //     gl_bind_buffers_and_upload_data(&grid.vbd, GL_STATIC_DRAW, grid.vbd.v_cap, grid.vbd.i_cap, STATIC_ARRAY_COUNT(T) / 15, 0);

    //     // POSITION
    //     gl_set_and_enable_vertex_attrib_ptr(0, 3, GL_FLOAT, GL_FALSE, 0, &grid.vao);
    //     // UV
    //     gl_set_and_enable_vertex_attrib_ptr(1, 2, GL_FLOAT, GL_FALSE, 3, &grid.vao);

    //     glBindBuffer(GL_ARRAY_BUFFER, 0);

    // glBindVertexArray(0);

///////////////////////////////////////////////////////////////////////////////////////////////////////////

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
	
    glCullFace(GL_FRONT);

	// glEnable(GL_DEPTH_TEST);
 //    glDepthRange(0, 1);
 //    glDepthFunc(GL_LEQUAL);


    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ONE);
    //glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);

    
    printf("USING GL VERSION: %s\n", glGetString(GL_VERSION));

    glm::mat4 mat_ident(1.0f);
    glm::mat4 mat_projection = glm::ortho(
        0.0f, 
        1.0f * ((GLfloat)SCREEN_WIDTH), 
        1.0f * ((GLfloat)SCREEN_HEIGHT),
        0.0f,
        0.0f, 
        1.0f * 10.0f
    );
    //glm::mat4 mat_projection = glm::perspective(glm::radians(45.0f), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

//////////////////
// TEST INPUT
    glm::vec3 start_pos(0.0f, 0.0f, 1.0f);
    
    FreeCamera main_cam(start_pos);
    main_cam.orientation = glm::quat();
    main_cam.speed = 0.01 * 360.0f;
    // ViewCamera_init(
    //     &main_cam,
    //     start_pos,
    //     ViewCamera_default_speed,
    //     -1000.0f,
    //     1000.0f,
    //     0.0f,
    //     0.0f,
    //     0.0f,
    //     0.0f
    // );
    
    const Uint8* key_states = SDL_GetKeyboardState(NULL);

    const Uint8* up         = &key_states[SDL_SCANCODE_W];
    const Uint8* down       = &key_states[SDL_SCANCODE_S];
    const Uint8* left       = &key_states[SDL_SCANCODE_A];
    const Uint8* right      = &key_states[SDL_SCANCODE_D];
    const Uint8* rot_r      = &key_states[SDL_SCANCODE_RIGHT];
    const Uint8* rot_l      = &key_states[SDL_SCANCODE_LEFT];
//  const Uint8& up_right   = key_states[SDL_SCANCODE_E];
//  const Uint8& up_left    = key_states[SDL_SCANCODE_Q];
//  const Uint8& down_right = key_states[SDL_SCANCODE_X];
//  const Uint8& down_left  = key_states[SDL_SCANCODE_Z];
    const Uint8* forwards = &key_states[SDL_SCANCODE_UP];
    const Uint8* backwards = &key_states[SDL_SCANCODE_DOWN];

    const Uint8* reset = &key_states[SDL_SCANCODE_0];



    
#ifdef GRID
    const Uint8* toggle_grid = &key_states[SDL_SCANCODE_G];
    bool grid_held = false;
    bool grid_active = false;

    bool up_held = false;
    bool down_held = false;
#endif


    const f64 POS_ACC = 1.08;
    const f64 NEG_ACC = 1.0 / POS_ACC;

    // double up_acc = 110.0;
    // double down_acc = 110.0;
    // double left_acc = 110.0;
    // double right_acc = 110.0;
    // double forwards_acc = 110.0;
    // double backwards_acc = 110.0;

    double up_acc = 1.0;
    double down_acc = 1.0;
    double left_acc = 1.0;
    double right_acc = 1.0;
    double forwards_acc = 1.0;
    double backwards_acc = 1.0;

    double max_acc = 1000.0;


/////////////////
// MAIN LOOP
#ifdef DEBUG_PRINT
    glm::vec3 prev_pos(0.0);
#endif

    // Texture tex0;
    // if (GL_texture_gen_and_load_1(&tex0, "./textures/bg_test_3_3.png", GL_TRUE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE) == GL_FALSE) {
    //     return EXIT_FAILURE;
    // }


    Texture bgs[5];
    foreach(i, 5) {
        if (GL_FALSE == GL_texture_gen_and_load_1(
                &bgs[i], ("./textures/separate_test_2/" + std::to_string(i) + ".png").c_str(), 
                GL_TRUE, GL_REPEAT, GL_CLAMP_TO_EDGE
        )) {
            return EXIT_FAILURE;
        }
    }

    gl_get_errors();


    glUseProgram(shader_2d);

    //UniformLocation RES_LOC = glGetUniformLocation(shader_2d, "u_resolution");
    //UniformLocation COUNT_LAYERS_LOC = glGetUniformLocation(shader_2d, "u_count_layers");
    UniformLocation MAT_LOC = glGetUniformLocation(shader_2d, "u_matrix");
    //UniformLocation TIME_LOC = glGetUniformLocation(shader_2d, "u_time");
    UniformLocation CAM_LOC = glGetUniformLocation(shader_2d, "u_position_cam");
    //UniformLocation ASPECT_LOC = glGetUniformLocation(shader_2d, "u_aspect");

    const GLuint UVAL_COUNT_LAYERS = 5;

    //glUniform2fv(RES_LOC, 1, glm::value_ptr(glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT)));
    //glUniform1i(COUNT_LAYERS_LOC, UVAL_COUNT_LAYERS);
    //glUniform1f(ASPECT_LOC, (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT);
    // TEXTURE 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bgs[0]);
    glUniform1i(glGetUniformLocation(shader_2d, "tex0"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bgs[1]);
    glUniform1i(glGetUniformLocation(shader_2d, "tex1"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, bgs[2]);
    glUniform1i(glGetUniformLocation(shader_2d, "tex2"), 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, bgs[3]);
    glUniform1i(glGetUniformLocation(shader_2d, "tex3"), 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, bgs[4]);
    glUniform1i(glGetUniformLocation(shader_2d, "tex4"), 4);


    gl_get_errors();


    #ifdef GL_DRAW2D
    GLDraw2D gl_imm;
    if (!gl_imm.init(mat_projection)) {
        return EXIT_FAILURE;
    }
    #endif

    #ifdef GRID
    glUseProgram(shader_grid);

    UniformLocation COLOR_LOC_GRID = glGetUniformLocation(shader_grid, "u_color");
    glUniform4fv(COLOR_LOC_GRID, 1, glm::value_ptr(glm::vec4(0.25f, 0.25f, 0.25f, 0.5f)));

    UniformLocation SQUARE_PIXEL_LOC_GRID = glGetUniformLocation(shader_grid, "u_grid_square_pix");

    GLfloat grid_square_pixel_size = 16.0f;
    glUniform1f(SQUARE_PIXEL_LOC_GRID, tex_res.x / grid_square_pixel_size);

    UniformLocation MAT_LOC_GRID = glGetUniformLocation(shader_grid, "u_matrix");

    UniformLocation CAM_LOC_GRID = glGetUniformLocation(shader_grid, "u_position_cam");
    #endif


    size_t display_mode_count = 0;
    SDL_DisplayMode mode;

    if (SDL_GetDisplayMode(0, 0, &mode) != 0) {
        SDL_Log("SDL_GetDisplayMode failed: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    printf("REFRESH_RATE: %d\n", mode.refresh_rate);


    SDL_GL_SetSwapInterval(1);
    const double INTERVAL = MS_PER_S / mode.refresh_rate;

    bool keep_running = true;
    SDL_Event event;


    f64 frequency  = SDL_GetPerformanceFrequency();

    u64 t_now      = SDL_GetPerformanceCounter();
    u64 t_prev     = 0.0;
    u64 t_start    = t_now;
    u64 t_delta    = 0;
    
    f64 t_now_s         = (f64)t_now / frequency;
    f64 t_prev_s        = 0.0;
    f64 t_since_start_s = 0.0;
    f64 t_delta_s       = 0.0;

    //#define FPS_COUNT
    #ifdef FPS_COUNT
    f64 frame_time = 0.0;
    u32 frame_count = 0;
    u32 fps = 0;
    #endif

    while (keep_running) {
        t_prev = t_now;
        t_prev_s = t_now_s;

        t_now = SDL_GetPerformanceCounter();
        t_now_s = (f64)t_now / frequency;

        t_delta = (t_now - t_prev);
        t_delta_s = (f64)t_delta / frequency;

        f64 t_since_start = ((f64)(t_now - t_start)) / frequency;

        //std::cout << t_since_start << std::endl;

        // std::cout << "T_NOW: " << t_now << std::endl <<
        //              " T_DELTA: " << t_delta << std::endl <<
        //              " T_ELAPSED: " << (t_now - t_start) << std::endl <<
        //              " T_ELAPSED: " << ((double)(t_now - t_start) / 1000000000) << std::endl;

        // INPUT /////////////////////////////////
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                keep_running = false;
            }
        }

        #define CONTROLS

        #ifdef CONTROLS
        {

            double CHANGE = t_delta_s;
            main_cam.orientation = glm::quat();

            if (*up) {
                FreeCamera_process_directional_movement(&main_cam, Movement_Direction::UPWARDS, CHANGE * up_acc);
                up_acc *= POS_ACC;
                up_acc = glm::min(max_acc, up_acc);
            } else {
                if (up_acc > 1.0) {
                    FreeCamera_process_directional_movement(&main_cam, Movement_Direction::UPWARDS, CHANGE * up_acc);
                }
                up_acc = glm::max(1.0, up_acc * NEG_ACC);
            }
            if (*down) {
                FreeCamera_process_directional_movement(&main_cam, Movement_Direction::DOWNWARDS, CHANGE * down_acc);
                down_acc *= POS_ACC;
                down_acc = glm::min(max_acc, down_acc);
            } else {
                if (down_acc > 1.0) {
                    FreeCamera_process_directional_movement(&main_cam, Movement_Direction::DOWNWARDS, CHANGE * down_acc);
                } 
                down_acc = glm::max(1.0, down_acc * NEG_ACC);
            }

            if (*left) {
                FreeCamera_process_directional_movement(&main_cam, Movement_Direction::LEFTWARDS, CHANGE * left_acc);
                left_acc *= POS_ACC;
                left_acc = glm::min(max_acc, left_acc);
            } else {
                if (left_acc > 1.0) {
                    FreeCamera_process_directional_movement(&main_cam, Movement_Direction::LEFTWARDS, CHANGE * left_acc);
                }
                left_acc = glm::max(1.0, left_acc * NEG_ACC);
                left_acc = glm::min(max_acc, left_acc);
            }

            if (*right) {
                FreeCamera_process_directional_movement(&main_cam, Movement_Direction::RIGHTWARDS, CHANGE * right_acc);
                right_acc *= POS_ACC;
                right_acc = glm::min(max_acc, right_acc);
            } else {
                if (right_acc > 1.0) {
                    FreeCamera_process_directional_movement(&main_cam, Movement_Direction::RIGHTWARDS, CHANGE * right_acc);
                }
                right_acc = glm::max(1.0, right_acc * NEG_ACC);
            }

            // if (*forwards) {
            //     FreeCamera_process_directional_movement(&main_cam, Movement_Direction::FORWARDS, CHANGE * forwards_acc);
            //     forwards_acc *= POS_ACC;
            //     forwards_acc = glm::min(max_acc, forwards_acc);
            // } else {
            //     if (forwards_acc > 1.0) {
            //         FreeCamera_process_directional_movement(&main_cam, Movement_Direction::FORWARDS, CHANGE * forwards_acc);
            //     }
            //     forwards_acc = glm::max(1.0, forwards_acc * NEG_ACC);
            // }
            // if (*backwards) {
            //     FreeCamera_process_directional_movement(&main_cam, Movement_Direction::BACKWARDS, CHANGE * backwards_acc);
            //     backwards_acc *= POS_ACC;
            //     backwards_acc = glm::min(max_acc, backwards_acc);
            // } else {
            //     if (backwards_acc > 1.0) {
            //         FreeCamera_process_directional_movement(&main_cam, Movement_Direction::BACKWARDS, CHANGE * backwards_acc);
            //     } 
            //     backwards_acc = glm::max(1.0, backwards_acc * NEG_ACC);  
            // }

            if (*reset) {
                // FreeCamera_init(
                //     &main_cam,
                //     start_pos,
                //     ViewCamera_default_speed,
                //     -1000.0f,
                //     1000.0f,
                //     0.0f,
                //     0.0f,
                //     0.0f,
                //     0.0f
                // );
                main_cam.position = start_pos;
                main_cam.orientation = glm::quat();

                up_acc        = 1.0;
                down_acc      = 1.0;
                left_acc      = 1.0;
                right_acc     = 1.0;
                backwards_acc = 1.0;
                forwards_acc  = 1.0;
            }

            #ifdef MOUSE_ON
            i32 mouse_x = 0;
            i32 mouse_y = 0;

            SDL_GetMouseState(&mouse_x, &mouse_y);
            #endif
        }
        #endif



        //main_cam.rotate((GLfloat)curr_time / MS_PER_S, 0.0f, 0.0f, 1.0f);
    //////////////////
    // DRAW

                
        glClearColor(97.0 / 255.0, 201.0 / 255.0, 255.0 / 255.0, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader_2d);


        //glUniformMatrix4fv(MAT_LOC, 1, GL_FALSE, glm::value_ptr(ViewCamera_calc_view_matrix(&main_cam) * mat_ident));
        glUniformMatrix4fv(MAT_LOC, 1, GL_FALSE, glm::value_ptr(
            mat_projection
            /**FreeCamera_calc_view_matrix(&main_cam)*/
            /*glm::translate(mat_ident, glm::vec3(glm::sin(((double)t_now / frequency)), 0.0, 0.0)) * */
            /*glm::scale(mat_ident, glm::vec3(0.25, 0.25, 1.0))* */
                        )
        );

        //glUniform1f(TIME_LOC, t_since_start);

        glm::vec3 pos = main_cam.position;
        #ifdef DEBUG_PRINT

            if (pos.x != prev_pos.x || pos.y != prev_pos.y || pos.z != prev_pos.z) {
                std::cout << "VIEW_POSITION{x : " << pos.x << ", y : " << pos.y << ", z: " << pos.z << "}" << std::endl;
            }
            prev_pos.x = pos.x;
            prev_pos.y = pos.y;
            prev_pos.z = pos.z;
        #endif

        glUniform3fv(CAM_LOC, 1, glm::value_ptr(pos * world_bguv_factor));
        glEnable(GL_DEPTH_TEST);
        //glClear(GL_DEPTH_BUFFER_BIT);
        // glDepthRange(0, 1);
        
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glDisable(GL_DEPTH_TEST);
        
        glBindVertexArray(vao_2d2.vao);
        glDrawElements(GL_TRIANGLES, tri_data.i_count, GL_UNSIGNED_INT, 0);
        //glBindVertexArray(0);

        #ifdef GL_DRAW2D

        glEnable(GL_DEPTH_TEST);
        glDepthRange(0, 1);
        glClear(GL_DEPTH_BUFFER_BIT);


        gl_imm.begin();

            //gl_imm.transform_matrix = FreeCamera_calc_view_matrix(&main_cam);
            gl_imm.transform_matrix = glm::mat4(1.0f);

            // gl_imm.draw_type = GL_LINES;
            // gl_imm.color = Color::RED;
            // gl_imm.vertex({0.5, 0.0, -1.0});
            // gl_imm.vertex({1.0, 1.0, -1.0});
            
            // gl_imm.draw_type = GL_LINES;
            // gl_imm.color = Color::GREEN;
            // gl_imm.line({0.0, 0.0, -5.0}, {1.0, 1.0, -5.0});

            // gl_imm.color = Color::GREEN;
            // gl_imm.circle(0.25, {0.0, 0.0, 0.0});

            gl_imm.draw_type = GL_TRIANGLES;

            GLfloat CX = (SCREEN_WIDTH / 2.0f);
            GLfloat CY = 384.0f;
            glm::mat4 cam = FreeCamera_calc_view_matrix(&main_cam);

            gl_imm.color = glm::vec4(252.0f / 255.0f, 212.0f / 255.0f, 64.0f / 255.0f, 1.0f);

            gl_imm.transform_matrix = cam;

            gl_imm.circle(90.0f, {CX, CY, -1.0});

            gl_imm.color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            gl_imm.transform_matrix = glm::translate(cam, glm::vec3(CX - 27.0f, CY - 25.0f, 0.0f));
            gl_imm.circle(10.0f, {0.0f, 0.0f, 0.0f});

            gl_imm.color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            gl_imm.transform_matrix = glm::translate(cam, glm::vec3(CX + 27.0f, CY - 25.0f, 0.0f));
            gl_imm.circle(10.0f, {0.0f, 0.0f, 0.0f});

            
            gl_imm.draw_type = GL_LINES;

            #define BASE_TILE_SIZE (128.0f)
            #define TILE_SCALE (2.0f)

            CX = 5.0f / TILE_SCALE;
            CY = 3.0f / TILE_SCALE;
            
            glm::mat4 model(1.0f);
            model = glm::scale(model, glm::vec3(BASE_TILE_SIZE * TILE_SCALE, BASE_TILE_SIZE * TILE_SCALE, 1.0f));
            model = glm::translate(model, glm::vec3({CX, CY, 0.0}));
            model = glm::rotate(model, (GLfloat)t_since_start, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::translate(model, glm::vec3({-CX, -CY, 0.0}));
            

            gl_imm.transform_matrix = cam * model;

            gl_imm.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            {
                GLfloat off = 2.0f;
                // horizontal
                gl_imm.line(glm::vec3(CX - off, CY - off, 0.0f), glm::vec3(CX + off, CY - off, 0.0f));
                gl_imm.line(glm::vec3(CX - off, CY + off, 0.0f), glm::vec3(CX + off, CY + off, 0.0f));
                // vertical
                gl_imm.line(glm::vec3(CX - off, CY - off, 0.0f), glm::vec3(CX - off, CY + off, 0.0f));
                gl_imm.line(glm::vec3(CX + off, CY - off, 0.0f), glm::vec3(CX + off, CY + off, 0.0f));
            }   


            
            // gl_imm.color = Color::BLUE;
            // gl_imm.transform_matrix = glm::translate(gl_imm.transform_matrix, glm::vec3(-0.5f, -0.5f, 0.0f));
            // gl_imm.vertex({0.0, 0.0, 0.0});
            // gl_imm.vertex({1.0, 0.0, 0.0});
            // gl_imm.vertex({0.5, glm::sqrt(3.0f) / 2.0f, 0.0});

            // gl_imm.color = Color::RED;
            // gl_imm.transform_matrix = glm::translate(gl_imm.transform_matrix, glm::vec3(-0.5f + glm::sin(t_since_start), -0.5f, 0.0f));
            // gl_imm.vertex({0.0, 0.0, -0.5});
            // gl_imm.vertex({1.0, 0.0, -0.5});
            // gl_imm.vertex({0.5, glm::sqrt(3.0f) / 2.0f, -0.5});


        gl_imm.end();

        #endif


        #ifdef GRID

        if (*toggle_grid) {
            if (!grid_held) {
                grid_active = !grid_active;
                grid_held = true;
            }
        } else {
            grid_held = false;
        }

        if (grid_active) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glClear(GL_DEPTH_BUFFER_BIT);


            glUseProgram(shader_grid);


            if (*forwards) {
                if (!up_held) {
                    up_held = true;

                    grid_square_pixel_size *= 2;
                    grid_square_pixel_size = glm::clamp(grid_square_pixel_size, 4.0f, 64.0f);

                    glUniform1f(SQUARE_PIXEL_LOC_GRID, tex_res.x / grid_square_pixel_size);

                }
            } else {
                up_held = false;
            }
            if (*backwards) {
                if (!down_held) {
                    down_held = true;

                    grid_square_pixel_size /= 2;
                    grid_square_pixel_size = glm::clamp(grid_square_pixel_size, 4.0f, 64.0f);

                    glUniform1f(SQUARE_PIXEL_LOC_GRID, tex_res.x / grid_square_pixel_size);
                }
            } else {
                down_held = false;
            }


            glUniformMatrix4fv(MAT_LOC_GRID, 1, GL_FALSE, glm::value_ptr(
                    mat_projection
                )
            );

            glUniform3fv(CAM_LOC_GRID, 1, glm::value_ptr(pos));


            glBindVertexArray(vao_2d2.vao);
            glDrawElements(GL_TRIANGLES, tri_data.i_count, GL_UNSIGNED_INT, 0);

            glDisable(GL_BLEND);
        }

        #endif

        SDL_GL_SwapWindow(window);

        #ifdef FPS_COUNT
        frame_count += 1;
        if (t_now_s - frame_time > 1.0) {
            fps = frame_count;
            frame_count = 0;
            frame_time = t_now_s;
            printf("%f\n", (double)fps);
        }
        #endif
    //////////////////
    }
    
    VertexAttributeArray_delete(&vao_2d2);
    VertexBufferData_delete_inplace(&tri_data);
    #ifdef GL_DRAW2D
        gl_imm.free();
    #endif
    SDL_GL_DeleteContext(program_data.context);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return EXIT_SUCCESS;
}
