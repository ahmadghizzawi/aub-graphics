#version 150

uniform float uVertexScale;
uniform float uXSpan;
uniform float uYSpan;

in vec2 aPosition;
in vec2 aTexCoord;

out vec2 vTexCoord;
out vec2 vTemp;

void main() {
  gl_Position = vec4(aPosition.x * uVertexScale * uXSpan, aPosition.y * uYSpan, 0, 1);
  vTexCoord = aTexCoord;
  vTemp = vec2(1, 1);
}
