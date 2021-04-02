#version 430 
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 pPos;
in vec3 pNormal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{    
    gPosition = pPos;
	gNormal = normalize(pNormal);
	//gAlbedoSpec.rgb = vec3(0.95);
	gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
	gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;
}