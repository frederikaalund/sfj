layout(location = 0) in vec4 oc_position;
layout(location = 1) in vec3 wc_view_ray_direction;

struct vertex_data
{
	vec3 wc_view_ray_direction;
};
noperspective out vertex_data vertex;



void main()
{
	gl_Position = oc_position;
	vertex.wc_view_ray_direction = wc_view_ray_direction;
}
