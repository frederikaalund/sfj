#extension GL_NV_shader_buffer_load: enable
#extension GL_NV_gpu_shader5: enable
#extension GL_EXT_shader_image_load_store: enable

#define USE_PHYSICAL_SOFT_SHADOWS
#define USE_OREN_NAYAR_DIFFUSE_REFLECTANCE

const int poisson_disc_size = 16;
uniform vec2 poisson_disc[poisson_disc_size];

uniform sampler2D depths;
uniform sampler2D wc_normals, wc_positions;
uniform sampler2D albedos;
uniform sampler2D random;
uniform sampler2D ambient_occlusion;
uniform sampler2D shadow_map_0, shadow_map_1;

uniform samplerBuffer lights;
#ifdef USE_TILED_SHADING
uniform isamplerBuffer light_grid;
uniform isamplerBuffer light_index_list;
uniform int tile_size;
#else
uniform int lights_size;
#endif

uniform ivec2 window_dimensions;
uniform ivec2 grid_dimensions;

uniform vec3 wc_view_eye_position;
uniform float z_near, z_far;

uniform mat4 projection_matrix;



struct light_type {
	mat4 projection_matrix, view_projection_matrix;
	vec4 wc_direction;
	float radius;
};

layout(std140) uniform light_block
{ light_type light; };



struct oit_data {
	uint32_t next, compressed_diffuse;
	float depth;
};
readonly restrict layout (std430) buffer data_buffer
{ oit_data data[]; };
readonly restrict layout (std430) buffer debug_view_buffer
{ uint32_t debug_view[]; };
struct color_data {
	uint32_t r, g, b;
};
readonly restrict layout (std430) buffer photon_splat_buffer
{ color_data photon_splats[]; };




struct vertex_data
{
	vec3 wc_view_ray_direction;
};
noperspective in vertex_data vertex;

layout(location = 0) out vec4 color;
//out vec4 overbright;



////////////////////////////////////////////////////////////////////////////////
/// Utility Functions
////////////////////////////////////////////////////////////////////////////////
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



////////////////////////////////////////////////////////////////////////////////
/// Shading functions
////////////////////////////////////////////////////////////////////////////////
float calculate_shadow_coefficient( 
	in sampler2D shadow_map, 
	in light_type light,
	in vec3 wc_position, 
	in vec3 wc_normal,
	in vec2 tc_window,
	in float uniform_distribution_random )
{
	// Normal bias (virtually translate scene along normal vectors)
	float cos_alpha = clamp(dot(wc_normal, light.wc_direction.xyz), 0.0, 1.0);
    float normal_bias_coefficient = sqrt(1.0 - cos_alpha * cos_alpha); // <=> sin(acos(wc_normal, light_direction));
	wc_position += wc_normal * normal_bias_coefficient;

	// Coordinate transformations
	vec4 cc_position = light.view_projection_matrix * vec4(wc_position, 1.0);
	vec3 ndc_position = cc_position.xyz / cc_position.w;
	vec3 tc_position = (ndc_position + vec3(1.0)) * 0.5;
	float ec_position_z = get_ec_z(tc_position.z, light.projection_matrix);

	// Shadow behind light
	if (ec_position_z > 0.0) return 0.0;

	// Sample count as a function of light size
	int chi = int(light.radius / 12.5);
	int samples = clamp(chi * chi, 4, poisson_disc_size);

	// Random 2D rotation
	float alpha = uniform_distribution_random * PI * 2.0;
	mat2 random_rotation = mat2(cos(alpha), sin(alpha), -sin(alpha), cos(alpha));

	// Occluder search radius
	vec2 inverted_shadow_map_size = 1.0 / vec2(textureSize(shadow_map, 0));
	float tc_occluder_search_radius = light.radius * inverted_shadow_map_size;

#ifdef USE_PHYSICAL_SOFT_SHADOWS
	// Occluder search
	int occluder_count = 0;
	float ec_occluder_z = 0.0;
	for (int i = 0; i < samples; ++i)
	{
		vec2 tc_offset = random_rotation * poisson_disc[i] * tc_occluder_search_radius;

		float tc_occluder_sample_z = get_tc_z(shadow_map, tc_position.xy, tc_offset);
		float ec_occluder_sample_z = get_ec_z(tc_occluder_sample_z, light.projection_matrix);

		float ec_occluder_distance = ec_occluder_sample_z - ec_position_z;
		if (0.0 < ec_occluder_distance)
		{
			ec_occluder_z += ec_occluder_sample_z;
			++occluder_count;
		}
	}
	// Return if no occluders were found
	if (0 == occluder_count) return 1.0;
	// Average occluder position
	ec_occluder_z /= occluder_count;
	// Distance from occluder to shading position
	float ec_occluder_distance = ec_occluder_z - ec_position_z;

	// Penumbra ratio relative to the light size (Calculated using similar triangles)
	float penumbra_ratio = ec_occluder_distance / ec_occluder_z;
#else
	const float penumbra_ratio = 0.05;
#endif
	// Sample for shadows in the penumbra
	float tc_shadow_sampling_radius = tc_occluder_search_radius * penumbra_ratio;

	// Shadow sampling (Using percentage-closer filtering)
	float shadow_coefficient = 0.0;
	for (int i = 0; i < samples; ++i)
	{
		vec2 tc_offset = random_rotation * poisson_disc[i] * tc_shadow_sampling_radius;
		float tc_occluder_sample_z = get_tc_z(shadow_map, tc_position.xy, tc_offset);

		if (tc_occluder_sample_z > tc_position.z)
			shadow_coefficient += 1.0;
	}
	return shadow_coefficient / float(samples);
}



