#ifndef BLACK_LABEL_WORLD_ENTITIES_HPP
#define BLACK_LABEL_WORLD_ENTITIES_HPP

#include <black_label/container/svector.hpp>
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

template<typename model_type, typename transformation_type>
class entities
{
public:
	typedef std::size_t size_type;
	typedef container::svector<model_type> model_container;
	typedef typename model_container::size_type model_id_type;



////////////////////////////////////////////////////////////////////////////////
/// Iterator
////////////////////////////////////////////////////////////////////////////////
	template<typename value>
	class iterator_base : public boost::iterator_facade<
		iterator_base<value>, 
		value, 
		boost::random_access_traversal_tag,
		value,
		size_type>
	{
	public:
    typedef size_type difference_type;
    
		iterator_base() {}
		iterator_base( value id ) : id_(id) {}
		template<typename other_value>
		iterator_base( 
			const iterator_base<other_value>& other,
			typename std::enable_if<std::is_convertible<other_value*, value*>::value>::type* = nullptr )
			: id_(other.id_) {}

	protected:
		size_type id_;

	private:
		friend boost::iterator_core_access;
		template<typename other_value> friend class iterator_base;

		value dereference() const
		{ return id_; }

		template<typename other_value>
		bool equal( const iterator_base<other_value>& other ) const
		{ return id_ == other.id_; }

		void increment() { ++id_; }
		void decrement() { --id_; }
		void advance( difference_type n ) { id_ += n; }

		template<typename other_value>
		difference_type distance_to( const iterator_base<other_value>& other ) const
		{ return other.id_ - id_; }
	};

	typedef iterator_base<size_type> iterator;
	typedef iterator_base<const size_type> const_iterator;

	typedef transformation_type* transformation_iterator;
	typedef const transformation_type* const_transformation_iterator;



////////////////////////////////////////////////////////////////////////////////
/// Entity
////////////////////////////////////////////////////////////////////////////////	
	template<typename iterator_type>
	class entity_base : public iterator_type
	{
	public:
		entity_base( entities& entities, iterator_type entity ) 
			: entities_(entities), iterator_type(entity) {}

		typename iterator_type::value_type id() const { return **this; }

		model_type& model() const
		{ return entities_.model_for_entity(id()); }
		model_id_type& model_id() const
		{ return entities_.model_ids[id()]; }
		transformation_type& transformation() const
		{ return entities_.transformations[id()]; }

	protected:
		entities& entities_;
	};

	template<typename iterator_type>
	class const_entity_base : public iterator_type
	{
	public:
		const_entity_base( const entities& entities, iterator_type entity ) 
			: iterator_type(entity), entities_(entities) {}

		typename iterator_type::value_type id() const { return **this; }

		const model_type& model() const
		{ return entities_.model_for_entity(id()); }
		const model_id_type& model_id() const
		{ return entities_.model_ids[id()]; }
		const transformation_type& transformation() const
		{ return entities_.transformations[id()]; }

	protected:
		const entities& entities_;
	};

	typedef entity_base<iterator> entity;
	typedef const_entity_base<const_iterator> const_entity;



////////////////////////////////////////////////////////////////////////////////
/// Group
////////////////////////////////////////////////////////////////////////////////
	class group
	{
	public:
		typedef container::svector<size_type> member_id_container;

		typedef member_id_container::iterator iterator;
		typedef member_id_container::const_iterator const_iterator;

		typedef entity_base<iterator> entity;
		typedef const_entity_base<const_iterator> const_entity;



////////////////////////////////////////////////////////////////////////////////
/// Group
////////////////////////////////////////////////////////////////////////////////
		group( entities& entities ) 
			: entities(entities)
			, member_ids(entities.capacity)
		{}
		~group()
		{
			std::for_each(member_ids.cbegin(), member_ids.cend(), 
				[this] ( size_type entity ) { this->entities.remove(entity); });
		}

		typename entities::iterator create(
			const model_type& model = model_type(), 
			const transformation_type& transformation = transformation_type() )
		{ 
			member_ids.push_back(entities.create(model, transformation));
			return entities::iterator(member_ids.back());
		}

