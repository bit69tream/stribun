#version 100

varying vec2 fragTexCoord;

uniform float alpha;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec4 trailColor;

void main() {
  vec4 origColor = texture2D(texture0, fragTexCoord) * colDiffuse;

  gl_FragColor =
    vec4(trailColor.rgb, alpha) *
    float(origColor.a != 0.);
}