vec3 fresnel_schlick( in vec3 specular_color, in vec3 wc_direction, in vec3 wc_half_angle )
{
	return specular_color + (vec3(1.0) - specular_color) 
		* pow(1.0 - max(dot(wc_direction, wc_half_angle), 0.0), 5.0);
}



vec4 get_reflected_light( 
	in vec3 wc_position, 
	in vec3 wc_normal,
	in vec3 wc_view_direction,
	in vec3 albedo,
	in vec3 bent_normal,
	in float roughness )
{
	vec3 wc_reflection = reflect(wc_view_direction, wc_normal);
	// View direction is more convenient to store negated
	wc_view_direction = -wc_view_direction;
	// Common terms in the BRDFs
	float a = roughness * roughness;
	float a_squared = a * a;
	vec3 material_specular_color = vec3(1.0, 1.0, 1.0);

	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);
	const int count = 1;

	for (int l = 0; l < count; ++l)
	{
		int light_id = l;

		#define LIGHT_STRUCT_SIZE 6
		
		vec3 wc_light_position = vec3(texelFetch(lights, light_id * LIGHT_STRUCT_SIZE).x, texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 1).x, texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 2).x);
		vec3 light_color = vec3(texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 3).x, texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 4).x, texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 5).x);

		float light_radius = 50.0;
		light_color *= 10000000.0;
		

		// Representative point approximation of spherical lights
		// Reference: http://www.unrealengine.com/files/downloads/2013SiggraphPresentationsNotes.pdf
		vec3 wc_light_direction_unnormalized = wc_light_position - wc_position;
		vec3 wc_center_to_reflection = dot(wc_light_direction_unnormalized, wc_reflection) 
			* wc_reflection - wc_light_direction_unnormalized;
		vec3 wc_representative_point = wc_light_position 
			+ wc_light_direction_unnormalized + wc_center_to_reflection 
			* clamp(light_radius / length(wc_center_to_reflection), 0.0, 1.0);

		// Common BRDF parameters
		vec3 wc_light_direction = wc_representative_point - wc_position;
		float wc_light_distance = length(wc_light_direction);
		wc_light_direction /= wc_light_distance;
		// The half angle vector (h)
		vec3 wc_half_angle = normalize(wc_light_direction + wc_view_direction);
		// Common dot products
		float dotNH = dot(wc_normal, wc_half_angle);
		float dotNV = dot(wc_normal, wc_view_direction);
		float dotNL = dot(wc_normal, wc_light_direction);

		// Falloff
		float falloff = 1.0 / (wc_light_distance * wc_light_distance);

		// Representative point normalization
		float a_prime = clamp(a + light_radius / (3.0 * wc_light_distance), 0.0, 1.0);
		float a_ratio = a / a_prime;
		float specular_sphere_normalization = a_ratio * a_ratio;

		// Specularly reflected light
		// Reference: http://www.unrealengine.com/files/downloads/2013SiggraphPresentationsNotes.pdf
		float chi = PI * (dotNH * dotNH * (a_squared - 1.0) + 1.0);
		float D = a_squared / (chi * chi);
		float k = (roughness + 1.0) * (roughness + 1.0);
		float Gv = dotNV / (dotNV * (1.0 - k) + k);
		float Gl = dotNL / (dotNL * (1.0 - k) + k);
		float G = Gv * Gl;
		vec3 F = fresnel_schlick(material_specular_color, wc_view_direction, wc_half_angle);
		vec3 specularly_reflected_light = (D * F * G) / (4.0 * dotNL * dotNV) 
			* specular_sphere_normalization;

		// Diffusely reflected light
		vec3 diffusely_reflected_light = albedo * max(dotNL, 0.0)
			* (vec3(1.0) - specularly_reflected_light);

