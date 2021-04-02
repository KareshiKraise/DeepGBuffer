#version 430 
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoords;
out vec3 pPos;
out vec3 pNormal;

uniform bool invertedNormals;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void main()
{
	vec4 viewPos = V * M * vec4(aPos, 1.0);
	pPos = viewPos.xyz;
	TexCoords = aTexCoords;

	mat3 normalMatrix = transpose(inverse(mat3(V * M)));
	if(invertedNormals)
	{
		pNormal = normalMatrix *  -aNormal;
	}else{
		pNormal = normalMatrix *  aNormal;
	}    
	
	gl_Position = P * viewPos;
}