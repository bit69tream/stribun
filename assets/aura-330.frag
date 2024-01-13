#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform float time;

// https://www.shadertoy.com/view/XtjXRD

void main() {
  vec4 orig = texture(texture0, fragTexCoord) * colDiffuse;

  if (orig.a == 0.) {
    finalColor = orig;
    return;
  }

  vec2 uv = fragTexCoord;

  uv *= 42.0;

  float r =
    cos(uv.y * 12.345 -
        time * 4. +
        cos(12.234) * 2. +
        cos(uv.x * 32.2345 + cos(uv.y * 17.234))) +
    cos(uv.x * 12.345);

  vec3 c = mix(vec3(1., 0., 0.),
               vec3(.6, .2, .2),
               cos(r + cos(uv.y * 24.3214) * .1 +
                   cos(uv.x * 6.324 + time * 4.) + time) *
               .5 + .5);

  finalColor = vec4(c, .9);
}
