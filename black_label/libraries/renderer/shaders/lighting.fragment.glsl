uniform sampler2D depths;
uniform sampler2D wc_normals;
uniform sampler2D albedos;
uniform sampler2D random_texture;
uniform sampler2D ambient_occlusion_texture;
uniform sampler2DShadow shadow_map;

uniform samplerBuffer lights;
#ifdef USE_TILED_SHADING
uniform samplerBuffer light_grid;
uniform samplerBuffer light_index_list;
uniform int tile_size;
#else
uniform int lights_size;
#endif

uniform ivec2 window_dimensions;
uniform ivec2 grid_dimensions;

uniform vec3 wc_camera_eye_position;
uniform float z_far;

uniform mat4 projection_matrix;
uniform mat4 light_matrix;



struct vertex_data
{
	vec3 wc_camera_ray_direction;
};
noperspective in vertex_data vertex;

out vec4 color;
out vec4 overbright;



float eye_z( in vec2 tc )
{
	float buffer_z = texture(depths, tc).x;
	return projection_matrix[3][2] / (-2.0 * buffer_z + 1.0 - projection_matrix[2][2]);
}



vec4 calculate_direct_light( 
	in vec3 wc_position, 
	in vec3 wc_normal, 
	in vec3 albedo,
	in float specular_exponent )
{
	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);

#ifdef USE_TILED_SHADING
	ivec2 grid_index = ivec2(gl_FragCoord.xy) / tile_size;
	ivec2 grid_data = ivec2(texelFetch(light_grid, grid_dimensions.x * grid_index.y + grid_index.x).xy);

	int offset = grid_data.x;
	int count = grid_data.y;
#else
	int count = lights_size;
#endif

	for (int l = 0; l < count; ++l)
	{
#ifdef USE_TILED_SHADING
		int light_id = int(texelFetch(light_index_list, offset + l).x);
#else
		int light_id = l;
#endif
#define LIGHT_STRUCT_SIZE 9
		
		vec3 position = vec3(texelFetch(lights, light_id * LIGHT_STRUCT_SIZE).x, texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 1).x, texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 2).x);
		vec3 light_color = vec3(texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 3).x, texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 4).x, texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 5).x);
		float constant_attenuation = texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 6).x;
		float linear_attenuation = texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 7).x;
		float cubic_attenuation = texelFetch(lights, light_id * LIGHT_STRUCT_SIZE + 8).x;

		vec3 light_direction = position - wc_position;
		float light_distance = length(light_direction);
		light_direction /= light_distance;
		
		float falloff = 1.0 / (constant_attenuation + light_distance * (linear_attenuation + cubic_attenuation * light_distance));
		
		// Diffuse
		vec3 light_contribution = albedo * light_color * max(dot(wc_normal, light_direction), 0.0);
		// Specular
		light_contribution += specular_exponent * max(pow(dot(wc_normal, -normalize(vertex.wc_camera_ray_direction)), 2.0), 0.0);
		
		// TODO: Remove the following line. I'm just testing HDR!
		light_contribution *= 4.0;
		
		// Falloff
		result.rgb += light_contribution * falloff;
	}

	return result;
}



float shadow_map_lookup_with_offset( in sampler2DShadow shadow_map, in vec2 inverted_shadow_map_size, in vec4 v, in vec2 offset )
{
	return textureProj(shadow_map, vec4(v.st + offset * inverted_shadow_map_size * v.w, v.zw));
}

float calculate_shadow_coefficient( in vec3 wc_position )
{
	const float bias = 1.0;
	vec4 cc_position_from_lights_view = light_matrix * vec4(wc_position, 1.0);
	cc_position_from_lights_view.z -= bias;
	/*
	float shadow_coefficient = textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(-2, 2));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(-1, 2));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(0, 2));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(1, 2));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(2, 2));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(-2, 1));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(-1, 1));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(0, 1));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(1, 1));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(2, 1));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(-2, 0));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(-1, 0));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(0, 0));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(1, 0));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(2, 0));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(-2, -1));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(-1, -1));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(0, -1));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(1, -1));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(2, -1));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(-2, -2));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(-1, -2));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(0, -2));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(1, -2));
	shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(2, -2));
	*/

	// The following works just as well on NVidia hardware.
	// Note that the pragma works globally! I.e., all loops will be unrolled.
	//
	//#pragma optionNV(unroll all)
	//for (int t = 2; t >= -2; --t)
	//	for (int s = -2; s <= 2; ++s)
	//		shadow_coefficient += textureProjOffset(shadow_map, cc_position_from_lights_view, ivec2(s, t));

	vec2 inverted_shadow_map_size = 1.0 / vec2(textureSize(shadow_map, 0));
	vec2 inverted_random_texture_size = 1.0 / vec2(textureSize(random_texture, 0));
	vec2 tc_random_texture = gl_FragCoord.xy * inverted_random_texture_size;
	float shadow_coefficient = 0.0;

	

	vec2 random_direction = texture(random_texture, tc_random_texture).xy;

	mat2 orient = mat2(random_direction, vec2(-random_direction.y, random_direction.x));

	for (float t = 3.5; t >= -3.5; t -= 1.0)
	{
		for (float s = -3.5; s <= 3.5; s += 1.0)
		{
			
			shadow_coefficient += shadow_map_lookup_with_offset(shadow_map, inverted_shadow_map_size, cc_position_from_lights_view, vec2(s, t)) + orient[0][0] * 1.0e-32;

			//tc_random_texture.x += s * inverted_random_texture_size.x;
		}
		//tc_random_texture.x += t * inverted_random_texture_size.y;
	}

	shadow_coefficient *= 1.0 / (64.0);
	return shadow_coefficient;
}



void main()
{
	vec2 tc_window = gl_FragCoord.xy / window_dimensions;
	vec3 wc_position = wc_camera_eye_position + vertex.wc_camera_ray_direction * -eye_z(tc_window) / z_far;
	vec3 wc_normal = texture(wc_normals, tc_window).xyz;
	vec3 albedo = texture(albedos, tc_window).xyz;
	float specular_exponent = texture(albedos, tc_window).w;
	float ambient_occlusion = texture(ambient_occlusion_texture, tc_window).r;

	// Direct light
	color = 0.5880 * calculate_direct_light(wc_position, wc_normal, albedo, specular_exponent);

	// Shadow Mapping
	color.rgb *= calculate_shadow_coefficient(wc_position);

	// Indirect light
	color.rgb += 0.5880 * albedo * (1.0 - ambient_occlusion);

	// Overrides
	//color.rgb *= 1.0e-32;
	//color.rgb += vec3(1.0 - ambient_occlusion);

	// Overbright
    const float bloom_limit = 1.0;
    vec3 bright_color = max(color.rgb - vec3(bloom_limit), vec3(0.0));
    float brightness = dot(bright_color, vec3(1.0));
    brightness = smoothstep(0.0, 0.5, brightness);
    overbright.rgb = mix(vec3(0.0), color.rgb, brightness);
}
