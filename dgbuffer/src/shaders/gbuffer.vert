#version 430 
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoords;
out vec3 pPos;
out vec3 pNormal;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;



void main()
{
	vec4 worldPos = M * vec4(aPos, 1);
	pPos = worldPos.xyz;
	TexCoords = aTexCoords;
	pNormal = aNormal;
		
	gl_Position = P * V * worldPos;
}
