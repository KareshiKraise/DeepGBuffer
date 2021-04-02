#version 430

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 Tex;
layout(location = 2) in vec3 Normal;

uniform mat4 MV;
uniform mat4 Proj;

out vec3 pos;
out vec2 tex;
out vec3 normal;
out vec4 clipspace;

void main()
{
	vec4 viewPos = MV * vec4(Pos, 1.0f);
	pos = vec3(viewPos);
	clipspace = Proj * viewPos;
	mat3 nMat = transpose(inverse(mat3(MV)));
	normal = nMat * Normal;
	tex = Tex;
	gl_Position = clipspace;		
}