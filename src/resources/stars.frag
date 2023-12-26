#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform float time;
uniform vec2 resolution;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

float hash21(vec2 p) {
    p = fract(p*vec2(123.34, 456.21));
    p += dot(p, p+45.32);
    return fract(p.x*p.y);
}

float star(vec2 uv, float flare) {
  float d = length(uv);
  float m = .05/d;

  float rays = max(0., 1. - abs(uv.x * uv.y * 1000.));
  m += rays * flare;
  m *= smoothstep(1., .2, d);

  return m;
}

void main() {
  vec4 noiseColor = texture(texture0, fragTexCoord) * colDiffuse;

  vec4 nebulaColor = vec4(.325, .271, .51, .3) * noiseColor;

  vec2 uv = vec2(fragTexCoord.x, 1-fragTexCoord.y) - .5;

  // make uv follow aspect ratio
  uv = (uv * resolution) / resolution.y;

  uv *= 3.;

  float starColor = 0;

  vec2 gv = fract(uv) - .5;
  vec2 id = floor(uv);

  for (float y = -1; y <= 1; y += 1) {
    for (float x = -1; x <= 1; x += 1) {
      vec2 offset = vec2(x, y);

      float n = hash21(id + offset);

      float size = fract(n * 791.65);

      float star = star(gv - offset - vec2(n, fract(n * 34.)) + .5,
                        smoothstep(.9, 1., size) * .5);

      star *= sin((time / 5.) + n * 6.2831) * .5;
      starColor += star * size;
    }
  }

  finalColor =
    nebulaColor +
    vec4(clamp(starColor, 0., 10.));
}
