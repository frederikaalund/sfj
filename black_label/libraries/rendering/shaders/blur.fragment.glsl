uniform sampler2D source;



out vec3 result;



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



	// Naive Method
	//
	// This is the simple way to do a separable filter.
//	result = textureOffset(source, tc, ivec2(4, 0).DIRECTION_SWIZZLE).rgb * 0.05;
//	result += textureOffset(source, tc, ivec2(3, 0).DIRECTION_SWIZZLE).rgb * 0.09;
//	result += textureOffset(source, tc, ivec2(2, 0).DIRECTION_SWIZZLE).rgb * 0.12;
//	result += textureOffset(source, tc, ivec2(1, 0).DIRECTION_SWIZZLE).rgb * 0.15;
//	result += texture(source, tc).rgb * 0.18;
//	result += textureOffset(source, tc, ivec2(1, 0).DIRECTION_SWIZZLE).rgb * 0.15;
//	result += textureOffset(source, tc, ivec2(2, 0).DIRECTION_SWIZZLE).rgb * 0.12;
//	result += textureOffset(source, tc, ivec2(3, 0).DIRECTION_SWIZZLE).rgb * 0.09;
//	result += textureOffset(source, tc, ivec2(4, 0).DIRECTION_SWIZZLE).rgb * 0.05;



	// Smart Method
	//
	// From "Faster filters using bilinear texture look-ups" by Mike Acton.
	// Url: https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2012/06/faster_filters.pdf?9d7bd4
	result = texture(source, tc + vec2(-4.30908, 0.5).DIRECTION_SWIZZLE * inverted_source_size).rgb * 0.055028;
	result += texture(source, tc + vec2(-2.37532, 0.5).DIRECTION_SWIZZLE * inverted_source_size).rgb * 0.244038;
	result += texture(source, tc + vec2(-0.50000, 0.5).DIRECTION_SWIZZLE * inverted_source_size).rgb * 0.401870;
	result += texture(source, tc + vec2( 1.37532, 0.5).DIRECTION_SWIZZLE * inverted_source_size).rgb * 0.244038;
	result += texture(source, tc + vec2( 3.30908, 0.5).DIRECTION_SWIZZLE * inverted_source_size).rgb * 0.055028;
}
