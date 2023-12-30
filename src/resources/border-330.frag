#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 borderColor;

void main() {
  vec4 texColor = texture(texture0, fragTexCoord) * colDiffuse;
  vec2 uv = fragTexCoord;

  float size = .01;

  float lx = smoothstep(0., size, uv.x);
  float ly = smoothstep(0., size, uv.y);

  float rx = 1. - smoothstep(1. - size, 1., uv.x);
  float ry = 1. - smoothstep(1. - size, 1., uv.y);

  float border = 1. - (lx * rx * ly * ry);

  finalColor = texColor + (borderColor * border);
}
