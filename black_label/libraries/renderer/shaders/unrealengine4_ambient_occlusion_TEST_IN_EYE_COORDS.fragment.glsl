uniform sampler2D depths;
uniform sampler2D wc_normals;
uniform sampler2D random_texture;

uniform vec3 wc_camera_eye_position;
uniform float z_far;

uniform vec2 tc_window;

uniform mat4 view_matrix;
uniform mat4 projection_matrix;
uniform mat4 view_projection_matrix;



struct vertex_data
{
	vec3 wc_camera_ray_direction;
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

// Reference: http://stackoverflow.com/a/3380723/554283
float acos_approximation( float x ) 
{
   return (-0.69813170079773212 * x * x - 0.87266462599716477) * x + 1.5707963267948966;
}

float calculate_angle( 
	in vec2 direction,
	in vec2 tc_depths,
	in float radius,
	in float projected_radius,
	in vec3 ec_position,
	in vec3 ec_normal,
	in mat4 inverse_vpm,
	in float bias,
	inout float pair_weight )
{
	vec2 tc_sample = tc_depths + direction * projected_radius;
	vec3 ec_sample = vec3(direction * radius, ec_depth(tc_sample) - ec_position.z);

	vec3 s = normalize(ec_sample);
	vec3 v = normalize(vec3(0.0, 0.0, 1.0));

	float vn = dot(v, ec_normal);
	float vs = dot(v, s);
	float sn = dot(s, ec_normal);

	// Cap to tangent plane
	vec3 tangent = normalize(s - sn * ec_normal);
	float cos_angle = (0.0 <= sn) ? vs : dot(v, tangent);

	// Invalid samples are approximated by looking at the partner in the pair of samples
	if (20.0 < ec_sample.z)
	{
		pair_weight -= 0.5;
		cos_angle = max(dot(v, -s), 0.0);
	}

	return max(acos(cos_angle - bias), 0.0);
}



void main()
{
	vec2 tc_depths = gl_FragCoord.xy / tc_window;
	vec3 wc_normal = texture(wc_normals, tc_depths).xyz;
	float ndc_linear_depth = -ec_depth(tc_depths) / z_far;
	vec3 wc_position = wc_camera_eye_position + vertex.wc_camera_ray_direction * ndc_linear_depth;

	mat3 normal_matrix = mat3(transpose(inverse(view_matrix)));
	vec3 ec_position = (view_matrix * vec4(wc_position, 1.0)).xyz;
	vec3 ec_normal = normal_matrix * wc_normal;

	ambient_occlusion.a = 0.0;
	
	const int base_samples = 32; 
	const int min_samples = 32; // Should be around 8
	const float radius = 10.0;
	const float k = 4.0;
	const float bias = 0.02;
	const float projection_factor = 0.75;

	int samples = max(int(base_samples / (1.0 + base_samples * ndc_linear_depth)), min_samples);

	mat4 inverse_vpm = inverse(view_projection_matrix); // TODO: Put in uniform.
	float projected_radius = radius * projection_factor / -ec_depth(tc_depths);

	vec2 inverted_random_texture_size = 1.0 / vec2(textureSize(random_texture, 0));
	vec2 tc_random_texture = gl_FragCoord.xy * inverted_random_texture_size;

	vec3 random_direction = texture(random_texture, tc_random_texture).xyz;
	random_direction = normalize(random_direction * 2.0 - 1.0);

	float weight_sum = 0.0001;
	for (int i = 0; i < samples; ++i)
	{
		vec2 sample_random_direction = texture(random_texture, vec2(float(i) * inverted_random_texture_size.x, float(i / textureSize(random_texture, 0).x) * inverted_random_texture_size.y)).xy;
		sample_random_direction = sample_random_direction * 2.0 - 1.0;
		vec2 sample_random_direction_negated = -sample_random_direction;	
		
		float pair_weight = 1.0;
		
		float angle_sum = 
			calculate_angle(sample_random_direction, tc_depths, radius, projected_radius, ec_position, ec_normal, inverse_vpm, bias, pair_weight)
			+ calculate_angle(sample_random_direction_negated, tc_depths, radius, projected_radius, ec_position, ec_normal, inverse_vpm, bias, pair_weight);

		ambient_occlusion.a += angle_sum * pair_weight;
		weight_sum += pair_weight;
	}
	
	ambient_occlusion.a /= weight_sum * PI;
	ambient_occlusion.a = pow(ambient_occlusion.a, k);
}



