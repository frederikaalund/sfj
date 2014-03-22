uniform mat4 model_view_matrix;
uniform mat4 model_view_projection_matrix;
uniform float z_near, z_far;



in vec4 oc_position;



void main()
{
	gl_Position = model_view_projection_matrix * oc_position;
}