#ifdef USE_OREN_NAYAR_DIFFUSE_REFLECTANCE
		// Reference: http://content.gpwiki.org/index.php/D3DBook:(Lighting)_Oren-Nayar
		float acos_dotNV = acos(dotNV);
		float acos_dotNL = acos(dotNL);
		float alpha = max(acos_dotNV, acos_dotNL);
		float beta = min(acos_dotNV, acos_dotNL);
		float gamma = dot(wc_view_direction - wc_normal * dotNV, 
			wc_light_direction - wc_normal * dotNL);

		float A = 1.0 - 0.5 * a / (a + 0.33);
		float B = 0.45 * a / (a + 0.09);

		diffusely_reflected_light *= (A + (B * max(0.0, gamma) * sin(alpha) * tan(beta)));
#endif

		// Total reflected light
		result.rgb += (specularly_reflected_light + diffusely_reflected_light) 
			* light_color * falloff;
	}

	return result;
}






vec4 decompress(uint32_t rgba)
{
  return vec4( float((rgba>>24u)&255u),float((rgba>>16u)&255u),float((rgba>>8u)&255u),float(rgba&255u) ) / 255.0;
}

vec4 blend(vec4 clr, vec4 srf)
{
  return clr + (1.0 - clr.w) * vec4(srf.xyz * srf.w , srf.w);  
}

vec4 oit() {
	// Get the head node
	uint32_t heads_index = uint32_t(gl_FragCoord.x - 0.5) + uint32_t(gl_FragCoord.y - 0.5) * window_dimensions.xs;
	uint32_t current = data[heads_index].next;

	// Constants
	const int max_list_length = 100;

	vec4 color = vec4(0.0);
	int list_length = 0;
	while (0 != current && list_length < max_list_length) {
		//color = blend(decompress(data[current].compressed_diffuse), color);
		color = blend(decompress(data[current].compressed_diffuse), color);

		current = data[current].next;
		++list_length;
	}

	if (list_length == max_list_length)
		return vec4(1.0, 0.0, 0.0, 0.0);

	return color;
}




vec4 get_view_color( in int view_id ) {
	const vec4 colors[3] = {vec4(0.0, 1.0, 0.0, 0.0), vec4(0.0, 0.0, 1.0, 0.0), vec4(1.0, 0.0, 0.0, 0.0)};
	return colors[view_id % 3];
}


vec4 get_debug_view() {
	uint32_t index = uint32_t(gl_FragCoord.x) + uint32_t(gl_FragCoord.y) * window_dimensions.x;

	if (0 < debug_view[index])
		return get_view_color(int(debug_view[index] - 1));
	return vec4(0.0);
}


vec4 get_photon_splats() {
	uint32_t index = uint32_t(gl_FragCoord.x) + uint32_t(gl_FragCoord.y) * window_dimensions.x;
	color_data color = photon_splats[index];
	return vec4(float(color.r) / 255.0, float(color.g) / 255.0, float(color.b) / 255.0, 1.0);
}





struct view_type {
	mat4 view_matrix, projection_matrix, view_projection_matrix;
	vec4 eye;
	vec4 right, forward, up;
	ivec2 dimensions;
};

layout(std140) uniform user_view_block
{ view_type user_view; };

layout(std140) uniform view_block
{ view_type views[9]; };

layout(std140) uniform data_offset_block
{ uvec4 data_offsets[9]; };


