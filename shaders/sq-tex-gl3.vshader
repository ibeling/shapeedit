#version 150

in vec2 aPosition;
in vec2 aTexCoord;

out vec2 vTexCoord;

void main() {
  gl_Position = vec4(aPosition.x, aPosition.y, 0, 1);
  vTexCoord = aTexCoord;
}
