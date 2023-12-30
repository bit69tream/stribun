#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec4 borderColor;

void main() {
  vec2 uv = vec2(fragTexCoord.x, 1. - fragTexCoord.y);

  float size = .005;

  float lx = smoothstep(0., size, uv.x);
  float ly = smoothstep(0., size, uv.y);

  float rx = 1. - smoothstep(1. - size, 1., uv.x);
  float ry = 1. - smoothstep(1. - size, 1., uv.y);

  float border = 1. - (lx * rx * ly * ry);

  finalColor = (borderColor * border);
}
