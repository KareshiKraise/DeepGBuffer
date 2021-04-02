#version 430

in vec2 Tex;
out vec3 color;

uniform sampler2D SSAOInput;

const int blur_kernel = 4;

void main()
{
	vec2 texelSize = 1.0 / vec2(textureSize(SSAOInput , 0));

	vec3 result = vec3(0.0f);
	int blurOffset = blur_kernel/2;
	for(int x = -blurOffset; x < blurOffset; x++)
	{
		for(int y = -blurOffset;y < blurOffset; y++)
		{
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(SSAOInput, Tex + offset).rgb;
		}
	}
	
	color = result / (blur_kernel * blur_kernel);
}