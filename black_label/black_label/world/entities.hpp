#ifndef BLACK_LABEL_WORLD_ENTITIES_HPP
#define BLACK_LABEL_WORLD_ENTITIES_HPP

#include <atomic>

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>

#include "tbb/concurrent_priority_queue.h"



namespace black_label {
namespace world {

template<typename model_type, typename dynamic_type, typename transformation_type>
class entities
{
public:
	using size_type = std::size_t;



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
	using difference_type = size_type;
    
		iterator_base() {}
		iterator_base( value id ) : id_{id} {}
		template<typename other_value>
		iterator_base( 
			const iterator_base<other_value>& other,
			typename std::enable_if<std::is_convertible<other_value*, value*>::value>::type* = nullptr )
			: id_{other.id_} {}

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

	using iterator = iterator_base<size_type>;
	using const_iterator = iterator_base<const size_type>;

	using transformation_iterator_ = transformation_type*;
	using const_transformation_iterator_ = const transformation_type*;



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
		dynamic_type& dynamics() const
		{ return entities_.dynamics[id()]; }
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
		const dynamic_type& dynamics() const
		{ return entities_.dynamics[id()]; }
		const transformation_type& transformation() const
		{ return entities_.transformations[id()]; }

	protected:
		const entities& entities_;
	};

	using entity = entity_base<iterator>;
	using const_entity = const_entity_base<const_iterator>;




////////////////////////////////////////////////////////////////////////////////
/// Range & Scoped Range
////////////////////////////////////////////////////////////////////////////////
	class range
	{
	public:
		range() {}
		range( size_type ids_begin, size_type ids_end )
			: ids_begin{ids_begin}
			, ids_end{ids_end}
		{ assert(ids_begin <= ids_end); }

		size_type size() const { return ids_end - ids_begin; }

		iterator begin() { return ids_begin; }
		iterator end() { return ids_end; }

		template<typename container>
		typename container::element_type* begin( container& container )
		{ return &container[ids_begin]; }
		template<typename container>
		typename container::element_type* end( container& container )
		{ return &container[ids_end]; }

		friend bool operator<( range lhs, range rhs )
		{ return lhs.size() < rhs.size(); }

		size_type ids_begin, ids_end;
	};

	class scoped_range {
	public:
		scoped_range( 
			entities& entities_, 
			range range ) : entities_(entities_), range(range) {}
		~scoped_range() { entities_.remove(range); }

		iterator begin() { return range.begin(); }
		iterator end() { return range.end(); }

		template<typename container>
		typename container::element_type* begin( container& container )
		{ return range.begin(container); }
		template<typename container>
		typename container::element_type* end( container& container )
		{ return range.end(container); }

		entities& entities_;
		range range;
	};



////////////////////////////////////////////////////////////////////////////////
/// Entities
////////////////////////////////////////////////////////////////////////////////
	entities( size_type capacity = 0 )
		: capacity{capacity}
		, size{0}
		, models{std::make_unique<std::shared_ptr<model_type>[]>(capacity)}
		, dynamics{std::make_unique<std::shared_ptr<dynamic_type>[]>(capacity)}
		, transformations{std::make_unique<transformation_type[]>(capacity)}
	{}
	entities( const entities& other ) = delete;
	entities( entities&& other ) 
		: capacity{std::move(other.capacity)}
		, size{other.size.load()}
		, models{std::move(other.models)}
		, dynamics{std::move(other.dynamics)}
		, transformations{std::move(other.transformations)}
	{}
	entities& operator=( entities rhs ) { std::swap(*this, rhs); return *this; }

	bool allocate_from_free_list( size_type size, range& result )
	{
		range free_range;

		// Failure; nothing in free list
		if (!free_list.try_pop(free_range)) return false;

		// Failure; not enough space
		if (free_range.size() < size) {
			// Return unused space
			free_list.push(free_range); 
			return false;
		}

		// Success; take space from the free range
		result = range{free_range.ids_begin, free_range.ids_begin + size};

		// Return unused space
		if (free_range.ids_end == result.ids_end)
			free_list.push(range(result.ids_end, free_range.ids_end));

		return true;
	}

	range allocate( size_type size )
	{
		// Try the free list first
		range result;
		if (allocate_from_free_list(size, result)) return result;

		// Just do a new allocation
		this->size += size;
		assert(this->size <= capacity);

		return range{this->size - size, this->size};

		// TODO: Add a third option where the free list is first
		// de-fragmented and then the allocation process is tried again.
	}

