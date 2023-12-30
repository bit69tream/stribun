#version 100

precision mediump float;

varying vec2 fragTexCoord;

uniform vec4 borderColor0;
uniform vec4 borderColor1;

uniform float time;

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
  vec2 uv = fragTexCoord;

  float size = .007;

  float lx = smoothstep(0., size, uv.x);
  float ly = smoothstep(0., size, uv.y);

  float rx = 1. - smoothstep(1. - size, 1., uv.x);
  float ry = 1. - smoothstep(1. - size, 1., uv.y);

  float border = 1. - (lx * rx * ly * ry);

  vec2 coord = uv * 2048.;

  float t = rand(coord + time);

  finalColor = mix(borderColor0, borderColor1, t) * border;
}
