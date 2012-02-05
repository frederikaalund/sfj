#ifndef BLACK_LABEL_THREAD_POOL_TASKS_HPP
#define BLACK_LABEL_THREAD_POOL_TASKS_HPP

#include "black_label/shared_library/utility.hpp"

#include "black_label/thread_pool/types.hpp"



namespace black_label
{
namespace thread_pool
{

struct tasks
{
public:
	typedef size_t size_type;

	typedef void data_type;
	typedef void (function_type)( data_type* data );
	typedef int weight_type;



////////////////////////////////////////////////////////////////////////////////
/// Static Interface
////////////////////////////////////////////////////////////////////////////////
#ifndef BLACK_LABEL_THREAD_POOL_DYNAMIC_INTERFACE

	tasks( size_type capacity );
	~tasks();

	size_type create(
		function_type* function,
		data_type* data = 0,
		thread_id_type thread_affinity = NOT_A_THREAD_ID,
		weight_type weight = 0,
		size_type parent_count = 0,
		size_type* parents = 0 );
	void remove( size_type task );
	void run( size_type task );

	inline void tasks::add_child(size_type parent, size_type child);

	size_type parent(size_type task, size_type index) const
	{ 
		return parents[max_parents * task + index]; 
	}
	bool has_parents(size_type task) const
	{ 
		return 0 < parent_counts[task]; 
	}

	size_type capacity, size, max_parents, max_children;

	function_type** functions;
	data_type** data;
	thread_id_type* thread_affinities;
	weight_type* weights;
	size_type* parent_counts;
	size_type* parents;
	size_type* child_counts;
	size_type* children;
	
	size_type* ids;



////////////////////////////////////////////////////////////////////////////////
/// Dynamic Interface for Runtime-loaded Shared Libraries
////////////////////////////////////////////////////////////////////////////////
#else

	struct method_pointers_type
	{
		void* (*construct)(
			size_type capacity );
		void (*destroy)(
			void* tasks );
		size_type (*create)(
			void* tasks,
			function_type* function,
			data_type* data,
			thread_id_type thread_affinity,
			weight_type weight,
			size_type parent_count,
			size_type* parents );
		void (*remove)(
			void* tasks,
			size_type task );
		void (*run)(
			void* tasks,
			size_type task );
	} static method_pointers;

	tasks( void* tasks ) 
		: _this(tasks)
		, owns_this(false)
	{}
	tasks( size_type capacity ) 
		: _this(method_pointers.construct(capacity))
		, owns_this(true)
	{}
	~tasks()
	{ if (owns_this) method_pointers.destroy(_this); }
	size_type create( 
		function_type* function,
		data_type* data = 0,
		thread_id_type thread_affinity = NOT_A_THREAD_ID,
		weight_type weight = 0,
		size_type parent_count = 0,
		size_type* parents = 0 )
	{
		return method_pointers.create(
			_this, 
			function, 
			data, 
			thread_affinity, 
			weight, 
			parent_count, 
			parents);
	}
	void remove( size_type task )
	{ method_pointers.remove(_this, task); }
	void run( size_type task )
	{ method_pointers.run(_this, task); }

	void* _this;
	bool owns_this;

#endif
};

} // namespace thread_pool
} // namespace black_label



#endif
