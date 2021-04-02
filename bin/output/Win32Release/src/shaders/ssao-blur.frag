#version 430

out float FragColor;

in vec2 TexCoords;

uniform sampler2D ssaoInput;

int kernel = 2;


void main()
{
	vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput , 0));
	float result = 0.0f;

	for(int x = -kernel; x < kernel; ++x)
	{
		for(int y = -kernel; y < kernel; ++y)
		{
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(ssaoInput, TexCoords + offset).r;
		}
	}

	FragColor = result / (4.0 * 4.0);
}