#version 150

uniform sampler2D uTex;

in vec2 vTexCoord;

out vec4 fragColor;

void main(void) {
  fragColor = texture(uTex, vTexCoord);
}
