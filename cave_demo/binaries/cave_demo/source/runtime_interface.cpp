#include "black_label/world/dynamic_interface.hpp"
#include "black_label/thread_pool/dynamic_interface.hpp"

using namespace black_label::world;
using namespace black_label::thread_pool;

world::method_pointers_type			world::method_pointers;
entities::method_pointers_type		entities::method_pointers;
thread_pool::method_pointers_type	thread_pool::method_pointers;
tasks::method_pointers_type			tasks::method_pointers;
