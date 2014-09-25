#define USE_RANDOM_DIRECTION 1

const int poisson_disc_size = 128;
uniform vec2 poisson_disc[poisson_disc_size];

uniform sampler2D depths;
uniform sampler2D wc_normals;
uniform sampler2D random;

uniform vec3 wc_view_eye_position;
uniform float z_far;

uniform ivec2 window_dimensions;

uniform mat4 view_matrix;
uniform mat4 projection_matrix;
uniform mat4 view_projection_matrix;
uniform mat4 inverse_view_projection_matrix;



struct vertex_data
{
	vec3 wc_view_ray_direction;
};
noperspective in vertex_data vertex;

layout(location = 0) out vec4 ambient_occlusion;




////////////////////////////////////////////////////////////////////////////////
/// Utility Functions
////////////////////////////////////////////////////////////////////////////////
vec2 get_one_over_tan_half_fov( in mat4 projection_matrix )
{ return vec2(projection_matrix[0][0], projection_matrix[1][1]); }

float get_tc_z( 
	in sampler2D sampler, 
	in vec2 tc_position )
{ return texture(sampler, tc_position).z; }

float get_tc_z( 
	in sampler2D sampler, 
	in vec2 tc_position, 
	in vec2 tc_offset )
{ return get_tc_z(sampler, tc_position + tc_offset); }

float get_ec_z( 
	in float tc_z,
	in mat4 projection_matrix )
{ return projection_matrix[3][2] / (-2.0 * tc_z + 1.0 - projection_matrix[2][2]); }

float get_ec_z( 
	in sampler2D sampler, 
	in vec2 tc_position,
	in mat4 projection_matrix)
{ return get_ec_z(get_tc_z(sampler, tc_position), projection_matrix); }

vec3 get_ec_position( in vec2 tc_position, in float ec_position_z, in mat4 projection_matrix )
{
	vec2 one_over_tan_half_fov = get_one_over_tan_half_fov(projection_matrix);
	vec2 tan_half_fov = 1.0 / one_over_tan_half_fov;

	vec2 ndc_position = tc_position * vec2(2.0) - vec2(1.0);
	vec2 cc_position = ndc_position * -ec_position_z;
	return vec3(tan_half_fov * cc_position, ec_position_z);
}

vec3 get_ec_position( in sampler2D sampler, in vec2 tc_position, in mat4 projection_matrix )
{
	float ec_position_z = get_ec_z(sampler, tc_position, projection_matrix);
	return get_ec_position(tc_position, ec_position_z, projection_matrix);
}

vec2 get_tc_length( in float ec_length, in float ec_position_z, in mat4 projection_matrix )
{
	vec2 one_over_tan_half_fov = get_one_over_tan_half_fov(projection_matrix);
	return 0.5 * ec_length * one_over_tan_half_fov / -ec_position_z;
}


////////////////////////////////////////////////////////////////////////////////
/// Trigonometric Functions
////////////////////////////////////////////////////////////////////////////////
float tan_to_sin( in float x )
{
    return x * pow(x * x + 1.0, -0.5);
}


vec3 minimum_difference( in vec3 p, in vec3 p_right, in vec3 p_left )
{
    vec3 v1 = p_right - p;
    vec3 v2 = p - p_left;
    return (dot(v1, v1) < dot(v2, v2)) ? v1 : v2;
}

vec3 tangent_eye_pos( in sampler2D sampler, in vec2 tc, in vec4 tangentPlane, in mat4 projection_matrix )
{
    // view vector going through the surface point at tc
    vec3 V = get_ec_position(sampler, tc, projection_matrix);
    float NdotV = dot(tangentPlane.xyz, V);
    // intersect with tangent plane except for silhouette edges
    if (NdotV < 0.0) V *= (tangentPlane.w / NdotV);
    return V;
}



