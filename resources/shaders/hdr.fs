#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform bool bloom;
uniform bool hdr;
uniform float exposure;

void main(){
    const float gamma = 2.2f;
    vec3 hdrColor = texture(scene, TexCoords).xyz;
    vec3 bloomColor = texture(bloomBlur, TexCoords).xyz;

    if(bloom){
        hdrColor += bloomColor;
    }

    vec3 result = hdrColor;

    if(hdr){
        result = vec3(1.0f) - exp(-hdrColor * exposure);
    }

    result = pow(result, vec3(1.0f / gamma));

    FragColor = vec4(result, 1.0f);
}