#version 100

precision mediump float;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 resolution;

uniform float pixelSize;

varying vec2 fragTexCoord;
varying vec4 fragColor;

void main() {
  vec2 d = pixelSize * (1. / resolution);
  vec2 coordinate = d * floor(fragTexCoord / d);

  gl_FragColor = texture2D(texture0, coordinate) * colDiffuse;
}
