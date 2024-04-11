#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int grayscaleEnabled;

void main(){
if (grayscaleEnabled == 0){
    vec3 col = texture(screenTexture, TexCoords).rgb;
    FragColor = vec4(col, 1.0f);
}else{
    FragColor = texture(screenTexture, TexCoords);
    float average = (0.2*FragColor.r + 0.7*FragColor.g + 0.07*FragColor.b);
    FragColor = vec4(average, average, average, 1.0f);
}
}