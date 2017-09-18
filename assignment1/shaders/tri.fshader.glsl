// The fragment shader blends 3 colors: two from textures and one
// yellowish (1,1,0.5)  background color.
// sampler2D is a special data type that refers to an OpenGL texture unit.
// Fragment shader uses two texture images simultaneously
#version 150

uniform sampler2D uTex0;

in vec2 vTexCoord;
in vec2 vTemp;
in vec3 vColor;

out vec4 fragColor;

void main(void) {
  // The texture(...) calls always return a vec4. Data comes out of a texture in RGBA format
  vec4 texColor0 = texture(uTex0, vTexCoord);

  // fragColor is a vec4. The components are interpreted as red, green, blue, and alpha
  fragColor = texColor0 * vec4(vColor, 1.0);
}
