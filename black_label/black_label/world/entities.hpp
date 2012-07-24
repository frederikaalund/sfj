#ifndef BLACK_LABEL_WORLD_ENTITIES_HPP
#define BLACK_LABEL_WORLD_ENTITIES_HPP

#include <black_label/utility/boost_atomic_extensions.hpp>

#include <algorithm>
#include <cstring>
#include <memory>
#include <type_traits>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>



namespace black_label
{
namespace world
{

template<typename model_type, typename transformation_type, typename id_type>
class entities
{
public:
	typedef id_type size_type;



////////////////////////////////////////////////////////////////////////////////
/// Iterator
////////////////////////////////////////////////////////////////////////////////
	template<typename value>
	class iterator_base : public boost::iterator_facade<
		iterator_base<value>, 
		value, 
		boost::random_access_traversal_tag>
	{
	public:
		iterator_base() {}
		iterator_base( value* node ) 
			: node(node) {}
		template<typename other_value>
		iterator_base( 
			const iterator_base<other_value>& other,
			typename std::enable_if<std::is_convertible<other_value*, value*>::value>::type* = nullptr )
			: node(other.node) {}

	private:
		friend boost::iterator_core_access;

		value& dereference() const
		{ return *node; }

		template<typename other_value>
		bool equal( const iterator_base<other_value>& other ) const
		{ return this->node == other.node; }

		void increment() { ++node; }
		void decrement() { --node; }
		void advance( difference_type n ) { node += n; }

		template<typename other_value>
		difference_type distance_to( const iterator_base<other_value>& other ) const
		{ return other.node - node; }

		value* node;
	};
	typedef iterator_base<model_type> iterator;
	typedef iterator_base<const model_type> const_iterator;



////////////////////////////////////////////////////////////////////////////////
/// Entity
////////////////////////////////////////////////////////////////////////////////
	class entity
	{
	public:
		entity( const entities& entities, size_type id ) 
			: entities(entities), id(id) {}

		model_type& model() 
		{ return entities.models[id]; }
		transformation_type& transformation() 
		{ return entities.transformations[id]; }

		operator model_type&()
		{ return model(); }
		operator transformation_type&()
		{ return transformation(); }
		operator size_type()
		{ return id; }

		const entities& entities;
		const size_type id;
	};



////////////////////////////////////////////////////////////////////////////////
/// Group
////////////////////////////////////////////////////////////////////////////////
	class group
	{
	public:



////////////////////////////////////////////////////////////////////////////////
/// Iterator
////////////////////////////////////////////////////////////////////////////////
		template<typename value, typename group_type, typename model_type_, typename transformation_type_>
		class iterator_base : public boost::iterator_facade<
			iterator_base<value, group_type, model_type_, transformation_type>, 
			value,
			boost::random_access_traversal_tag>
		{
		public:
			iterator_base() {}
			iterator_base( group_type* group, size_type* node ) 
				: group(group), node(node) {}
			template<typename other_value, typename other_group_type, typename other_model_type_, typename other_transformation_type_>
			iterator_base( 
				const iterator_base<other_value, other_group_type, other_model_type_, other_transformation_type_>& other,
				typename std::enable_if<std::is_convertible<other_value*, value*>::value>::type* = nullptr )
				: group(other.group), node(other.node) {}

			model_type_& model() const
			{ return group->entities.models[group->member_ids[*node]]; }
			transformation_type_& transformation() const
			{ return group->entities.transformations[group->member_ids[*node]]; }
			size_type& id() const
			{ return group->member_ids[*node]; }

			operator model_type_&() const
			{ return model(); }
			operator transformation_type_&() const
			{ return transformation(); }
			operator size_type() const
			{ return id(); }



		private:
			friend boost::iterator_core_access;

			value& dereference() const
			{ return entity(group->entities, group->member_ids[*node]); }

			template<typename other_value, typename other_group_type, typename other_model_type_, typename other_transformation_type_>
			bool equal( const iterator_base<other_value, other_group_type, other_model_type_, other_transformation_type_>& other ) const
			{ return this->node == other.node; }

			void increment() { ++node; }
			void decrement() { --node; }
			void advance( difference_type n ) { node += n; }

			template<typename other_value, typename other_group_type, typename other_model_type_, typename other_transformation_type_>
			difference_type distance_to( const iterator_base<other_value, other_group_type, other_model_type_, other_transformation_type_>& other ) const
			{ return other.node - node; }

			group_type* group;
			size_type* node;
		};
		typedef iterator_base<entity, group, model_type, transformation_type>
			iterator;
		typedef iterator_base<const entity, const group, const model_type, const transformation_type>
			const_iterator;



		group( entities& entities ) 
			: entities(entities)
			, capacity(entities.capacity)
			, size(0)
			, member_ids(new size_type[capacity])
		{}
		~group()
		{
			std::for_each(&member_ids[0], &member_ids[size], 
				[this] ( size_type entity ) { this->entities.remove(entity); });
		}

		entity create(
			const model_type& model = model_type(), 
			const transformation_type& transformation = transformation_type() )
		{ 
			return entity(
				entities, 
				member_ids[size++] = entities.create(model, transformation)); 
		}

		void remove( size_type id )
		{
			entities.remove(id);
			*std::find(&member_ids[0], &member_ids[size], id) = 
				member_ids[--size];
		}
		void remove( entity entity ) { remove(entity.id); }

		iterator begin() { return iterator(this, member_ids.get()); };
		iterator end() { return iterator(this, &member_ids[size]); };
		const_iterator cbegin() const
		{ return const_iterator(this, member_ids.get()); };
		const_iterator cend() const
		{ return const_iterator(this, &member_ids[size]); };

		entities& entities;
		size_type capacity, size;
		std::unique_ptr<size_type[]> member_ids;
	};



////////////////////////////////////////////////////////////////////////////////
/// Entities
////////////////////////////////////////////////////////////////////////////////
	entities() {}

	friend void swap( entities& lhs, entities& rhs )
	{
		using std::swap;
		swap(lhs.capacity, rhs.capacity);
		swap(lhs.size, rhs.size);
		swap(lhs.id_size, rhs.id_size);
		swap(lhs.models, rhs.models);
		swap(lhs.transformations, rhs.transformations);
		swap(lhs.ids, rhs.ids);
	}

	entities( size_type capacity )
		: capacity(capacity)
		, size(0)
		, id_size(0)
		, models(new model_type[capacity])
		, transformations(new transformation_type[capacity])
		, ids(new size_type[capacity])
	{ for (size_type id = 0; id < capacity; ++id) ids[id] = id; }
	entities( entities&& other ) { swap(*this, other); }

	entities& operator=( entities rhs ) { swap(*this, rhs); return *this; }

	size_type create( 
		const model_type& model = model_type(), 
		const transformation_type& transformation = transformation_type() )
	{
		entities::size_type id = ids[size++ % capacity];

		models[id] = model;
		transformations[id] = transformation;

		return id;
	}

	void remove( size_type id ) { ids[id_size++ % capacity] = id; }

	iterator models_begin() { return iterator(models.get()); };
	iterator models_end() { return iterator(&models[size]); };

	const_iterator models_cbegin() const
	{ return const_iterator(models.get()); };
	const_iterator models_cend() const 
	{ return const_iterator(&models[size]); };

	size_type capacity;
	boost::atomic<size_type> size, id_size;

	std::unique_ptr<model_type[]> models;
	std::unique_ptr<transformation_type[]> transformations;
	std::unique_ptr<size_type[]> ids;

protected:
	entities( const entities& other );
};

} // namespace world
} // namespace black_label



#endif
