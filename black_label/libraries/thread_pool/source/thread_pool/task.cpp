#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/thread_pool/task.hpp>



namespace black_label
{
namespace thread_pool
{

task* task::DUMMY_TASK = reinterpret_cast<task*>(1);



void swap( task& lhs, task& rhs )
{
	using std::swap;
	swap(lhs.function, rhs.function);
	swap(lhs.thread_affinity, rhs.thread_affinity);
	swap(lhs.weight, rhs.weight);
	swap(lhs.predecessor, rhs.predecessor);
	swap(lhs.sub_tasks, rhs.sub_tasks);
	swap(lhs.successor, rhs.successor);
	swap(lhs.sub_tasks_left, rhs.sub_tasks_left);
}

task::~task()
{
	if (!is_root()) return;

	std::for_each(sub_tasks.begin(), sub_tasks.end(), 
		[] ( task& sub_task ) {	sub_task.predecessor = nullptr;	});

	while (successor)
	{
		task* temporary = successor->successor;

		successor->predecessor = this;
		std::for_each(successor->sub_tasks.begin(), successor->sub_tasks.end(), 
			[] ( task& sub_task ) {	sub_task.predecessor = nullptr;	});
		delete successor;

		successor = temporary;
	}
}




void task::restore_task_hierarchy()
{
	std::for_each(sub_tasks.begin(), sub_tasks.end(), 
		[this]( task& sub_task )
	{
		sub_task.predecessor = this;
		sub_task.restore_task_hierarchy();
	});

	task* t = this;
	while (t->successor)
	{
		t->successor->predecessor = t;
		std::for_each(t->successor->sub_tasks.begin(), t->successor->sub_tasks.end(), 
			[&]( task& sub_task )
		{
			sub_task.predecessor = t->successor;
			sub_task.restore_task_hierarchy();
		});
		t->successor->sub_tasks_left.store(t->successor->sub_tasks.size());
		t = t->successor;
	}

	sub_tasks_left.store(sub_tasks.size());
}

void task::reset_task_counts()
{
	std::for_each(sub_tasks.begin(), sub_tasks.end(), 
		[this]( task& sub_task )
	{ sub_task.restore_task_hierarchy(); });

	if (successor)
		successor->restore_task_hierarchy();

	sub_tasks_left.store(sub_tasks.size());
}

task& task::last_successor()
{
	// Find sub_task's successor chain's end
	task* result = this;
	while (result->successor)
		result = result->successor;
	return *result;
}



task& task::in_parallel( task&& rhs )
{ 
	if (rhs.is_group_task() && !rhs.successor)
		this->sub_tasks.insert(this->sub_tasks.end(), rhs.sub_tasks.cbegin(), 
		rhs.sub_tasks.cend());
	else
		this->sub_tasks.push_back(std::move(rhs));

	return *this;
}
task& task::in_parallel( const task& rhs )
{ 
	if (rhs.is_group_task() && !rhs.successor)
		this->sub_tasks.insert(this->sub_tasks.end(), rhs.sub_tasks.cbegin(), 
		rhs.sub_tasks.cend());
	else
		this->sub_tasks.push_back(rhs);

	return *this;
}

task& task::operator|=( task&& rhs )
{ return in_parallel(std::move(rhs)); }
task& task::operator|=( const task& rhs )
{ return in_parallel(rhs); }

  
task& task::in_succession( task&& rhs )
{
	task& end = this->last_successor();

	// Make predecessor a sub task if necessary
	if (!end.is_group_task())
		end.sub_tasks.push_back(end);

	end.successor = new task(std::move(rhs));
	return *this;
}
task& task::in_succession( const task& rhs )
{
	task& end = this->last_successor();

	// Make predecessor a sub task if necessary
	if (!end.is_group_task())
		end.sub_tasks.push_back(end);

	end.successor = new task(rhs);
	return *this;
}

task& task::operator>>=( task&& rhs )
{ return in_succession(std::move(rhs)); }
task& task::operator>>=( const task& rhs )
{ return in_succession(rhs); }

} // namespace thread_pool
} // namespace black_label



using namespace black_label::thread_pool;



task in_parallel( task&& lhs, task&& rhs )
{
	task result;
  
	if (lhs.is_group_task() && !lhs.successor)
		result.sub_tasks.insert(result.sub_tasks.end(), lhs.sub_tasks.cbegin(),
                            lhs.sub_tasks.cend());
	else
		result.sub_tasks.push_back(std::move(lhs));
  
	if (rhs.is_group_task() && !rhs.successor)
		result.sub_tasks.insert(result.sub_tasks.end(), rhs.sub_tasks.cbegin(),
                            rhs.sub_tasks.cend());
	else
		result.sub_tasks.push_back(std::move(rhs));
  
	return result;
}
task in_parallel( const task& lhs, const task& rhs )
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

task operator|( task&& lhs, task&& rhs )
{ return in_parallel(std::move(lhs), std::move(rhs)); }
task operator|( const task& lhs, const task& rhs )
{ return in_parallel(lhs, rhs); }



task in_succession( task&& lhs, task&& rhs )
{
	task result = std::move(lhs);
	task& end = result.last_successor();
  
	// Make predecessor a sub task if necessary
	if (!end.is_group_task())
		end.sub_tasks.push_back(end);
  
	end.successor = new task(std::move(rhs));
	return result;
}
task in_succession( const task& lhs, const task& rhs )
{
	task result = lhs;
	task& end = result.last_successor();
  
	// Make predecessor a sub task if necessary
	if (!end.is_group_task())
		end.sub_tasks.push_back(end);
  
	end.successor = new task(rhs);
	return result;
}

task operator>>( task&& lhs, task&& rhs )
{ return in_succession(std::move(lhs), std::move(rhs)); }
task operator>>( const task& lhs, const task& rhs )
{ return in_succession(lhs, rhs); }
