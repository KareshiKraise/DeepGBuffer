#version 430

out vec3 ao_map;
in vec2 Tex;

layout (binding = 0) uniform sampler2DArray Position;
layout (binding = 1) uniform sampler2DArray Normal;
layout (binding = 2) uniform sampler2D      texNoise;

const int k_size = 32; //kernel size
uniform vec3 kernel_samples[32];

uniform float radius; 
uniform float bias;
//uniform float beta; 
//uniform float epsilon; 
uniform mat4 P; // projection matrix

const vec2 noise_scale = vec2(1280.0f/4.0f, 720.0f/4.0f);

void main()
{
	vec3 fragPos = texture(Position, vec3(Tex, 0)).xyz;
	vec3 normal = normalize(texture(Normal, vec3(Tex, 0)).rgb);
	vec3 randomNoise = normalize(texture(texNoise, Tex * noise_scale).xyz);

	vec3 tangent = normalize(randomNoise - normal * dot(randomNoise, normal));
	vec3 bitangent = normalize(cross(tangent, normal));
	mat3 TBN = mat3(tangent, bitangent, normal);

	float occlusion = 0.0f;
	for(int i = 0; i < k_size; ++i)
	{
		vec3 samplePos = TBN * kernel_samples[i];
		samplePos = fragPos + samplePos * (radius);

		vec4 offset = vec4(samplePos, 1.0f);
		offset = P * offset;
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * 0.5f + 0.5f;
		
		float sample_depth = texture(Position, vec3(offset.xy, 0)).z;
		float range_check = smoothstep(0.0f, 1.0f, radius/abs(fragPos.z - sample_depth));
		
		occlusion += (sample_depth >= (samplePos.z + bias) ? 1.0f : 0.0f) * range_check;
	}
	occlusion = 1.0f - (occlusion / k_size);
	ao_map = vec3(occlusion);
}