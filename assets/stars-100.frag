#version 100

precision mediump float;

varying vec2 fragTexCoord;

uniform float time;
uniform float dom;
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

vec2 rotateUV(vec2 uv, float rotation)
{
    float mid = 0.5;
    return vec2(
        cos(rotation) * (uv.x - mid) + sin(rotation) * (uv.y - mid) + mid,
        cos(rotation) * (uv.y - mid) - sin(rotation) * (uv.x - mid) + mid
    );
}

void main() {
  vec4 noiseColor1 = texture2D(texture0, fragTexCoord) * colDiffuse;
  vec4 noiseColor2 = texture2D(texture0, rotateUV(fragTexCoord, 1.570796)) * colDiffuse;

  vec3 nebulaBlue = vec3(84. / 255., 104. / 255., 255. / 255.);
  vec3 nebulaRed = vec3(147. / 255., 38. / 255., 69. / 255.);

  float c = noiseColor1.r;
  float c2 = noiseColor2.r;

  vec4 nebulaColor =
    vec4((nebulaBlue * c) +
         (nebulaRed * c2),
         .9) *
    smoothstep(.1, 1.,
               (c2 * float(dom == 1.)) +
               (c  * float(dom != 1.)));

  vec2 uv = vec2(fragTexCoord.x, 1.-fragTexCoord.y) - .5;

  // make uv follow aspect ratio
  uv = (uv * resolution) / resolution.y;

  uv *= 5.;

  float starBrigtness = 0.;

  vec2 gv = fract(uv) - .5;
  vec2 id = floor(uv);

  for (float y = -1.; y <= 1.; y += 1.) {
    for (float x = -1.; x <= 1.; x += 1.) {
      vec2 offset = vec2(x, y);

      float n = hash21(id + offset);

      float size = fract(n * 791.65);

      float star = star(gv - offset - vec2(n, fract(n * 34.)) + .5,
                        smoothstep(.9, 1., size) * .5);

      star *= sin((time / 5.) + n * 6.2831) * .5;
      starBrigtness += star * size;
    }
  }

  // vec4 starColor = vec4(0., .31, .596, .1);
  vec4 starColor = vec4(.235, .314, .569, .1);

  gl_FragColor =
    nebulaColor +
    starColor * vec4(clamp(starBrigtness, 0., 10.));
}
