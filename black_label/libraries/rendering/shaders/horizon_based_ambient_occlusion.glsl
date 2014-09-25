#define USE_RANDOM_DIRECTION 0

uniform sampler2D depths;
uniform sampler2D wc_normals;
uniform sampler2D random_texture;

uniform vec3 wc_view_eye_position;
uniform float z_far;

uniform vec2 tc_window;

uniform mat4 view_matrix;
uniform mat4 projection_matrix;
uniform mat4 view_projection_matrix;
uniform mat4 inverse_view_projection_matrix;



struct vertex_data
{
	vec3 wc_view_ray_direction;
};
noperspective in vertex_data vertex;

out vec4 ambient_occlusion;



float tc_depth( in vec2 tc )
{
	return texture(depths, tc).x;
}

float ec_depth( in vec2 tc )
{
	float buffer_z = texture(depths, tc).x;
	return projection_matrix[3][2] / (-2.0 * buffer_z + 1.0 - projection_matrix[2][2]);
}

float tan_to_sin( in float x )
{
    return x * pow(x * x + 1.0, -0.5);
}

vec3 tc_to_ec( in vec2 tc )
{
	vec3 tc_sample;
	tc_sample.xy = tc;
	tc_sample.z = tc_depth(tc_sample.xy);
	vec3 ndc_sample = tc_sample * 2.0 - 1.0;		
	vec4 temporary = inverse_view_projection_matrix * vec4(ndc_sample, 1.0);
	vec3 wc_sample = temporary.xyz / temporary.w;
	vec3 ec_sample = (view_matrix * vec4(wc_sample, 1.0)).xyz;
	return ec_sample;
}

vec3 minimum_difference( in vec3 p, in vec3 p_right, in vec3 p_left )
{
    vec3 v1 = p_right - p;
    vec3 v2 = p - p_left;
    return (dot(v1, v1) < dot(v2, v2)) ? v1 : v2;
}

void main()
{
	vec2 depths_size = textureSize(depths, 0);
	vec2 depths_size_inversed = vec2(1.0) / depths_size;
	vec2 tc_depths = gl_FragCoord.xy / tc_window;
	vec3 wc_normal = texture(wc_normals, tc_depths).xyz;
	float ndc_linear_depth = -ec_depth(tc_depths) / z_far;
	vec3 wc_position = wc_view_eye_position + vertex.wc_view_ray_direction * ndc_linear_depth;

	vec3 ec_position = (view_matrix * vec4(wc_position, 1.0)).xyz;
	float ec_position_depth = ec_position.z;

	ambient_occlusion.a = 0.0;
	
	const int base_samples = 0; 
	const int min_samples = 8;
	const float radius = 20.0;
	const float radius_squared = radius * radius;
	const float bias = 0.3;

	int samples = max(int(base_samples / (1.0 + base_samples * ndc_linear_depth)), min_samples);

	float projected_radius = radius / -ec_depth(tc_depths);

	vec2 inverted_random_texture_size = 1.0 / vec2(textureSize(random_texture, 0));
	vec2 tc_random_texture = gl_FragCoord.xy * inverted_random_texture_size;

	vec3 random_direction = texture(random_texture, tc_random_texture).xyz;
	random_direction = normalize(random_direction * 2.0 - 1.0);

 	float angle_step = 2.0 * PI / float(samples);
	for (int i = 0; i < samples; ++i)
	{
#if USE_RANDOM_DIRECTION
		vec2 sample_random_direction = texture(random_texture, vec2(float(i) * inverted_random_texture_size.x, float(i / textureSize(random_texture, 0).x) * inverted_random_texture_size.y)).xy;
		sample_random_direction = sample_random_direction * 2.0 - 1.0;
		vec2 tc_sample_direction = sample_random_direction;
#else
		vec2 tc_sample_direction = vec2(cos(float(i) * angle_step), sin(float(i) * angle_step));
#endif


	    // Tangent vector
	    vec3 p_right, p_left, p_top, p_bottom;
	    p_right = tc_to_ec(tc_depths + vec2(depths_size_inversed.x, 0.0));
	    p_left = tc_to_ec(tc_depths + vec2(-depths_size_inversed.x, 0.0));
	    p_top = tc_to_ec(tc_depths + vec2(0.0, depths_size_inversed.y));
	    p_bottom = tc_to_ec(tc_depths + vec2(0.0, -depths_size_inversed.y));
	    vec3 dp_du = minimum_difference(ec_position, p_right, p_left);
	    vec3 dp_dv = minimum_difference(ec_position, p_top, p_bottom) * (depths_size.y * depths_size_inversed.x);
    	vec3 ec_tangent = tc_sample_direction.x * dp_du + tc_sample_direction.y * dp_dv;


		const int steps = 6;
		vec2 tc_step_size = tc_sample_direction * projected_radius / float(steps);
		vec2 ec_step_size = tc_sample_direction * radius / float (steps);

		float tan_tangent_angle = ec_tangent.z / length(ec_tangent.xy) + tan(bias);
		float tan_horizon_angle = tan_tangent_angle;
		float sin_horizon_angle = tan_to_sin(tan_horizon_angle);

		for (float j = 1.0; j <= float(steps); j += 1.0)
		{
			vec2 tc_sample = vec2(tc_depths + tc_step_size * j);
			vec3 ec_horizon = vec3(ec_step_size * j, ec_depth(tc_sample) - ec_position_depth);
			float ec_horizon_length_squared = dot(ec_horizon, ec_horizon);
			float tan_sample = ec_horizon.z / length(ec_horizon.xy);

			if (radius_squared >= ec_horizon_length_squared && tan_sample > tan_horizon_angle)
			{
				float sin_sample = tan_to_sin(tan_sample);
				float weight = 1.0 - ec_horizon_length_squared / radius_squared;
				ambient_occlusion.a += (sin_sample - sin_horizon_angle) * weight;
				tan_horizon_angle = tan_sample;
				sin_horizon_angle = sin_sample;
			}
		}
	}
	
	ambient_occlusion.a /= samples;
	ambient_occlusion.a = 1.0 - ambient_occlusion.a;
}



