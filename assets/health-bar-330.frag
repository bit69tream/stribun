#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform float health;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec4 goodColor;
uniform vec4 badColor;

uniform float healTimer;
uniform vec4 healColor;

vec3 rgb2hsv(vec3 c) {
  vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
  vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

  float d = q.x - min(q.w, q.y);
  float e = 1.0e-10;
  return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)),
              d / (q.x + e),
              q.x);
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx,
                   clamp(p - K.xxx, 0.0, 1.0),
                   c.y);
}

void main() {
  vec4 origColor = texture(texture0, fragTexCoord) * colDiffuse;

  if (origColor.a == 0. || origColor.xyz == vec3(0.)) {
    finalColor = origColor;
    return;
  }
  float v = rgb2hsv(origColor.rgb).z;

  vec3 goodColorAdjusted = rgb2hsv(goodColor.rgb);
  goodColorAdjusted.z = v;
  goodColorAdjusted = hsv2rgb(goodColorAdjusted);

  vec3 badColorAdjusted = rgb2hsv(badColor.rgb);
  badColorAdjusted.z = v;
  badColorAdjusted = hsv2rgb(badColorAdjusted);

  vec4 color = vec4(0.);

  if (fragTexCoord.y > health) {
    color = vec4(badColorAdjusted, 1.);
  } else {
    color = vec4(goodColorAdjusted, 1.);
  }

  finalColor = color + (healColor * healTimer);
}