		void remove( size_type id )
		{
			entities.remove(id);
			*std::find(member_ids.cbegin(), member_ids.cend(), id) = 
				member_ids.back();
			member_ids.pop_back();
		}
		void remove( typename entities::iterator entity ) { remove(entity.id()); }

		iterator begin() { return member_ids.begin(); };
		iterator end() { return member_ids.end(); };
		const_iterator cbegin() const { return member_ids.cbegin(); };
		const_iterator cend() const	{ return member_ids.cend(); };

		entity ebegin() { return entity(this->entities, begin()); };
		entity eend() { return entity(this->entities, end()); };
		const_entity cebegin() const 
		{ return group::const_entity(this->entities, cbegin()); };
		const_entity ceend() const
		{ return const_entity(this->entities, cend()); };

		entities& entities;
		member_id_container member_ids;
	};



////////////////////////////////////////////////////////////////////////////////
/// Entities
////////////////////////////////////////////////////////////////////////////////
	friend void swap( entities& lhs, entities& rhs )
	{
		using std::swap;
		swap(lhs.models, rhs.models);
		swap(lhs.capacity, rhs.capacity);
		swap(lhs.size, rhs.size);
		swap(lhs.id_size, rhs.id_size);
		swap(lhs.model_ids, rhs.model_ids);
		swap(lhs.transformations, rhs.transformations);
		swap(lhs.ids, rhs.ids);
	}

	entities( model_container& models, size_type capacity = 0 )
		: models(models)
		, capacity(capacity)
		, size(0)
		, id_size(0)
		, model_ids(new model_id_type[capacity])
		, transformations(new transformation_type[capacity])
		, ids(new size_type[capacity])
	{ size_type id = 0; std::generate(ids.get(), &ids[capacity], [&id](){ return id++; }); }
	entities( entities&& other ) { swap(*this, other); }

	entities& operator=( entities rhs ) { swap(*this, rhs); return *this; }

	size_type create( 
		const model_type& model = model_type(), 
		const transformation_type& transformation = transformation_type() )
	{
		entities::size_type id = ids[size++ % capacity];

		auto existing_model = std::find(models.cbegin(), models.cend(), model);
		if (models.cend() != existing_model)
			model_ids[id] = existing_model - models.cbegin();
		else
		{
			models.push_back(model);
			model_ids[id] = models.end() - 1 - models.cbegin();
		}
		
		transformations[id] = transformation;

		return id;
	}

	void remove( size_type id ) { ids[id_size++ % capacity] = id; }

	model_type& model_for_entity( size_type id )
	{ return models[model_ids[id]]; }
	const model_type& model_for_entity( size_type id ) const
	{ return models[model_ids[id]]; }

	std::vector<size_type> entities_for_model( 
		const model_id_type id ) const
	{
		std::vector<size_type> entity_ids;
		std::for_each(cbegin(), cend(), [&] ( size_type entity_id ) {
			if (id == model_ids[entity_id]) entity_ids.push_back(entity_id);
		});
		return std::move(entity_ids);
	}


	iterator begin() { return iterator(0); }
	iterator end() { return iterator(size); }
	const_iterator cbegin() const { return const_iterator(0); }
	const_iterator cend() const { return const_iterator(size); }

	entity ebegin() { return entity(*this, begin()); }
	entity eend() { return entity(*this, end()); }
	const_entity cebegin() const { return const_entity(*this, cbegin()); }
	const_entity ceend() const { return const_entity(*this, cend()); }

	transformation_iterator transformations_begin() { return transformations.get(); };
	transformation_iterator transformations_end() { return &transformations[size]; };
	const_transformation_iterator transformations_cbegin() const { return transformations.get(); };
	const_transformation_iterator transformations_cend() const { return &transformations[size]; };

	model_container& models;

	size_type capacity;
	boost::atomic<size_type> size, id_size;

	std::unique_ptr<model_id_type[]> model_ids;
	std::unique_ptr<transformation_type[]> transformations;
	std::unique_ptr<size_type[]> ids;



protected:
	entities( const entities& other );
};

} // namespace world
} // namespace black_label



#endif
