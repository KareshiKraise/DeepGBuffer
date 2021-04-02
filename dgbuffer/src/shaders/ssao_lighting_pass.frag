#version 430

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;


struct Light {
	vec3 Position;
	vec3 Color;
	float Linear;
	float Quadratic;
};
uniform Light light;
//uniform vec3 eyePos;

uniform bool AO;
uniform bool remove_AO;

void main()
{
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Diffuse = texture(gAlbedo, TexCoords).rgb;
	float Spec = texture(gAlbedo, TexCoords).a;
	float AmbientOcclusion = texture(ssao, TexCoords).r;
	vec3 ambient;
	if(!remove_AO)
	{
		ambient = vec3(0.3 * Diffuse * AmbientOcclusion);
	}else{
		ambient = vec3(0.3 * Diffuse);
	}
	
	
	vec3 lighting = ambient;
	vec3 viewDir = normalize(-FragPos);
	
	vec3 lightDir = normalize(light.Position - FragPos);
	vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.Color;
	
	vec3 halfVec = normalize(lightDir + viewDir);
	float spec = pow(max(dot(Normal, halfVec), 0.0), 8.0);
	vec3 specular = spec * light.Color * vec3(Spec);
	
	//float distance = length(light.Position - FragPos);
	//float attenuation = 1.0 / (1.0 + light.Linear * distance + light.Quadratic * distance * distance);
	
	//diffuse *= attenuation;
	//specular *= attenuation;
	lighting += diffuse + specular;
	//lighting += diffuse;
	
	if(!AO)
	{
		FragColor = vec4(lighting, 1.0);
	}
	else
	{ 
		FragColor = vec4(vec3(AmbientOcclusion), 1.0);
	}
		


}