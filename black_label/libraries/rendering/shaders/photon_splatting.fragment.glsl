uniform sampler2D wc_positions, albedos;
uniform float specular_exponent;
uniform ivec2 window_dimensions;


in splat_data {
	flat vec3 wc_position;
	flat mat3 M;
	flat vec4 irradiance;
} splat;

layout(location = 0) out vec4 radiance;



float K( float x ) {
	return (1.0 > x) 
		? 3.0 / PI * (1.0 - x * x) * (1.0 - x * x)
		: 0.0;
}

void main() {
	vec2 tc_position = gl_FragCoord.xy / vec2(window_dimensions);
	vec3 wc_position = texture(wc_positions, tc_position).xyz;
	vec4 albedo = texture(albedos, tc_position); // [1]

	float d = length(splat.M * (wc_position - splat.wc_position));
	vec4 f = albedo / PI; // [sr^-1]

	radiance = PI * K(d) * f * splat.irradiance; // [W * m^-2 * sr^-1]
}