vec4 draw_view_as_points( in int view_id, in vec3 wc_position ) {
	view_type view = views[view_id];
	uint32_t data_offset = data_offsets[view_id];

	// Coordinate transformations
	vec4 cc_position = view.view_projection_matrix * vec4(wc_position, 1.0);
	if (cc_position.x > cc_position.w || cc_position.x < -cc_position.w
		|| cc_position.y > cc_position.w || cc_position.y < -cc_position.w
		|| cc_position.z > cc_position.w || cc_position.z < -cc_position.w)
		return vec4(0.1, 0.1, 0.1, 0.0);
	vec3 ndc_position = cc_position.xyz / cc_position.w;
	vec3 tc_position = (ndc_position + vec3(1.0)) * 0.5;
	uvec2 window_position = uvec2(tc_position.xy * view.dimensions);

	vec3 wc_eye = view.eye.xyz;
	vec3 right = view.right.xyz;
	vec3 forward = view.forward.xyz;
	vec3 up = view.up.xyz;

	// Othographic
	const float right_scale = 1000.0;
	const float top_scale = 1000.0;


//float depth = -(view.view_matrix * vec4(wc_position, 1.0)).z;
//vec3 direction = (
//	forward * depth
//	+ right * right_scale * ndc_position.x
//	+ up * top_scale * ndc_position.y);
//vec3 wc_sample_position = wc_eye + direction;
//return vec4(wc_position / 1000.0, 0.0);
//return vec4(wc_sample_position / 1000.0, 0.0);



	// Get the head node
	uint32_t heads_index = data_offset + window_position.x + window_position.y * view.dimensions.x;
	uint32_t current = data[heads_index].next;

	if (0 == current)
		return vec4(1.0, 1.0, 1.0, 0.0);

	const int max_list_length = 200;
	const float distance_threshold = 0.1;
	float min_distance = distance_threshold;
	int list_length = 0;
	while (0 != current && list_length < max_list_length) {
		float depth = data[current].depth;
		vec3 direction = (
			forward * depth
			+ right * right_scale * ndc_position.x
			+ up * top_scale * ndc_position.y);
		vec3 wc_sample_position = wc_eye + direction;

		float sample_distance = distance(wc_sample_position, wc_position);
		if (sample_distance < min_distance) {
			min_distance = sample_distance;
		}

		current = data[current].next;
		++list_length;
	}

	if (max_list_length == list_length)
		return vec4(1.0, 1.0, 1.0, 0.0);

	if (distance_threshold > min_distance)
		return get_view_color(view_id);

	return vec4(0.0);
}

vec4 sampling_test( in vec3 wc_position ) {
	vec4 result = vec4(0.0);
	for (int i = 0; i < 1; ++i)
		result += draw_view_as_points(i, wc_position);
	return result;
}





float visibility( in vec3 wc_view_direction, in vec3 wc_normal, in float occluder_distance ) {
	float weight = max(dot(wc_view_direction, wc_normal), 0.0);
	return 1.0 - weight * min(400.0 / (occluder_distance * occluder_distance), 1.0);	
}

