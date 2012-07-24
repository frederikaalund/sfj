#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <black_label/thread_pool.hpp>
#include <glm/glm.hpp>



using namespace black_label::thread_pool;



struct fixture 
{
	fixture() : a(0), b(0), c(0) {}

	black_label::thread_pool::thread_pool pool;
	task t;
	boost::atomic<int> a, b, c;
};



////////////////////////////////////////////////////////////////////////////////
/// Empty Tasks
////////////////////////////////////////////////////////////////////////////////
BOOST_FIXTURE_TEST_SUITE(empty_task, fixture)

BOOST_AUTO_TEST_CASE(empty_task_1)
{
	pool.schedule(task());
}

BOOST_AUTO_TEST_CASE(empty_task_2)
{
	pool.schedule(task() >> [](){});
}

BOOST_AUTO_TEST_CASE(empty_task_3)
{
	pool.schedule([](){} >> task());
}

BOOST_AUTO_TEST_SUITE_END()



////////////////////////////////////////////////////////////////////////////////
/// Complicated Hierarchy
////////////////////////////////////////////////////////////////////////////////
BOOST_FIXTURE_TEST_SUITE(complicated_hierarchy, fixture)

BOOST_AUTO_TEST_CASE(complicated_hierarchy_1)
{
	pool.schedule(
		([this](){ a--; } | [this](){ a++; } | [this](){ a--; })		// -1
			>> [this](){ a.store(a*2); }								// -2
			>> ([this](){ a++; } | [this](){ a++; } | [this](){ a++; })	// +1
			>> ([this](){ a.store(a*5); } >> [this](){ a--; })			// +4
		);
	pool.join();

	BOOST_CHECK_EQUAL(a, 4);
}

BOOST_AUTO_TEST_CASE(complicated_hierarchy_2)
{
	pool.schedule(
		(
			([this](){ a--; } | [this](){ a--; })
			// a(-2), b(0), c(0)
			>> ([this](){ a.store(a*4); } | [this](){ b.store(b+5); })
			// a(-8), b(5), c(0)
			>> [this](){ a.store(a+b); }	
			// a(-3), b(5), c(0)
			| [this](){ c.store(c-2); }
			// a(-3), b(5), c(-2)
		)
		>> [this](){ b.store((b-c)*a); }
		// a(-3), b(-21), c(-2)
		>> ([this](){ c.store(b*c+2); } | [this](){ a.store(b*b); })
		// a(441), b(-21), c(44)
		);
	pool.join();
	
	BOOST_CHECK_EQUAL(a, 441);
	BOOST_CHECK_EQUAL(b, -21);
	BOOST_CHECK_EQUAL(c, 44);
}

BOOST_AUTO_TEST_SUITE_END()
	


////////////////////////////////////////////////////////////////////////////////
/// Stress
////////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_SUITE(stress, fixture)

#define STRESS_1_ITERATIONS 10000
BOOST_AUTO_TEST_CASE(stress_1)
{
	for (int i = 0; STRESS_1_ITERATIONS > i; ++i)
		t >>= ([this](){ a++; } | [this](){ a++; }) >> [this](){ a.store(a/2); };

	pool.schedule(t);
	pool.join();

	BOOST_CHECK_EQUAL(a, 1);
}

#define STRESS_2_ITERATIONS 10000
BOOST_AUTO_TEST_CASE(stress_2)
{
	t.sub_tasks.reserve(STRESS_2_ITERATIONS);
	for (int i = 0; STRESS_2_ITERATIONS > i; ++i)
		t |= [this](){ a++; };

	pool.schedule(t);
	pool.join();

	BOOST_CHECK_EQUAL(a, STRESS_2_ITERATIONS);
}

#define STRESS_3_ITERATIONS 10000
BOOST_AUTO_TEST_CASE(stress_3)
{
	for (int i = 0; STRESS_3_ITERATIONS > i; ++i)
	{
		task t;
		for (int i = 0; 10 > i; ++i)
			t |= [this](){ a++; };

		pool.schedule(t);
		pool.join();

		BOOST_CHECK_EQUAL(a, 10);
		a.store(0);
	}
}

#define STRESS_4_THREAD_POOLS 1000
#define STRESS_4_TASKS 10
BOOST_AUTO_TEST_CASE(stress_4)
{
	for (int i = 0; STRESS_4_THREAD_POOLS > i; ++i)
	{
		{
			black_label::thread_pool::thread_pool pool;

			task t;
			for (int i = 0; STRESS_4_TASKS > i; ++i)
				t |= [this](){ a++; };

			pool.schedule(t);
		}

		BOOST_CHECK_EQUAL(a, STRESS_4_TASKS);
		a.store(0);
	}
}

BOOST_AUTO_TEST_SUITE_END()
