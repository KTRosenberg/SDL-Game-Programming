#version 330 core
precision highp float;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;

out vec3 v_position;
out vec4 v_color;

uniform mat4 u_matrix;

void main(void) 
{
   gl_Position = u_matrix * vec4(a_position, 1.0);
   v_position = a_position.xyz;

   v_color = a_color;
}