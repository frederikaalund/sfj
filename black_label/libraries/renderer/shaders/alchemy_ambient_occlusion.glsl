uniform sampler2D depths;
uniform sampler2D wc_normals;
uniform sampler2D random_texture;

uniform vec3 wc_camera_eye_position;
uniform float z_far;

uniform vec2 tc_window;

uniform mat4 projection_matrix;
uniform mat4 view_projection_matrix;
uniform mat4 inverse_view_projection_matrix;


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



void main()
{
	vec2 tc_depths = gl_FragCoord.xy / tc_window;
	vec3 wc_normal = texture(wc_normals, tc_depths).xyz;
	float ndc_linear_depth = -ec_depth(tc_depths) / z_far;
	vec3 wc_position = wc_camera_eye_position + vertex.wc_camera_ray_direction * ndc_linear_depth;

	ambient_occlusion.a = 0.0;
	
	const int base_samples = 0; 
	const int min_samples = 64;
	const float radius = 10.0;
	const float projection_factor = 0.75;
	const float bias = 1.0;
	const float sigma = 2.0;
	const float epsilon = 0.00001;

	int samples = max(int(base_samples / (1.0 + base_samples * ndc_linear_depth)), min_samples);

	mat4 inverse_view_projection_matrix = inverse(view_projection_matrix);
	float projected_radius = radius * projection_factor / -ec_depth(tc_depths);

	vec2 inverted_random_texture_size = 1.0 / vec2(textureSize(random_texture, 0));
	vec2 tc_random_texture = gl_FragCoord.xy * inverted_random_texture_size;

	vec3 random_direction = texture(random_texture, tc_random_texture).xyz;
	random_direction = normalize(random_direction * 2.0 - 1.0);

	for (int i = 0; i < samples; ++i)
	{
		vec2 sample_random_direction = texture(random_texture, vec2(float(i) * inverted_random_texture_size.x, float(i / textureSize(random_texture, 0).x) * inverted_random_texture_size.y)).xy;
		sample_random_direction = sample_random_direction * 2.0 - 1.0;
		
		vec3 tc_sample;
		tc_sample.xy = tc_depths + sample_random_direction * projected_radius;
		tc_sample.z = tc_depth(tc_sample.xy);
		vec3 ndc_sample = tc_sample * 2.0 - 1.0;		
		vec4 temporary = inverse_view_projection_matrix * vec4(ndc_sample, 1.0);
		vec3 wc_sample = temporary.xyz / temporary.w;

		vec3 v = wc_sample - wc_position;

		ambient_occlusion.a += max(0.0, dot(v, wc_normal) - bias) / (dot(v, v) + epsilon);
	}
	
	ambient_occlusion.a = max(0.0, 1.0 - 2.0 * sigma / float(samples) * ambient_occlusion.a);
}



