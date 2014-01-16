uniform sampler2D depths;
uniform sampler2D wc_normals;
uniform sampler2D source;

uniform mat4 projection_matrix;



out vec4 result;



float ec_depth( in vec2 tc )
{
	float buffer_z = texture(depths, tc).x;
	return projection_matrix[3][2] / (-2.0 * buffer_z + 1.0 - projection_matrix[2][2]);
}



void main()
{
	vec2 inverted_source_size = 1.0 / vec2(textureSize(source, 0));
	vec2 tc = gl_FragCoord.xy * inverted_source_size;

#if defined HORIZONTAL
#define DIRECTION_SWIZZLE xy
#elif defined VERTICAL
#define DIRECTION_SWIZZLE yx
#else
	"Define HORIZONTAL or VERTICAL before compiling this shader!";
#endif



	result = texture(source, tc);



	const float normal_power = 10.0;
	const float depth_power = 0.5;
	const int samples_in_each_direction = 1;
	float weightSum = 1.0;

	for (int i = -1; i >= -samples_in_each_direction; --i)
	{
		vec2 offset = vec2(float(i), 0).DIRECTION_SWIZZLE * inverted_source_size;

		float normalWeight = pow(dot(texture(wc_normals, tc + offset).xyz, texture(wc_normals, tc).xyz) * 0.5 + 0.5, normal_power);
		float positionWeight = 1.0 / pow(1.0 + abs(ec_depth(tc) - ec_depth(tc + offset)), depth_power);
		float weight = normalWeight * positionWeight;

		result += texture(source, tc + offset) * weight;
		weightSum += weight;
	}

	for (int i = 1; i <= samples_in_each_direction; ++i)
	{
		vec2 offset = vec2(float(i), 0).DIRECTION_SWIZZLE * inverted_source_size;

		float normalWeight = pow(dot(texture(wc_normals, tc + offset).xyz, texture(wc_normals, tc).xyz) * 0.5 + 0.5, normal_power);
		float positionWeight = 1.0 / pow(1.0 + abs(ec_depth(tc) - ec_depth(tc + offset)), depth_power);
		float weight = normalWeight * positionWeight;

		result += texture(source, tc + offset) * weight;
		weightSum += weight;
	}

	result /= weightSum;
}
