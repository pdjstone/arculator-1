#version 330
precision highp float;
out vec4 FragColor;
in vec3 ourColor;
in vec2 TexCoord;
uniform sampler2D texture1;
uniform vec4 zoom;
void main()
{
    vec4 izoom = zoom;
    ivec2 isize = textureSize(texture1, 0);
    vec2  size = vec2(isize.x, isize.y);

    // we flipped the y co-ordinate of the texture, compensate for that
    izoom.y += izoom.w;

    // pan and zoom into the texture wherever we're told by the VIDC
    vec2 zoomed = izoom.xy / size + TexCoord * (izoom.zw / size);

    // We've copied the VIDC memory directly into the texture, 
    // which is BGRA with alpha at 0.
    //
    // So flip it to RGBA which is the portable format the texture is set up for,
    // and force the alpha to 1.0.
    FragColor = texture(texture1, zoomed).bgra;
    FragColor.a = 1.0;
}