float visibility_both_directions( in int view_id, in vec3 wc_position, in vec3 wc_normal ) {
	// Normal bias (virtually translate scene along normal vectors)
	float cos_alpha = clamp(dot(wc_normal, light.wc_direction.xyz), 0.0, 1.0);
    float normal_bias_coefficient = sqrt(1.0 - cos_alpha * cos_alpha); // <=> sin(acos(wc_normal, light_direction));
	wc_position += wc_normal * normal_bias_coefficient * 2.0;

	view_type view = views[view_id];
	uint32_t data_offset = data_offsets[view_id];

	// Coordinate transformations
	vec4 cc_position = view.view_projection_matrix * vec4(wc_position, 1.0);
	if (cc_position.x > cc_position.w || cc_position.x < -cc_position.w
		|| cc_position.y > cc_position.w || cc_position.y < -cc_position.w
		|| cc_position.z > cc_position.w || cc_position.z < -cc_position.w)
		return vec4(1.0);
	vec3 ndc_position = cc_position.xyz / cc_position.w;
	vec3 tc_position = (ndc_position + vec3(1.0)) * 0.5;
	uvec2 window_position = uvec2(tc_position.xy * view.dimensions);

	vec3 wc_eye = view.eye.xyz;
	vec3 right = view.right.xyz;
	vec3 forward = view.forward.xyz;
	vec3 up = view.up.xyz;

	// Othographic
	const float right_scale = 1000.0;
	const float top_scale = 1000.0;

	// Get the head node
	uint32_t heads_index = data_offset + window_position.x + window_position.y * view.dimensions.x;
	uint32_t current = data[heads_index].next;

	const int max_list_length = 200;
	const float distance_threshold = 999999.0;
	float min_distance = distance_threshold;
	float previous_distance = min_distance;
	float next_distance = min_distance;
	bool get_next = false;

	int list_length = 0;
	while (0 != current && list_length < max_list_length) {
		float depth = data[current].depth;
		vec3 direction = (
			forward * (depth)
			+ right * right_scale * ndc_position.x
			+ up * top_scale * ndc_position.y);
		vec3 wc_sample_position = wc_eye + direction;

		float sample_distance = distance(wc_sample_position, wc_position);

		if (get_next) {
			get_next = false;
			next_distance = sample_distance;
		}

		if (sample_distance < min_distance) {
			previous_distance = min_distance;
			min_distance = sample_distance;
			get_next = true;
		}

		current = data[current].next;
		++list_length;
	}
	if (get_next)
		next_distance = distance_threshold;

	float V1 = visibility(view.forward.xyz, wc_normal, next_distance);
	float V2 = visibility(-view.forward.xyz, wc_normal, previous_distance);

	return (V1 + V2) / 2.0;
}

float visibility( in vec3 wc_position, in vec3 wc_normal ) {
	float result = 0.0;
	const int views = 9;
	for (int i = 0; i < views; ++i)
		result += visibility_both_directions(i, wc_position, wc_normal);
	return result / float(views);
}




void main()
{
	vec2 tc_window = gl_FragCoord.xy / window_dimensions;
	float ec_position_z = get_ec_z(depths, tc_window, projection_matrix);
	vec3 wc_position = wc_view_eye_position + vertex.wc_view_ray_direction * -ec_position_z / z_far;
	vec3 wc_normal = texture(wc_normals, tc_window).xyz;
	vec3 wc_view_direction = normalize(vertex.wc_view_ray_direction);
	vec3 albedo = texture(albedos, tc_window).xyz;


	float roughness = 1.0;
	float uniform_distribution_random = texture(random, tc_window).x;

	vec3 bent_normal = normalize(texture(ambient_occlusion, tc_window).rgb * 2.0 - 1.0);
	float ambient_occlusion_factor = texture(ambient_occlusion, tc_window).a;
	const float a = 0.0;
	const float b = 1.0;
	const float c = 1.0;
	//ambient_occlusion_factor = pow(b * (ambient_occlusion_factor + a), c);

	// Direct light
	color = get_reflected_light(
		wc_position, 
		wc_normal, 
		wc_view_direction,
		albedo,
		bent_normal, 
		roughness);

	// Shadow Mapping
	color.rgb *= 
		 calculate_shadow_coefficient(
		 	shadow_map_0, 
		 	light,
		 	wc_position, 
		 	wc_normal, 
		 	tc_window, 
		 	uniform_distribution_random);

	// Indirect light
	color.rgb += 0.08 * albedo;
	//color.rgb += 0.08 * albedo * ambient_occlusion_factor;

	// Overrides
	//color.rgb = texture(wc_positions, tc_window).xyz;
	//color = oit();
	//color.rgb *= 0.1;
	//color += get_debug_view();
	//color = vec4(visibility(wc_position, wc_normal));
	color = get_photon_splats();
	//color += sampling_test(wc_position);
	//color.rgb = wc_position;
	//color.rgb = albedo;
	//color.rgb = vec3(ambient_occlusion_factor);	
	//color.rgb = vec3(float(counts[index]) / 20.0);

	// Overbright
	const float bloom_limit = 1.0;
	vec3 bright_color = max(color.rgb - vec3(bloom_limit), vec3(0.0));
	float brightness = dot(bright_color, vec3(1.0));
	brightness = smoothstep(0.0, 0.5, brightness);
	//overbright.rgb = mix(vec3(0.0), color.rgb, brightness);
}
