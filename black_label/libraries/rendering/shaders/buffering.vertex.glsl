uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;
uniform mat3 normal_matrix;



layout(location = 0) in vec4 oc_position;
layout(location = 1) in vec3 oc_normal;
layout(location = 2) in vec2 oc_texture_coordinate;

struct vertex_data
{
	vec3 wc_normal;
	vec3 wc_position;
	vec2 oc_texture_coordinate;
};
out vertex_data vertex;



void main()
{
	gl_Position = model_view_projection_matrix * oc_position;
	vertex.wc_normal = normalize(normal_matrix * oc_normal);
	vertex.wc_position = (model_matrix * oc_position).xyz;
	vertex.oc_texture_coordinate = oc_texture_coordinate;
}
