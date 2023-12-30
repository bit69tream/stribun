#version 100

precision mediump float;

varying vec2 fragTexCoord;

uniform vec4 borderColor;

void main() {
  vec2 uv = fragTexCoord;

  float size = .01;

  float lx = smoothstep(0., size, uv.x);
  float ly = smoothstep(0., size, uv.y);

  float rx = 1. - smoothstep(1. - size, 1., uv.x);
  float ry = 1. - smoothstep(1. - size, 1., uv.y);

  float border = 1. - (lx * rx * ly * ry);

  gl_FragColor = (borderColor * border);
}
