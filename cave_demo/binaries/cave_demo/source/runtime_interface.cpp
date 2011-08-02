#include "black_label/world.hpp"
#include "black_label/thread_pool.hpp"

using namespace black_label::world;
using namespace black_label::thread_pool;

world::construct_world_type* world::construct;
world::destroy_world_type* world::destroy;
world::copy_world_type* world::copy;
entities::add_entity_type* entities::add;
entities::remove_entity_type* entities::remove;

thread_pool::construct_thread_pool_type* thread_pool::construct;
thread_pool::destroy_thread_pool_type* thread_pool::destroy;
thread_pool::add_task_type* thread_pool::add_task;
thread_pool::add_raw_task_type* thread_pool::add_raw_task;
thread_pool::join_type* thread_pool::join;
