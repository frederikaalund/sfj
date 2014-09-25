uniform mat4 model_view_matrix;
uniform mat4 model_view_projection_matrix;
uniform mat3 normal_matrix;
uniform float z_far, z_near;



layout(location = 0) in vec4 oc_position;
layout(location = 1) in vec3 oc_normal;
layout(location = 2) in vec2 oc_texture_coordinate;

struct vertex_data
{
	vec3 wc_normal;
	vec2 oc_texture_coordinate;
	float normalized_ec_position_z;
};
out vertex_data vertex;



void main()
{
	gl_Position = model_view_projection_matrix * oc_position;
	vertex.wc_normal = normalize(normal_matrix * oc_normal);
	vertex.oc_texture_coordinate = oc_texture_coordinate;

	vec4 ec_position = model_view_matrix * oc_position;
	vertex.normalized_ec_position_z = -ec_position.z / (z_far - z_near);
}
