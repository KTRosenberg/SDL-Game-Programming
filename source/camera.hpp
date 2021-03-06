#ifndef CAMERA_HPP
#define CAMERA_HPP

#if !(UNITY_BUILD)
#include "opengl.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>

#include <limits>

#include "core_utils.h"
#endif

#define MAX_BOUND 89.0f
#define MIN_BOUND -89.0f
#define MIN_MOUSE_ZOOM 1.0f
#define MAX_MOUSE_ZOOM 45.0f

static const GLfloat ViewCamera_default_speed = 4.0f;

typedef struct ViewCamera {
    Vec3      position;
    GLfloat   speed;
    Mat4      matrix;
    GLfloat   min_x;
    GLfloat   max_x;
    GLfloat   min_y;
    GLfloat   max_y;
    GLfloat   min_z;
    GLfloat   max_z;
} ViewCamera;

void ViewCamera_init(
    ViewCamera* view,
    Vec3 position_start,
    GLfloat speed,
    GLfloat min_z = NEGATIVE_INFINITY,
    GLfloat max_z = POSITIVE_INFINITY,
    GLfloat min_x = 0.0f,
    GLfloat max_x = POSITIVE_INFINITY,
    GLfloat min_y = 0.0f,
    GLfloat max_y = POSITIVE_INFINITY
);

void ViewCamera_process_directional_movement(ViewCamera* view, MOVEMENT_DIRECTION direction, GLfloat delta_time);
Mat4 ViewCamera_calc_view_matrix(ViewCamera* view);
////////////////////////////////////////////////////////////////////////////////////////////

enum class Camera_Movement : unsigned char 
{
    FORWARDS,
    BACKWARDS,
    LEFTWARDS,
    RIGHTWARDS,
    UPWARDS,
    DOWNWARDS
};

namespace camera_defaults {
    const GLfloat YAW = -90.0f;
    const GLfloat PITCH = 0.0f;
    const GLfloat SPEED = 4.0f;
    const GLfloat SENSITIVITY = 0.25f;
    const GLfloat ZOOM = 45.0f;
}

struct Camera {
    // attributes
    Vec3 pos;
    Vec3 front;
    Vec3 up;
    Vec3 right;
    Vec3 world_up;
    // Euler angles
    GLfloat yaw;
    GLfloat pitch;
    // camera options
    GLfloat movement_speed;
    GLfloat mouse_sensitivity;
    GLfloat zoom;

    GLfloat minZ;
    GLfloat maxZ;
    
    // constructor (vector values)
    Camera(
        Vec3 pos = Vec3(0.0f, 0.0f, 0.0f),
        Vec3 up = Vec3(0.0f, 1.0f, 0.0f),
        GLfloat yaw = camera_defaults::YAW,
        GLfloat pitch = camera_defaults::PITCH
    );
    
    // constructor (scalar values)
    Camera(
        GLfloat pos_x, GLfloat pos_y, GLfloat pos_z,
        GLfloat up_x, GLfloat up_y, GLfloat up_z,
        GLfloat yaw = camera_defaults::YAW,
        GLfloat pitch = camera_defaults::PITCH
    );
    
    Mat4 get_view_matrix(void);

    void process_directional_movement(Camera_Movement direction, GLfloat delta_time);
    void process_mouse_movement(GLfloat x_offset, GLfloat y_offset, GLboolean constrain_pitch = GL_TRUE);
    void process_mouse_scroll(GLfloat y_offset);
private:
    void update_camera_vectors(void);
};

// based on code by Laurie Bradshaw, from comment in https://learnopengl.com/#!Getting-started/Camera
struct FreeCamera {
	Vec3 position;
	Quat orientation;
    Mat4 matrix;
    GLfloat speed;

    Vec2 target;

    Vec2 offset;

    Vec2 target_diff;

    bool is_catching_up;

    float32 scale;

	//FreeCamera (void) = default;
	//FreeCamera (const FreeCamera &) = default;

	//FreeCamera (const Vec3 &position) : position(position) { is_catching_up = false; }
	//FreeCamera (const Vec3 &position, const Quat &orientation) : position(position), orientation(orientation) {}

	//FreeCamera  &operator=(const FreeCamera&) = default;

	//Mat4 get_view_matrix(void) const { return glm::translate(glm::mat4_cast(orientation), -position); }

	void translate(const Vec3 &v) { position += v * orientation; }
	void translate(float x, float y, float z) { position += Vec3(x, y, z) * orientation; }

	void rotate(float angle, const Vec3& axis) { orientation *= glm::angleAxis(angle, axis * orientation); }
	void rotate(float angle, float x, float y, float z) { orientation *= glm::angleAxis(angle, Vec3(x, y, z) * orientation); }

	void yaw(float angle) { this->rotate(-angle, 0.0f, 1.0f, 0.0f); }
	void pitch(float angle) { this->rotate(-angle, 1.0f, 0.0f, 0.0f); }
	void roll(float angle) { this->rotate(-angle, 0.0f, 0.0f, 1.0f); }
};

void FreeCamera_init(FreeCamera* view, Vec3 start_position = Vec3(0.0));


void FreeCamera_process_directional_movement(FreeCamera* view, MOVEMENT_DIRECTION direction, GLfloat delta_time);

Mat4 FreeCamera_calc_view_matrix(FreeCamera* view);
Mat4 FreeCamera_calc_calc_reverse_translation(FreeCamera* view);
Mat4 FreeCamera_calc_screen_to_world_matrix(FreeCamera* view);

void FreeCamera_target_set(FreeCamera* view, Vec2 target);
void FreeCamera_target_x_set(FreeCamera* view, f64 target);
void FreeCamera_target_y_set(FreeCamera* view, f64 target);

void FreeCamera_target_follow(FreeCamera* view, f64 t_delta_s);
void FreeCamera_target_follow_x(FreeCamera* view, f64 t_delta_s);
void FreeCamera_target_follow_y(FreeCamera* view, f64 t_delta_s);


#ifdef CAMERA_IMPLEMENTATION
#undef CAMERA_IMPLEMENTATION
#include "camera.cpp"
#endif

#endif // CAMERA_HPP