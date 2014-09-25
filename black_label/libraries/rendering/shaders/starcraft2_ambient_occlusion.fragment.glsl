uniform sampler2D depths;
uniform sampler2D wc_normals;
uniform sampler2D random_texture;

uniform vec3 wc_view_eye_position;
uniform float z_far;

uniform vec2 tc_window;

uniform mat4 view_matrix;
uniform mat4 projection_matrix;
uniform mat4 view_projection_matrix;



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
	vec3 wc_normal = texture(wc_normals, tc_depths).xyz;
	vec3 wc_position = wc_view_eye_position + vertex.wc_view_ray_direction * -ec_depth(tc_depths) / z_far;

	ambient_occlusion.a = 0.0;
	//vec3 bent_normal = vec3(0.0);
	const float radius = 10.0;	
	const int samples = 80; 

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
		sample_random_direction = faceforward(sample_random_direction, sample_random_direction, -wc_normal);
		
		vec3 wc_sample = wc_position + sample_random_direction * radius;
		vec3 ec_sample = (view_matrix * vec4(wc_sample, 1.0)).xyz;
		vec4 cc_sample = view_projection_matrix * vec4(wc_sample, 1.0);
		vec3 ndc_sample = cc_sample.xyz / cc_sample.w;
		vec2 tc_sample = (ndc_sample.xy + vec2(1.0)) * 0.5;

		float scene_depth = ec_depth(tc_sample);
		float sample_depth = ec_sample.z;

		float depth_difference = scene_depth - sample_depth;

		// CryEngine2 rho
		//float rho = (depth_difference <= 0.0 || depth_difference > radius) ? 1.0 : 0.0;

		float rho = clamp((depth_difference - radius) / depth_difference, 0.0, 1.0);
		ambient_occlusion.a += rho;
		//bent_normal += normalize(sample_random_direction) * rho;
	}
	
	//bent_normal = normalize(bent_normal) * 0.5 + 0.5;
	//ambient_occlusion.rgb = bent_normal;
	ambient_occlusion.a /= float(samples);
}



