#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/dynamics.hpp>

#include <black_label/utility/scoped_stream_suppression.hpp>

#include <algorithm>

#include <glm/gtc/type_ptr.hpp>



using namespace std;



namespace black_label {
namespace dynamics {

dynamics::dynamics( world_type& world )
	: world(world)
	, dynamics_(new dynamics_container::element_type[world.static_entities.capacity])
	, sorted_statics(world.static_entities.capacity)
	, sorted_dynamics(world.dynamic_entities.capacity)
	, collision_configuration(new btDefaultCollisionConfiguration)
	, dispatcher(new btCollisionDispatcher(collision_configuration.get()))
	, broadphase(new btDbvtBroadphase)
	, solver(new btSequentialImpulseConstraintSolver)
	, dynamics_world(new btDiscreteDynamicsWorld(dispatcher.get(), broadphase.get(), solver.get(), collision_configuration.get()))
	, world_importer(new btBulletWorldImporter(dynamics_world.get()))
{
	dynamics_world->setGravity(btVector3(0.0f, -10.0f, 0.0f));
}



void dynamics::import_static_dynamics( entity_id_type id )
{
	auto t = world_importer->getNumRigidBodies();
	world_importer->loadFile(world.static_entities.dynamics[id].c_str());
	auto rigid_body = dynamics_[id] = world_importer->getRigidBodyByIndex(t);

	btTransform entity_transform;
	entity_transform.setFromOpenGLMatrix(glm::value_ptr(world.static_entities.transformations[id]));
	rigid_body->setWorldTransform(entity_transform * rigid_body->getWorldTransform());
}

void dynamics::import_dynamic_dynamics( entity_id_type id )
{
	auto t = world_importer->getNumRigidBodies();
	world_importer->loadFile(world.dynamic_entities.dynamics[id].c_str());
	auto rigid_body = dynamics_[id] = world_importer->getRigidBodyByIndex(t);

	glm::mat4 bl_entity_transform = world.dynamic_entities.transformations[id];
	auto scale = btVector3(bl_entity_transform[0][0], bl_entity_transform[1][1], bl_entity_transform[2][2]);
	bl_entity_transform[0][0] = bl_entity_transform[1][1] = bl_entity_transform[1][1] = 1.0f;

	btTransform entity_transform;
	entity_transform.setFromOpenGLMatrix(glm::value_ptr(bl_entity_transform));
	rigid_body->setWorldTransform(entity_transform * rigid_body->getWorldTransform());

	rigid_body->getCollisionShape()->setLocalScaling(scale);
}



void dynamics::update()
{
	{
		utility::scoped_stream_suppression suppress(stdout);

		for (entity_id_type id; dirty_static_dynamics.dequeue(id);)
		{
			import_static_dynamics(id);
			sorted_statics.push_back(id);
		}

		for (entity_id_type id; dirty_dynamic_dynamics.dequeue(id);)
		{
			import_dynamic_dynamics(id);
			sorted_dynamics.push_back(id);
		}
	}

	sort(sorted_statics.begin(), sorted_statics.end());
	sort(sorted_dynamics.begin(), sorted_dynamics.end());

	dynamics_world->stepSimulation(1.0f / 60.0f, 10);



	std::for_each(sorted_dynamics.cbegin(), sorted_dynamics.cend(), 
		[&] ( const entity_id_type id )
	{
		auto d = dynamics_[id];
		auto& transform = this->world.dynamic_entities.transformations[id];

		glm::mat4 test1, test2(0.0f);
		d->getWorldTransform().getOpenGLMatrix(glm::value_ptr(test1));
		auto scale = d->getCollisionShape()->getLocalScaling();
		test2[0][0] = scale[0];
		test2[1][1] = scale[1];
		test2[2][2] = scale[2];
		test2[3][3] = 1.0f;

		transform = test1 * test2;
	});
}

} // namespace dynamics
} // namespace black_label
