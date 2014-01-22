uniform mat4 model_view_projection_matrix;
uniform mat3 normal_matrix;



in vec4 oc_position;
in vec3 oc_normal;
in vec2 oc_texture_coordinate;

struct vertex_data
{
	vec3 wc_normal;
	vec2 oc_texture_coordinate;
};
out vertex_data vertex;



void main()
{
	gl_Position = model_view_projection_matrix * oc_position;
	vertex.wc_normal = normalize(normal_matrix * oc_normal);
	vertex.oc_texture_coordinate = oc_texture_coordinate;
}
