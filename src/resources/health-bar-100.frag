#version 100

precision mediump float;

varying vec2 fragTexCoord;

out vec4 finalColor;

uniform float health;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec4 goodColor;
uniform vec4 badColor;

void main() {
  vec4 origColor = texture2D(texture0, fragTexCoord) * colDiffuse;

  if (origColor.a == 0. || origColor.xyz == vec3(0.)) {
    gl_FragColor = origColor;
    return;
  }

  if ((fragTexCoord.y) > health) {
    gl_FragColor = badColor;
  } else {
    gl_FragColor = goodColor;
  }
}
