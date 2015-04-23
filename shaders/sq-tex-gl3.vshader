#version 150

uniform float uOffsetX;
uniform float uOffsetY;

in vec2 aPosition;
in vec2 aTexCoord;

out vec2 vTexCoord;

void main() {
  gl_Position = vec4(aPosition.x + uOffsetX, aPosition.y + uOffsetY, 0, 1);
  vTexCoord = aTexCoord;
}
