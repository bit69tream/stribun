#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform float alpha;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec4 glowColor;

void main() {
  vec4 shipColor = texture(texture0, fragTexCoord) * colDiffuse;
  vec4 color = shipColor + (glowColor * alpha);
  finalColor = color * float(shipColor.a > 0.);
}
