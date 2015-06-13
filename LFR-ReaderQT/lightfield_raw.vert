attribute vec4 vertex;
attribute vec3 texCoord;
uniform mat4 matrix;
varying vec3 texc;

void main(void)
{
    gl_Position = matrix * vertex;
    texc = texCoord;
}
