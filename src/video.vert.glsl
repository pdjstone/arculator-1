#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main()
{
    // map the position and colour of each monitor vertex directly
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;

    // but y-flip the texture coordinate
    TexCoord = aTexCoord * vec2(1.0, -1.0);
}
