#version 430 
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 LightPos;
uniform vec3 EyePos;

void main()
{    
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Diffuse= texture(gAlbedoSpec, TexCoords).rgb;

	vec3 lighting = Diffuse * 0.1; //ambient
	vec3 viewDir = normalize(EyePos - FragPos);

	vec3 lightDir = normalize(LightPos - FragPos);
	vec3 diffuse = max(dot(Normal, lightDir), 0.0f) * Diffuse ;

	FragColor = vec4((lighting + diffuse), 1.0);

}

