#extension GL_NV_shader_buffer_load: enable
#extension GL_NV_gpu_shader5: enable
#extension GL_EXT_shader_image_load_store: enable

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform float specular_exponent;
uniform ivec2 window_dimensions;

layout (std430) buffer headers
{
	uint32_t counts[];
};

struct vertex_data
{
	vec3 wc_normal;
	vec2 oc_texture_coordinate;
};
in vertex_data vertex;
layout(pixel_center_integer) in uvec2 gl_FragCoord;

layout(location = 0) out vec3 wc_normal;
layout(location = 1) out vec4 albedo;



void main()
{
	vec2 tc_texture_coordinates = vec2(vertex.oc_texture_coordinate.x, 1.0 - vertex.oc_texture_coordinate.y);

	vec4 diffuse = texture(diffuse_texture, tc_texture_coordinates);

	if (diffuse.a <= 1.0e-5) discard;

	// TODO: Do this correctly.
	float specular_exponent_ = specular_exponent * texture(specular_texture, tc_texture_coordinates).r;

	wc_normal = normalize(vertex.wc_normal);
	albedo.rgb = diffuse.rgb;
	albedo.a = specular_exponent_;



	// A-buffer
	uint32_t index = gl_FragCoord.x + gl_FragCoord.y * window_dimensions.x;
	//counts[index] = 0;
	atomicAdd(counts[index], 1);
}