	template<typename model_iterator, typename dynamic_iterator, typename transformation_iterator>
	range create(
		model_iterator models_begin,
		model_iterator models_end,
		dynamic_iterator dynamics_begin,
		dynamic_iterator dynamics_end,
		transformation_iterator transformations_begin,
		transformation_iterator transformations_end)
	{
		auto count = models_end - models_begin;
		assert(dynamics_end - dynamics_begin == count);
		assert(transformations_end - transformations_begin == count);

		auto all_ = all();
		range new_range{allocate(count)};

		// Copy input ranges into the corresponding containers
		for (auto id : new_range) {
			assign_existing(all_.begin(models), all_.end(models), models[id], *models_begin++);
			assign_existing(all_.begin(dynamics), all_.end(dynamics), dynamics[id], *dynamics_begin++);
		}
		std::copy(transformations_begin, transformations_end, new_range.begin(transformations));

		return new_range;
	}
	template<typename model_container, typename dynamic_container, typename transformation_container>
	range create(
		const model_container& models,
		const dynamic_container& dynamics,
		const transformation_container& transformations)
	{
		return create(
			std::cbegin(models),
			std::cend(models),
			std::cbegin(dynamics),
			std::cend(dynamics),
			std::cbegin(transformations),
			std::cend(transformations));
	}
	range create(
		const std::initializer_list<model_type> models,
		const std::initializer_list<dynamic_type> dynamics,
		const std::initializer_list<transformation_type> transformations)
	{
		return create(
			std::cbegin(models),
			std::cend(models),
			std::cbegin(dynamics),
			std::cend(dynamics),
			std::cbegin(transformations),
			std::cend(transformations));
	}

	template<typename model_iterator, typename dynamic_iterator, typename transformation_iterator>
	scoped_range create_scoped(
		model_iterator models_begin,
		model_iterator models_end,
		dynamic_iterator dynamics_begin,
		dynamic_iterator dynamics_end,
		transformation_iterator transformations_begin,
		transformation_iterator transformations_end)
	{
		return scoped_range{*this, create(
			models_begin,
			models_end,
			dynamics_begin,
			dynamics_end,
			transformations_begin,
			transformations_end)};
	}
	template<typename model_container, typename dynamic_container, typename transformation_container>
	scoped_range create_scoped(
		const model_container& models,
		const dynamic_container& dynamics,
		const transformation_container& transformations)
	{ return scoped_range{*this, create(models, dynamics, transformations)}; }
	scoped_range create_scoped(
		const std::initializer_list<model_type> models,
		const std::initializer_list<dynamic_type> dynamics,
		const std::initializer_list<transformation_type> transformations)
	{ return scoped_range{*this, create(models, dynamics, transformations)}; }

	void remove( range range )
	{ 
		std::fill(range.begin(models), range.end(models), std::shared_ptr<model_type>());
		free_list.push(range);
	}

//////////////////////////////////////////////////////////////////////////
/// Search range for the given value. If a previous entry exists in the
/// range then 'which' is assigned to the previous value. Otherwise,
/// the value is simply assigned to 'which'.
//////////////////////////////////////////////////////////////////////////
	template<typename value_type>
	void assign_existing( 
		std::shared_ptr<value_type>* begin,
		std::shared_ptr<value_type>* end,
		std::shared_ptr<value_type> which, 
		value_type value )
	{
		// Find existing value
		auto existing_model = std::find_if(begin, end, 
			[value] (const auto& model) { return *model == value; });

		// Value already exists; re-use it
		if (end != existing_model)
			which = *existing_model;
		// Otherwise, insert a new value
		else
			which = std::make_shared<value_type>(value);
	}

	model_type& model_for_entity( size_type id )
	{ return models[id]; }
	const model_type& model_for_entity( size_type id ) const
	{ return models[id]; }

	range all() const { return range(0, size); }

	iterator begin() { return iterator(0); }
	iterator end() { return iterator(size); }
	const_iterator cbegin() const { return const_iterator(0); }
	const_iterator cend() const { return const_iterator(size); }

	entity ebegin() { return entity(*this, begin()); }
	entity eend() { return entity(*this, end()); }
	const_entity cebegin() const { return const_entity(*this, cbegin()); }
	const_entity ceend() const { return const_entity(*this, cend()); }

	transformation_iterator_ transformations_begin() { return transformations.get(); };
	transformation_iterator_ transformations_end() { return &transformations[size]; };
	const_transformation_iterator_ transformations_cbegin() const { return transformations.get(); };
	const_transformation_iterator_ transformations_cend() const { return &transformations[size]; };



	size_type capacity;
	std::atomic<size_type> size;
	tbb::concurrent_priority_queue<range> free_list;

	std::unique_ptr<std::shared_ptr<model_type>[]> models;
	std::unique_ptr<std::shared_ptr<dynamic_type>[]> dynamics;
	std::unique_ptr<transformation_type[]> transformations;
};

} // namespace world
} // namespace black_label



#endif
