// Version 0.1
#ifndef BLACK_LABEL_DYNAMICS_DYNAMICS_HPP
#define BLACK_LABEL_DYNAMICS_DYNAMICS_HPP

#include <black_label/shared_library/utility.hpp>
#include <black_label/world.hpp>

#include <memory>

#include <boost/lockfree/fifo.hpp>

#include <btBulletDynamicsCommon.h>
#include <btBulletWorldImporter.h>



namespace black_label
{
namespace dynamics
{

class BLACK_LABEL_SHARED_LIBRARY dynamics
{
public:
	typedef world::world world_type;
	typedef world_type::entity_id_type entity_id_type;

	dynamics( world_type& world );

	void update();

	void report_dirty_static_dynamics( entity_id_type id )
	{ dirty_static_dynamics.enqueue(id); }
	void report_dirty_static_dynamics( 
		world_type::entities_type::group::const_iterator first, 
		world_type::entities_type::group::const_iterator last )
	{ while (first != last) report_dirty_static_dynamics(*first++); }

	void report_dirty_dynamic_dynamics( entity_id_type id )
	{ dirty_dynamic_dynamics.enqueue(id); }
	void report_dirty_dynamic_dynamics( 
		world_type::entities_type::group::const_iterator first, 
		world_type::entities_type::group::const_iterator last )
	{ while (first != last) report_dirty_dynamic_dynamics(*first++); }



protected:
	dynamics( const dynamics& other );

private:
MSVC_PUSH_WARNINGS(4251)

	typedef boost::lockfree::fifo<entity_id_type> dirty_dynamics_container;
	typedef std::unique_ptr<btCollisionObject*[]> dynamics_container;
	typedef container::svector<entity_id_type> sorted_dynamics_container;

	void import_static_dynamics( entity_id_type id );
	void import_dynamic_dynamics( entity_id_type id );



	world_type& world;

	dirty_dynamics_container dirty_static_dynamics, dirty_dynamic_dynamics;
	dynamics_container dynamics_;

	sorted_dynamics_container sorted_statics, sorted_dynamics;

	std::unique_ptr<btCollisionConfiguration> collision_configuration;
	std::unique_ptr<btCollisionDispatcher> dispatcher;
	std::unique_ptr<btBroadphaseInterface> broadphase;
	std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
	std::unique_ptr<btDynamicsWorld> dynamics_world;
	std::unique_ptr<btBulletWorldImporter> world_importer;

MSVC_POP_WARNINGS()
};

} // namespace dynamics
} // namespace black_label



#endif
