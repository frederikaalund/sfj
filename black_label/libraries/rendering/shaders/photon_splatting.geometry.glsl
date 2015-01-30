uniform mat4 view_projection_matrix;



layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in photon_data {
	vec3 wc_position, Du_x, Dv_x;
	vec4 irradiance;
	mat3 M;
} photon[];

out splat_data {
	flat vec3 wc_position;
	flat mat3 M;
	flat vec4 irradiance;
} splat;


vec4 cc_position( in vec3 wc_offset )
{ return view_projection_matrix * vec4(photon[0].wc_position + wc_offset, 1.0); }

void main() {
	splat.wc_position = photon[0].wc_position;
	splat.irradiance = photon[0].irradiance;
	splat.M = photon[0].M;

	gl_Position = cc_position(0.5 * (-photon[0].Du_x - photon[0].Dv_x));
	EmitVertex();

	gl_Position = cc_position(0.5 * (-photon[0].Du_x + photon[0].Dv_x));
	EmitVertex();

	gl_Position = cc_position(0.5 * ( photon[0].Du_x - photon[0].Dv_x));
	EmitVertex();

	gl_Position = cc_position(0.5 * ( photon[0].Du_x + photon[0].Dv_x));
	EmitVertex();
}