#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform float alpha;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec4 trailColor;

void main() {
  vec4 origColor = texture(texture0, fragTexCoord) * colDiffuse;

  finalColor =
    vec4(trailColor.rgb, alpha) *
    float(origColor.a != 0.);
}
