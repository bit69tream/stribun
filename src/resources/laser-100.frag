#version 100

precision mediump float;

varying vec2 fragTexCoord;

uniform float time;
uniform vec2 resolution;

float pattern(vec2 muv, float t) {
  muv += vec2(t, cos(muv.x / 2. + t));
  return abs(cos(muv.y)) / (muv.y * muv.y);
}

float wave(vec2 muv, float p) {
  float t = time * 2. + p * 2. * 3.14;
  return pattern(vec2((muv.x + p) * -1, muv.y), t);
}

vec2 transformUv(vec2 muv) {
  float t = smoothstep(10., 50., resolution.x * muv.x) * 5.5 + .1;

  muv.y -= .5;
  muv.y /= t;
  muv.x *= resolution.x / resolution.y;
  muv *= 10.;
  return muv;
}

void main() {
  vec2 uv = fragTexCoord;

  vec2 muv = transformUv(uv);
  vec2 fuv = transformUv(vec2(uv.x, 1.-uv.y));

  float it = 12.;
  vec3 c0 = vec3(.2, .2, .5);
  vec3 c = vec3(.2, .2, .5) / (it * 8.);
  // vec3 c = vec3(.5, .2, .2) / (it * 8.);

  float tm = time;

  float speed = tm / it;

  float a = 0.;

  float t = (1. / it) * speed;

  vec2 f2 = fuv * 2.;
  vec2 m2 = muv * 2.;

  fuv.y *= 1. + smoothstep(.1, .7, (sin(tm * .1) * cos(tm * .1)) + 1.);
  muv.y *= 1. + smoothstep(.1, .7, (sin(tm * .1) * cos(tm * .1)) + 1.);

  a += wave(muv, t); t += (2.  / it)*speed;
  a += wave(muv, t); t += (3.  / it)*speed;
  a += wave(muv, t); t += (4.  / it)*speed;
  a += wave(muv, t); t += (5.  / it)*speed;
  a += wave(fuv, t); t += (6.  / it)*speed;
  a += wave(fuv, t); t += (7.  / it)*speed;
  a += wave(fuv, t); t += (8.  / it)*speed;
  a += wave(fuv, t); t += (9.  / it)*speed;
  a += wave(fuv, t); t += (10.  / it)*speed;
  a += wave(fuv, t); t += (11. / it)*speed;
  a += wave(fuv, t); t += (12. / it)*speed;
  a += wave(fuv, t);

  a /= it;

  gl_FragColor =
    vec4((c * a * 2.) +
         (c0 * float(f2.y >= 0.) * smoothstep(2.5, 0., f2.y) * 6.) +
         (c0 * float(m2.y >= 0.) * smoothstep(2.5, 0., m2.y) * 6.), a);
}
