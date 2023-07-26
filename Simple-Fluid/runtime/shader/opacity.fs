out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D colorMap;

void main()
{
    FragColor = texture2D(colorMap, TexCoords);
}