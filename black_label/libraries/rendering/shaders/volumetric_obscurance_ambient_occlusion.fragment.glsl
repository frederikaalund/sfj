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

float sphere_height ( in vec2 position, in float radius )
{
	return sqrt(radius * radius - dot(position, position));
}

void main()
{
	vec2 tc_depths = gl_FragCoord.xy / tc_window;
	vec3 wc_normal = texture(wc_normals, tc_depths).xyz;
	float ndc_linear_depth = -ec_depth(tc_depths) / z_far;
	vec3 wc_position = wc_view_eye_position + vertex.wc_view_ray_direction * ndc_linear_depth;

	vec3 ec_position = (view_matrix * vec4(wc_position, 1.0)).xyz;
	float ec_position_depth = ec_position.z;

	ambient_occlusion.a = 0.5;
	
	const int base_samples = 0; 
	const int min_samples = 32;
	const float radius = 10.0;
	const float lower_bound = 0.35;
	const float upper_bound = 1.0;

	int samples = max(int(base_samples / (1.0 + base_samples * ndc_linear_depth)), min_samples);

	mat4 inverse_view_projection_matrix = inverse(view_projection_matrix);
	float projected_radius = radius / -ec_depth(tc_depths);

	vec2 inverted_random_texture_size = 1.0 / vec2(textureSize(random_texture, 0));
	vec2 tc_random_texture = gl_FragCoord.xy * inverted_random_texture_size;

	vec3 random_direction = texture(random_texture, tc_random_texture).xyz;
	random_direction = normalize(random_direction * 2.0 - 1.0);

	for (int i = 0; i < samples; ++i)
	{
		vec2 sample_random_direction = texture(random_texture, vec2(float(i) * inverted_random_texture_size.x, float(i / textureSize(random_texture, 0).x) * inverted_random_texture_size.y)).xy;
		sample_random_direction = sample_random_direction * 2.0 - 1.0;
		vec2 sample_random_direction_negated = -sample_random_direction;
		
		vec2 tc_sample_1 = tc_depths + sample_random_direction * projected_radius;
		vec2 tc_sample_2 = tc_depths + sample_random_direction_negated * projected_radius;

		float ec_sample_1_depth = ec_depth(tc_sample_1);
		float ec_sample_2_depth = ec_depth(tc_sample_2);
		float depth_difference_1 = ec_position_depth - ec_sample_1_depth;
		float depth_difference_2 = ec_position_depth - ec_sample_2_depth;
		float samples_sphere_height = sphere_height(tc_sample_1, radius);
		float samples_sphere_depth_inverted = 1.0 / (2.0 * samples_sphere_height);

		float volume_ratio_1 = (samples_sphere_height - depth_difference_1) * samples_sphere_depth_inverted;
		float volume_ratio_2 = (samples_sphere_height - depth_difference_2) * samples_sphere_depth_inverted;

		bool sample_1_valid = lower_bound <= volume_ratio_1 && upper_bound >= volume_ratio_1;
		bool sample_2_valid = lower_bound <= volume_ratio_2 && upper_bound >= volume_ratio_2;

		// Should evaluate to a conditional assignment (no branching )
		if (sample_1_valid || sample_2_valid)
		{
			// If the sample is	valid then use it. If not, then use the other one in the pair (inverted).
			ambient_occlusion.a += (sample_1_valid) ? volume_ratio_1 : 1.0 - volume_ratio_2;
			ambient_occlusion.a += (sample_2_valid) ? volume_ratio_2 : 1.0 - volume_ratio_1;
		}
		else
		{
			// Not 0.5 but 1.0 because both samples were invalid.
			ambient_occlusion.a += 1.0;
		}
	}
	
	ambient_occlusion.a /= float(samples * 2.0 + 1.0);
	ambient_occlusion.a = 1.0 - ambient_occlusion.a;
}



