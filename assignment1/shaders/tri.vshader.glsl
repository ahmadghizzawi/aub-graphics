#version 150

in vec2 aPosition;
in vec2 aTexCoord;
in vec3 aColor;

uniform mat4 uTransform;
uniform float uXSpan;
uniform float uYSpan;

out vec2 vTexCoord;
out vec2 vTemp;
out vec3 vColor;

void main() {
  gl_Position = uTransform * vec4(aPosition.x * uXSpan, aPosition.y * uYSpan, 0, 1);
  vTexCoord = aTexCoord;
  vTemp = vec2(1, 1);
  vColor = aColor;
}
