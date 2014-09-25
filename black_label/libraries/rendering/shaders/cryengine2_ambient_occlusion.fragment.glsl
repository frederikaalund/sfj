uniform sampler2D depths;
uniform sampler2D random_texture;

uniform vec3 wc_view_eye_position;
uniform float z_far;

uniform vec2 tc_window;


uniform mat4 projection_matrix;





struct vertex_data
{
	vec3 wc_view_ray_direction;
};
noperspective in vertex_data vertex;

out vec4 ambient_occlusion;



float ec_depth( in vec2 tc )
{
	float buffer_z = texture(depths, tc).x;
	return projection_matrix[3][2] / (-2.0 * buffer_z + 1.0 - projection_matrix[2][2]);
}



void main()
{
	vec2 tc_depths = gl_FragCoord.xy / tc_window;

	float ec_depth_negated = -ec_depth(tc_depths);
	vec3 wc_position = wc_view_eye_position + vertex.wc_view_ray_direction * ec_depth_negated / z_far;

	ambient_occlusion.a = 0.0f;
	const float radius = 10.0;	
	const int samples = 200;

	float projection_scale_xy = 1.0 / ec_depth_negated;
	float projection_scale_z = 100.0 / z_far * projection_scale_xy;

	float scene_depth = texture(depths, tc_depths).x;


	vec2 inverted_random_texture_size = 1.0 / vec2(textureSize(random_texture, 0));
	vec2 tc_random_texture = gl_FragCoord.xy * inverted_random_texture_size;

	vec3 random_direction = texture(random_texture, tc_random_texture).xyz;

	random_direction = normalize(random_direction * 2.0 - 1.0);

	for (int i = 0; i < samples; ++i)
	{
		vec3 sample_random_direction = texture(random_texture, vec2(float(i) * inverted_random_texture_size.x, float(i / textureSize(random_texture, 0).x) * inverted_random_texture_size.y)).xyz;
		sample_random_direction = sample_random_direction * 2.0 - 1.0;
		sample_random_direction = reflect(sample_random_direction, random_direction);

		vec3 tc_sample_pos = vec3(tc_depths.xy, scene_depth)
			+ vec3(sample_random_direction.xy * projection_scale_xy, sample_random_direction.z * scene_depth * projection_scale_z) * radius;

		float sample_depth = texture(depths, tc_sample_pos.xy).x;

		ambient_occlusion.a += float(sample_depth > tc_sample_pos.z);
	}
	
	ambient_occlusion.a /= float(samples);
}
