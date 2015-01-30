uniform mat4 view_matrix;
uniform mat4 view_projection_matrix;
uniform uint photon_count;



layout(location = 0) in vec3 wc_position;
layout(location = 1) in vec3 wc_normal;
layout(location = 2) in vec3 Du_x;
layout(location = 3) in vec3 Dv_x;
layout(location = 4) in vec4 radiant_flux;

out photon_data {
	vec3 wc_position, Du_x, Dv_x;
	vec4 irradiance;
	mat3 M;
} photon;

mat3 mat3_from_rows( in vec3 row1, in vec3 row2, in vec3 row3 ) {
	return mat3(
		row1.x, row2.x, row3.x,  // Column 1
		row1.y, row2.y, row3.y,  // Column 2
		row1.z, row2.z, row3.z); // Column 3
}

void main() {
	gl_Position = view_projection_matrix * vec4(wc_position, 1.0);
	photon.wc_position = wc_position;

	const float scale = 2000.0;
	photon.Du_x = Du_x * scale;
	photon.Dv_x = Dv_x * scale;

	const float a = scale;
	photon.M = mat3_from_rows(
		cross(photon.Dv_x, wc_normal),
		cross(wc_normal, photon.Du_x),
		a * wc_normal);
	photon.M *= 2.0 / dot(photon.Du_x, cross(photon.Dv_x, wc_normal));

	float area = PI / 4.0 * length(cross(photon.Du_x, photon.Dv_x)); // [m^2]
	photon.irradiance = radiant_flux / area; // [W * m^-2]
}
