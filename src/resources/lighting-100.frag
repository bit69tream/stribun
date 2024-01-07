#version 100

// https://github.com/raysan5/raylib/blob/be0ea89f830cce2fcb62acfedc69da4faca11b43/examples/shaders/resources/shaders/glsl100/lighting.fs

precision mediump float;

varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

#define     MAX_LIGHTS              4
#define     LIGHT_DIRECTIONAL       0
#define     LIGHT_POINT             1

struct Light {
  int enabled;
  int type;
  vec3 position;
  vec3 target;
  vec4 color;
};

uniform Light lights[MAX_LIGHTS];
uniform vec4 ambient;
uniform vec3 viewPos;

void main() {
  vec4 texelColor = texture2D(texture0, fragTexCoord);
  vec3 lightDot = vec3(0.0);
  vec3 normal = normalize(fragNormal);
  vec3 viewD = normalize(viewPos - fragPosition);
  vec3 specular = vec3(0.0);

  for (int i = 0; i < MAX_LIGHTS; i++) {
    if (lights[i].enabled == 1) {
      vec3 light = vec3(0.0);

      if (lights[i].type == LIGHT_DIRECTIONAL) {
        light = -normalize(lights[i].target - lights[i].position);
      }

      if (lights[i].type == LIGHT_POINT) {
        light = normalize(lights[i].position - fragPosition);
      }

      float NdotL = max(dot(normal, light), 0.0);
      lightDot += lights[i].color.rgb*NdotL;

      float specCo = 0.0;
      if (NdotL > 0.0) specCo = pow(max(0.0, dot(viewD, reflect(-(light), normal))), 16.0); // 16 refers to shine
      specular += specCo;
    }
  }

  lightDot = clamp(lightDot, 0., .8);

  vec4 finalColor = (texelColor*((colDiffuse + vec4(specular, 1.0))*vec4(lightDot, 1.0)));
  finalColor += texelColor*(ambient/10.0);

  gl_FragColor = pow(finalColor, vec4(1.0/1.1));
}
