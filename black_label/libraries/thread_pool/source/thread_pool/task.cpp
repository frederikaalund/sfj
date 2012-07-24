#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/thread_pool/task.hpp>



namespace black_label
{
namespace thread_pool
{

task operator|( task&& lhs, task&& rhs )
{ 
	task result;

	if (lhs.is_group_task() && !lhs.successor)
		result.sub_tasks.insert(result.sub_tasks.end(), lhs.sub_tasks.cbegin(), 
			lhs.sub_tasks.cend());
	else
		result.sub_tasks.push_back(lhs);
		
	if (rhs.is_group_task() && !rhs.successor)
		result.sub_tasks.insert(result.sub_tasks.end(), rhs.sub_tasks.cbegin(), 
			rhs.sub_tasks.cend());
	else
		result.sub_tasks.push_back(rhs);

	return result;
}

task operator>>( task&& lhs, task&& rhs )
{
	task result = std::move(lhs);
	task& end = result.last_successor();

	// Make predecessor a sub task if necessary
	if (!end.is_group_task())
		end.sub_tasks.push_back(end);

	end.successor.reset(new task(std::move(rhs)));
	return result;
}



task operator|( const task& lhs, const task& rhs )
{ 
	task result;

	if (lhs.is_group_task() && !lhs.successor)
		result.sub_tasks.insert(result.sub_tasks.end(), lhs.sub_tasks.cbegin(), 
		lhs.sub_tasks.cend());
	else
		result.sub_tasks.push_back(lhs);

	if (rhs.is_group_task() && !rhs.successor)
		result.sub_tasks.insert(result.sub_tasks.end(), rhs.sub_tasks.cbegin(), 
		rhs.sub_tasks.cend());
	else
		result.sub_tasks.push_back(rhs);

	return result;
}

task operator>>( const task& lhs, const task& rhs )
{
	task result = lhs;
	task& end = result.last_successor();

	// Make predecessor a sub task if necessary
	if (!end.is_group_task())
		end.sub_tasks.push_back(end);

	end.successor.reset(new task(rhs));
	return result;
}



task& operator|=( task& lhs, task&& rhs )
{ 
	if (rhs.is_group_task() && !rhs.successor)
		lhs.sub_tasks.insert(lhs.sub_tasks.end(), rhs.sub_tasks.cbegin(), 
		rhs.sub_tasks.cend());
	else
		lhs.sub_tasks.push_back(rhs);

	return lhs;
}

task& operator>>=( task& lhs, task&& rhs )
{
	task& end = lhs.last_successor();

	// Make predecessor a sub task if necessary
	if (!end.is_group_task())
		end.sub_tasks.push_back(end);

	end.successor.reset(new task(std::move(rhs)));
	return lhs;
}

} // namespace thread_pool
} // namespace black_label
