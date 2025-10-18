#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D textAtlas;
uniform vec4 color;

void main()
{
    float distance = texture(textAtlas, TexCoord).r;
    float alpha = smoothstep(0.5 - 0.1, 0.5 + 0.1, distance); // width control
    FragColor = vec4(color.rgb, alpha * color.a);
}
