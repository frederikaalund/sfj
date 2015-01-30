#extension GL_NV_shader_buffer_load: enable
#extension GL_NV_gpu_shader5: enable
#extension GL_EXT_shader_image_load_store: enable

uniform sampler2D diffuse_texture;

uniform ivec2 window_dimensions;
uniform uint32_t total_data_offset;



struct ldm_data {
	uint32_t next;
	//uint32_t compressed_diffuse;
	float depth;
};

layout(binding = 0, offset = 0) uniform atomic_uint count;
coherent restrict layout(std430) buffer data_buffer
{ ldm_data data[]; };



struct vertex_data
{
	float negative_ec_position_z;
	vec2 oc_texture_coordinate;
};
in vertex_data vertex;



uint32_t allocate() { return total_data_offset + atomicCounterIncrement(count); }


uint32_t compress( in vec4 color ) 
{ return (uint32_t(color.x * 255.0) << 24u) + (uint32_t(color.y * 255.0) << 16u) + (uint32_t(color.z * 255.0) << 8u) + (uint32_t(0.1*255.0)); } 



void main()
{
	//vec2 tc_texture_coordinates = vec2(vertex.oc_texture_coordinate.x, 1.0 - vertex.oc_texture_coordinate.y);
	//vec4 diffuse = texture(diffuse_texture, tc_texture_coordinates);

	//uint32_t compressed_diffuse = compress(diffuse);
	//float depth = gl_FragCoord.z;
	float depth = vertex.negative_ec_position_z;

	// Calculate indices
	uint32_t head = total_data_offset + uint32_t(gl_FragCoord.x) + uint32_t(gl_FragCoord.y) * window_dimensions.x;
	uint32_t new = allocate();

	// Store fragment data in node
	//data[new].compressed_diffuse = compressed_diffuse;
	data[new].depth = depth;

	// Start with the head node
	uint32_t previous = head;
	uint32_t current = data[head].next;

	// Insert the new node while maintaining a sorted list.
	// The algorithm finishes in a finite yet indeterminate number of steps.
	// Indeterminate, since some steps may be repeated due to concurrent updates.
	// Thus the total number of steps required for a single insertion
	// is not known beforehand. However, finiteness guarantees
	// that the algorithm terminates eventually. In other words,
	// it is a lock-free algorithm (though not wait-free).
	
	for (;;)
	//const int max_iterations = 2048;
	//for (int i = 0; i < max_iterations; ++i)
		// We are either at the end of the list or just before a node of greater depth...
		if (current == 0 || depth < data[current].depth) {
			// ...so we attempt to insert the new node here. First,
			// the new node is set to point to the current node. It is crucial
			// that this change happens now since the next step makes
			// the new node visible to other threads. That is, the new node must
			// be in a complete state before becoming visible.
			data[new].next = current;

			// Memory barrier omitted for added performance.

			// Then the previous node is atomically updated to point to new node
			// if the previous node still points to the current node.
			// Returns the original content of data[previous].next (regardless of the
			// result of the comparison).
			uint32_t previous_next = atomicCompSwap(data[previous].next, current, new);

			// The atomic update occurred...
			if (previous_next == current)
				// ...so we are done.
				break;
			// Another thread updated data[previous].next before us...
			else
				// ...so we continue from previous_next
				current = previous_next;
		// We are still searching for a place to insert the new node...
		} else {
			// ...so we advance to the next node in the list.
			previous = current;
			current = data[current].next;
			//current = atomicAdd(data[current].next, 0); // Atomic read
		}
}