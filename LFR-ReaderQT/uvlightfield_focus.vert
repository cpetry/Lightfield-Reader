attribute vec4 vertex;
attribute vec4 texCoord;
uniform mat4 matrix;
varying vec4 texc;

void main(void)
{
    gl_Position = matrix * vertex;
    texc = texCoord;
}
