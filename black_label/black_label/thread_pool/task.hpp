#ifndef BLACK_LABEL_THREAD_POOL_TASK
#define BLACK_LABEL_THREAD_POOL_TASK

#include "black_label/thread_pool/types.hpp"



namespace black_label
{
namespace thread_pool
{

struct task
{
	typedef void data_type;
	typedef void (function_type)( data_type* data );
	typedef int priority_type;
	typedef int weight_type;

	task() { }
	task( 
		function_type* function, 
		data_type* data, 
		thread_id_type thread_affinity = NOT_A_THREAD_ID,
		priority_type priority = 0,
		weight_type weight = 1)
		: function(function)
		, data(data)
		, thread_affinity(thread_affinity)
		, weight(weight)
	{ }

	void operator()() { function(data); }

	function_type* function;
	data_type* data;
	thread_id_type thread_affinity;
	priority_type priority;
	weight_type weight;
};

} // namespace thread_pool
} // namespace black_label



#endif
