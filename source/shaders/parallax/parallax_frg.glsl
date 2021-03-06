#version 330 core
precision highp float;

in vec3 v_position;
in vec4 v_color;
in vec2 v_uv;
in vec3 v_position_cam;

uniform float u_time;
uniform vec2 u_resolution;
uniform sampler2D tex0;
uniform sampler2D tex1;

uniform int   u_count_layers;
uniform float u_offset_layers_x;

out vec4 color;

float sin_pos(float f) { return (1.0 + sin(f)) / 2.0; }

vec3 sin_pos_v3(vec3 v3)
{
   return vec3(
      sin_pos(v3.x),
      sin_pos(v3.y),
      sin_pos(v3.z)
   );
}

void main(void)
{
   vec4 c = vec4(1.0);

   float Y = float(int(v_uv.y * 10.0)) / 10.0;


   vec4 texel = vec4(1.0);
   float rate = 0.65;
   for (int i = 0; i < u_count_layers; ++i) {
      float sample_coord = v_uv.x + (v_position_cam.x * rate);

      float off_y = (-v_position_cam.y * rate * rate * rate);
      off_y = max(off_y, 0.0);

      float off_x = (float(i) * u_offset_layers_x) + mod(sample_coord, u_offset_layers_x);
      //if (off_x - (float(i + 1) * u_offset_layers_x) >= 0) {
      //   color = vec4(1.0, 0.0, 0.0, 1.0);
      //   return;
      //}

      texel = vec4(texture(tex0, vec2(
         off_x,
         v_uv.y + off_y))
      );

      if (texel.a != 0.0) {
         break;
      }

      rate *= rate;
   }

   color = vec4(texel.rgb, texel.a);
}
