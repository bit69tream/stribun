#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform float health;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec4 goodColor;
uniform vec4 badColor;

void main() {
  vec4 origColor = texture(texture0, fragTexCoord) * colDiffuse;

  if (origColor.a == 0. || origColor.xyz == vec3(0.)) {
    finalColor = origColor;
    return;
  }

  if ((fragTexCoord.y) > health) {
    finalColor = badColor;
  } else {
    finalColor = goodColor;
  }
}
