in vec4 oc_position;
in vec3 wc_camera_ray_direction;

struct vertex_data
{
	vec3 wc_camera_ray_direction;
};
noperspective out vertex_data vertex;



void main()
{
	gl_Position = oc_position;
	vertex.wc_camera_ray_direction = wc_camera_ray_direction;
}
