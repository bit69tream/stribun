#version 100

varying vec2 fragTexCoord;

uniform float alpha;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec4 glowColor;

void main() {
  vec4 shipColor = texture2D(texture0, fragTexCoord) * colDiffuse;
  vec4 color = shipColor + (glowColor * alpha);
  gl_FragColor = color * float(shipColor.a > 0.);
}
