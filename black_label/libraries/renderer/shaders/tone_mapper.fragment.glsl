uniform sampler2D lit;
uniform sampler2D bloom;
uniform sampler3D lut;



layout(location = 0) out vec4 result;



const float 
		A = 0.22,
		B = 0.30,
		C = 0.10,
		D = 0.20,
		E = 0.01,
		F = 0.30,
		LinearWhite = 11.2;

// Filmic Tone Mapping Approximation
//
// As seen on: http://filmicgames.com/archives/75 by John Hable.
// Used in the video game Uncharted 2.
vec3 f( in vec3 x )
{
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F)) - E/F;
}


vec3 screen( in vec3 bottom, in vec3 top )
{
	return vec3(1.0) - (vec3(1.0) - bottom) * (vec3(1.0) - top);
}

float linear_to_srgb( in float color )
{
	if (color <= 0.0031308)
		return color * 12.92;
	else
		return 1.055 * pow(color, 1.0 / 2.4) - 0.055;
}
vec3 linear_to_srgb( in vec3 color )
{
	color.r = linear_to_srgb(color.r);
	color.g = linear_to_srgb(color.g);
	color.b = linear_to_srgb(color.b);
	return color;
}

float srgb_to_linear( in float color )
{
	if (color <= 0.04045)
		return color / 12.92;
	else
		return pow((color + 0.055) / 1.055, 2.4);
}
vec3 srgb_to_linear( in vec3 color )
{
	color.r = srgb_to_linear(color.r);
	color.g = srgb_to_linear(color.g);
	color.b = srgb_to_linear(color.b);
	return color;
}

void main()
{
	vec2 tc = gl_FragCoord.xy / vec2(textureSize(lit, 0));
	result = texture(lit, tc);

	result.rgb = f(result.rgb) / f(vec3(LinearWhite));

	// vec3 bloom_ = texture(bloom, tc).rgb;
	// result.rgb = min(result.rgb + bloom_ * 0.5, vec3(1.0));	

	// float lut_size = float(textureSize(lut, 0).x);
	// vec3 scale = vec3((lut_size - 1.0) / lut_size);
	// vec3 offset = vec3(1.0 / (2.0 * lut_size));

	//result.rgb = linear_to_srgb(result.rgb);//pow(result.rgb, vec3(1.0 / 2.2));
	//result.rgb = texture(lut, result.rgb * scale + offset).rgb;
	//result.rgb = srgb_to_linear(result.rgb);


	// Overrides
	result.rgb = texture(lit, tc).xyz;

}
