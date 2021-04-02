#version 430 
out vec4 FragColor;

in vec2 TexCoords;
in vec3 pPos;
in vec3 pNormal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform vec3 LightPos;


void main()
{    
	vec3 norm = normalize(pNormal);
	vec3 lightDir = normalize(LightPos - pPos);
	vec4 albedo = texture(texture_diffuse1, TexCoords);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
	vec3 ambient = 0.1 * vec3(1.0, 1.0, 1.0);

	vec3 result = (ambient + diffuse) * vec3(albedo);


	FragColor = vec4(result, 1.0);
}