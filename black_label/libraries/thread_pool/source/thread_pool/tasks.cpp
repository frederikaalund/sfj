#include "black_label/thread_pool/tasks.hpp"

#include <cstring>



namespace black_label
{
namespace thread_pool
{



tasks::tasks( size_type capacity )
	: capacity(capacity)
	, size(0)
	, max_parents(100)
	, max_children(100)
	, functions(new function_type*[capacity])
	, data(new data_type*[capacity])
	, thread_affinities(new thread_id_type[capacity])
	, weights(new weight_type[capacity])
	, parent_counts(new size_type[capacity])
	, parents(new size_type[capacity * max_parents])
	, child_counts(new size_type[capacity])
	, children(new size_type[capacity * max_children])
	, ids(new size_type[capacity])
{
	for (size_type id = 0; id < capacity; ++id) ids[id] = id;
}

tasks::~tasks()
{
	delete[] functions;
	delete[] data;
	delete[] thread_affinities;
	delete[] weights;
	delete[] parent_counts;
	delete[] parents;
	delete[] child_counts;
	delete[] children;
	delete[] ids;
}



tasks::size_type tasks::create(
	function_type* function, 
	data_type* data, 
	thread_id_type thread_affinity, 
	weight_type weight, 
	size_type parent_count, 
	size_type* parents )
{
	size_type task = ids[size++];

	functions[task] = function;
	this->data[task] = data;
	thread_affinities[task] = thread_affinity;
	weights[task] = weight;

	parent_counts[task] = parent_count;
	memcpy(
		this->parents + max_parents * task, 
		parents, 
		sizeof(size_type) * parent_count);

	child_counts[task] = 0;
	for (size_type i = 0; i < parent_counts[task]; ++i)
		add_child(parent(task, i), task);

	return task;
}


inline void tasks::add_child( 
	size_type parent, 
	size_type child )
{
	size_type child_index = child_counts[parent]++;
	children[max_children * parent + child_index] = child;
}

void tasks::remove( size_type task )
{
	ids[--size] = task;
}

void tasks::run( size_type task )
{
	functions[task](data[task]);
}



////////////////////////////////////////////////////////////////////////////////
/// Dynamic Interface for Runtime-loaded Shared Libraries
////////////////////////////////////////////////////////////////////////////////
BLACK_LABEL_SHARED_LIBRARY tasks* tasks_construct( tasks::size_type capacity )
{
	return new tasks(capacity);
}

BLACK_LABEL_SHARED_LIBRARY void tasks_destroy( tasks* tasks )
{
	delete tasks;
}

BLACK_LABEL_SHARED_LIBRARY tasks::size_type tasks_create(
	tasks* tasks,
	tasks::function_type* function,
	tasks::data_type* data,
	thread_id_type thread_affinity,
	tasks::weight_type weight,
	tasks::size_type parent_count,
	tasks::size_type* parents )
{
	return tasks->create(
		function, 
		data, 
		thread_affinity, 
		weight, 
		parent_count, 
		parents);
}

BLACK_LABEL_SHARED_LIBRARY void tasks_remove(
	tasks* tasks,
	tasks::size_type task )
{
	tasks->remove(task);
}

BLACK_LABEL_SHARED_LIBRARY void tasks_run( 
	tasks* tasks, 
	tasks::size_type task )
{
	tasks->run(task);
}



} // namespace thread_pool
} // namespace black_label
