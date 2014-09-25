uniform mat4 model_view_matrix;
uniform mat4 model_view_projection_matrix;
uniform float z_far, z_near;



layout(location = 0) in vec4 oc_position;

struct vertex_data
{ float negative_ec_position_z; };
out vertex_data vertex;



void main()
{
	gl_Position = model_view_projection_matrix * oc_position;

	vec4 ec_position = model_view_matrix * oc_position;
	vertex.negative_ec_position_z = -ec_position.z;
}
