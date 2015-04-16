#version 150

uniform sampler2D uTex;
uniform int uSolid;

in vec2 vTexCoord;

out vec4 fragColor;

void main(void) {
  fragColor = (uSolid == 1) ? vec4(1.0, 0.0, 0.0, 1.0) : texture(uTex, vTexCoord);
}
