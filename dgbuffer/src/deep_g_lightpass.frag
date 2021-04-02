#version 430

out vec4 color;
in vec2 Tex;

layout (location = 0) uniform sampler2DArray Position;
layout (location = 1) uniform sampler2DArray Normal;
layout (location = 2) uniform sampler2DArray Albedo;
layout (location = 3) uniform sampler2D ssao;

struct Light
{
	vec3 pos;
	vec3 col;	
};
uniform Light light;

uniform bool AO;

void main()
{
	vec3 fragpos = texture(Position, vec3(Tex, 0)).rgb;
	vec3 normal = texture(Normal, vec3(Tex, 0)).rgb;
	vec3 Diffuse = texture(Albedo, vec3(Tex, 0)).rgb;
	float AmbientOcclusion = texture(ssao, Tex).r;

	vec3 ambient;
	ambient = vec3(0.25f * Diffuse * AmbientOcclusion);

	vec3 lighting = ambient;

	vec3 viewDir = normalize(-fragpos);
	vec3 lightDir = normalize(light.pos - fragpos);
	vec3 diffuse = max(dot(normal, lightDir), 0.0f) * Diffuse * light.col;

	lighting += diffuse;

	if(!AO)
	{
		color = vec4(normal, 1.0);
	}
	else
	{
		color = vec4(vec3(AmbientOcclusion), 1.0);
	}
	

}


