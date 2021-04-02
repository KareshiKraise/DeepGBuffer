#version 430 
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoords;
out vec3 pPos;
out vec3 pNormal;

uniform mat4 MVP;


void main()
{
	TexCoords = aTexCoords;
	pPos      = aPos;
	pNormal   = aNormal;
	gl_Position = MVP * vec4(aPos, 1.0);
}
