#version 430
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

const int k_size = 64;
uniform vec3 samples[k_size];

uniform float radius = 0.5;
uniform float bias = 0.025;

const vec2 noise_scale = vec2(1280.0/4.0, 720.0/4.0);
uniform mat4 P;


void main()
{
	//extracting pos, normal and noise

	vec3 fragPos = texture(gPosition, TexCoords).xyz;
	vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
	vec3 randomVec = normalize(texture(texNoise, TexCoords * noise_scale).xyz);

	//gram schmit proccess
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);

	//change of basis matrix
	mat3 TBN = mat3(tangent, bitangent, normal);

	float occlusion = 0.0;
	for(int i = 0; i < k_size; ++i)
	{
		vec3 samp = TBN * samples[i];
		samp = fragPos + samp * radius;
		
		vec4 offset = vec4(samp, 1.0);
		offset = P * offset;
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * 0.5 + 0.5;

		float sample_depth = texture(gPosition, offset.xy).z;

		float range_check = smoothstep(0.0, 1.0, radius/abs(fragPos.z - sample_depth));
		occlusion += (sample_depth >= samp.z + bias ? 1.0 : 0.0) * range_check;
	}
	occlusion = 1.0f - (occlusion / k_size);
	FragColor = occlusion;
}