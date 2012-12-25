uniform mat4 model_view_projection_matrix;



in vec4 oc_position;



void main()
{
	gl_Position = model_view_projection_matrix * oc_position;
}
