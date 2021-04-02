#version 430

out vec3 ao_map;
in vec2 Tex;

layout (binding = 0) uniform sampler2DArray Position;
layout (binding = 1) uniform sampler2DArray Normal;
layout (binding = 2) uniform sampler2D      texNoise;

const int k_size = 32; //kernel size
uniform vec3 kernel_samples[32];

uniform float radius; 
//uniform float bias;
uniform float beta; 
uniform float epsilon; 
uniform mat4 P; // projection matrix

const vec2 noise_scale = vec2(1280.0f/4.0f, 720.0f/4.0f);
#define M_PI 3.1415926535897932384626433832795

float aoLayer(int i, int j, mat3 T, vec3 fragPos, vec3 fragNormal) {
	//rotate sample
	vec3 samplePos = T * kernel_samples[i];
	//add this offset to fragment position
	samplePos = fragPos + samplePos * radius; 

	//backproject sample to screen space
	vec4 coords = vec4(samplePos, 1.0);
	coords = P * coords;
	coords.xyz /= coords.w;
	coords.xyz = coords.xyz * 0.5 + 0.5;

	//look at position in given layer at these screen coordinates (Y)
	vec3 layerPos = texture(Position, vec3(coords.xy, j)).xyz;

	//v = Y - X
	vec3 v = layerPos - fragPos;
	//from 3.1 (3)
	return (1 - dot(v, v)/(radius * radius)) * max(0,(dot(v, fragNormal) - beta)/sqrt(dot(v,v) + epsilon));
}

void main()
{
	//get normal/position of closest layer (X)
	vec3 fragPos = texture(Position, vec3(Tex, 0)).xyz;
	vec3 normal = normalize(texture(Normal, vec3(Tex, 0)).xyz);

	//create matrix to rotate sample kernel a random amount using noise texture
	vec3 noise = normalize(texture(texNoise, Tex * noise_scale).xyz);
	vec3 tangent = normalize(noise - normal * dot(noise, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 T = mat3(tangent, bitangent, normal);

	//take samples
	float occlusion = 0.0;
	for(int i = 0; i < k_size; i++)
	{
		//use whichever layer gives highest value (aka closest)
		occlusion += max(0, max(aoLayer(i, 0, T, fragPos, normal), aoLayer(i, 1, T, fragPos, normal)));
	}
	//normalize occlusion factor and convert to ambient visibility
	ao_map = vec3(max(0, 1 - sqrt(occlusion * M_PI / k_size)));
}


//void main()
//{
//	vec3 fragPos = texture(Position, vec3(Tex, 0)).xyz;
//	vec3 normal = normalize(texture(Normal, vec3(Tex, 0)).rgb);
//	vec3 randomNoise = normalize(texture(texNoise, Tex * noise_scale).xyz);
//
//	vec3 tangent = normalize(randomNoise - normal * dot(randomNoise, normal));
//	vec3 bitangent = normalize(cross(tangent, normal));
//	mat3 TBN = mat3(tangent, bitangent, normal);
//
//	float occlusion = 0.0f;
//	for(int i = 0; i < k_size; ++i)
//	{
//		vec3 samplePos = TBN * kernel_samples[i];
//		samplePos = fragPos + samplePos * (radius);
//
//		vec4 offset = vec4(samplePos, 1.0f);
//		offset = P * offset;
//		offset.xyz /= offset.w;
//		offset.xyz = offset.xyz * 0.5f + 0.5f;
//		
//		float sample_depth = texture(Position, vec3(offset.xy, 0)).z;
//		float range_check = smoothstep(0.0f, 1.0f, radius/abs(fragPos.z - sample_depth));
//		
//		occlusion += (sample_depth >= (samplePos.z + bias) ? 1.0f : 0.0f) * range_check;
//	}
//	occlusion = 1.0f - (occlusion / k_size);
//	ao_map = vec3(occlusion);
//}