void main()
{
	vec2 tc_position = gl_FragCoord.xy / window_dimensions;
	vec3 ec_position = get_ec_position(depths, tc_position, projection_matrix);
	vec3 wc_normal = texture(wc_normals, tc_position).xyz;
	vec3 ec_normal = transpose(inverse(mat3(view_matrix))) * wc_normal;

	ambient_occlusion.a = 0.0;
	
	const int base_samples = 0;
	const int min_samples = 16;
	const float ec_radius = 100.0;
	const float ec_radius_squared = ec_radius * ec_radius;
	const float bias = 0.4;

	const int samples = min_samples;

	vec2 tc_radius = get_tc_length(ec_radius, ec_position.z, projection_matrix);
	vec2 sc_radius = tc_radius * window_dimensions;


	if (sc_radius.x < 1.0)
	{
		ambient_occlusion.a = 1.0;
		return;
	}

	// Stepping
	const int max_steps = 4;
	int steps = min(int(sc_radius.x), max_steps);


	vec3 random_direction = texture(random, tc_position).xyz;
	random_direction = normalize(random_direction * 2.0 - 1.0);

 	float angle_step = 2.0 * PI / float(samples);
 	float uniform_distribution_random = texture(random, tc_position).x;
	float alpha = uniform_distribution_random * PI * 2.0;
	mat2 random_rotation = mat2(cos(alpha), sin(alpha), -sin(alpha), cos(alpha));

	vec3 bent_normal = vec3(0.0);

    vec2 depths_size = textureSize(depths, 0);
	vec2 depths_size_inversed = vec2(1.0) / depths_size;

    vec3 p_right, p_left, p_top, p_bottom;
    vec4 tangentPlane = vec4(ec_normal, dot(ec_position, ec_normal));
    p_right = tangent_eye_pos(depths, tc_position + vec2(depths_size_inversed.x, 0.0), tangentPlane, projection_matrix);
    p_left = tangent_eye_pos(depths, tc_position + vec2(-depths_size_inversed.x, 0.0), tangentPlane, projection_matrix);
    p_top = tangent_eye_pos(depths, tc_position + vec2(0.0, depths_size_inversed.y), tangentPlane, projection_matrix);
    p_bottom = tangent_eye_pos(depths, tc_position + vec2(0.0, -depths_size_inversed.y), tangentPlane, projection_matrix);
    vec3 dp_du = minimum_difference(ec_position, p_right, p_left);
    vec3 dp_dv = minimum_difference(ec_position, p_top, p_bottom) * (depths_size.y * depths_size_inversed.x);

	for (int i = 0; i < samples; ++i)
	{
#if USE_RANDOM_DIRECTION
		vec2 tc_sample_direction = random_rotation * poisson_disc[i];
#else
		vec2 tc_sample_direction = vec2(cos(float(i) * angle_step), sin(float(i) * angle_step));
#endif
	    // Tangent vector
    	vec3 ec_tangent = tc_sample_direction.x * dp_du + tc_sample_direction.y * dp_dv;
		float tan_tangent_angle = ec_tangent.z / length(ec_tangent.xy) + tan(bias);

		// Stepping
		vec2 tc_step_size = tc_sample_direction * tc_radius / float(steps);
		vec2 random_offset = tc_step_size * uniform_distribution_random;

		// Initialize horizon angle to the tangent angle
		float tan_horizon_angle = tan_tangent_angle;
		float sin_horizon_angle = tan_to_sin(tan_horizon_angle);

		for (float j = 0.0; j < float(steps); j += 1.0)
		{
			vec2 tc_sample = vec2(tc_position + tc_step_size * j + random_offset);
			vec3 ec_sample = get_ec_position(depths, tc_sample, projection_matrix);
			vec3 ec_ray = ec_sample - ec_position;
			float ec_ray_length_squared = dot(ec_ray, ec_ray);
			float tan_sample_angle = ec_ray.z / length(ec_ray.xy);

			bool in_hemisphere = ec_radius_squared >= ec_ray_length_squared;
			bool new_occluder = tan_sample_angle > tan_horizon_angle;

			if (in_hemisphere && new_occluder)
			{
				float sin_sample_angle = tan_to_sin(tan_sample_angle);
				float falloff = 1.0 - ec_ray_length_squared / ec_radius_squared;
				float horizon = sin_sample_angle - sin_horizon_angle;
				ambient_occlusion.a += horizon * falloff;
				tan_horizon_angle = tan_sample_angle;
				sin_horizon_angle = sin_sample_angle;

				bent_normal += normalize(ec_ray) * falloff;
			}
		}
	}

	bent_normal = normalize(bent_normal) * 0.5 + 0.5;
	ambient_occlusion.rgb = bent_normal;
	ambient_occlusion.a /= samples;
	ambient_occlusion.a = 1.0 - ambient_occlusion.a;
}



