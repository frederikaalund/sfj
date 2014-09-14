#ifndef BLACK_LABEL_WORLD_ENTITIES_HPP
#define BLACK_LABEL_WORLD_ENTITIES_HPP

#include <black_label/path.hpp>

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>



namespace black_label {
namespace world {

template<typename model_type_, typename dynamic_type_, typename transformation_type_>
class entities_base
{
public:
////////////////////////////////////////////////////////////////////////////////
/// Types
////////////////////////////////////////////////////////////////////////////////
	using model_type = model_type_;
	using dynamic_type = dynamic_type_;
	using transformation_type = transformation_type_;
	using size_type = std::size_t;



////////////////////////////////////////////////////////////////////////////////
/// Entity
////////////////////////////////////////////////////////////////////////////////
	template<typename entities_type>
	class entity
	{
	public:
		entity() {}
		entity( entities_type* entities_, size_type index )
			: entities_{entities_}, index{index} {}
		template<typename other_entities_type>
		entity(
			const entity<other_entities_type>& other,
			typename std::enable_if<std::is_convertible<other_entities_type*, entities_type*>::value>::type* = nullptr )
			: entities_{ other.entities_ }, index{ other.index } {}

		// Custom members
		typename std::conditional<std::is_const<entities_type>::value, const model_type, model_type>::type&
		model() const { return entities_->models[index]; }

		typename std::conditional<std::is_const<entities_type>::value, const dynamic_type, dynamic_type>::type&
		dynamic() const { return entities_->dynamics[index]; }

		typename std::conditional<std::is_const<entities_type>::value, const transformation_type, transformation_type>::type&
		transformation() const { return entities_->transformations[index]; }

		entities_type* entities_;
		size_type index;
	};



////////////////////////////////////////////////////////////////////////////////
/// Iterator
////////////////////////////////////////////////////////////////////////////////
	template<typename entities_type, typename value>
	class iterator_base : public boost::iterator_facade<
		iterator_base<entities_type, value>, 
		entity<entities_type>, 
		boost::random_access_traversal_tag,
		entity<entities_type>&,
		size_type>
	{
	public:
	using difference_type = size_type;
    
		iterator_base() {}
		iterator_base( 
			entities_type* entities_, 
			value index ) : entities_{entities_}, index{index} {}
		template<typename other_value>
		iterator_base( 
			const iterator_base<entities_type, other_value>& other,
			typename std::enable_if<std::is_convertible<other_value*, value*>::value>::type* = nullptr )
			: entities_{other.entities_}, index{other.index} {}

	protected:
		entities_type* entities_;
		size_type index;

	private:
		friend boost::iterator_core_access;
		template<typename entities_type, typename other_value> friend class iterator_base;

		// Iterator members
		entity<entities_type> dereference() const { return entity<entities_type>{entities_, index}; }

		template<typename other_value>
		bool equal( const iterator_base<entities_type, other_value>& other ) const
		{ return index == other.index; }

		void increment() { ++index; }
		void decrement() { --index; }
		void advance( difference_type n ) { index += n; }

		template<typename other_value>
		difference_type distance_to( const iterator_base<entities_type, other_value>& other ) const
		{ return other.index - index; }
	};

	using iterator = iterator_base<entities_base, size_type>;
	using const_iterator = iterator_base<const entities_base, const size_type>;



////////////////////////////////////////////////////////////////////////////////
/// Range & Scoped Range
////////////////////////////////////////////////////////////////////////////////
	class range
	{
	public:
		range() {}
		range( 
			std::weak_ptr<entities_base> entities_,
			size_type begin_, 
			size_type end_ )
			: entities_(entities_)
			, begin_{begin_}
			, end_{end_}
		{ assert(begin_ <= end_); }

		size_type size() const { return end_ - begin_; }

		std::weak_ptr<entities_base> entities_;
		size_type begin_, end_;
	};



////////////////////////////////////////////////////////////////////////////////
/// Entities
////////////////////////////////////////////////////////////////////////////////
	friend void swap( entities_base& lhs, entities_base& rhs )
	{
		using std::swap;
		swap(lhs.size, rhs.size);
		swap(lhs.models, rhs.models);
		swap(lhs.dynamics, rhs.dynamics);
		swap(lhs.transformations, rhs.transformations);
	}
	
	entities_base() {}
	entities_base( const entities_base& other ) = delete;
	entities_base( entities_base&& other ) = default;
	entities_base& operator=( entities_base rhs ) { swap(*this, rhs); return *this; }

	template<
		typename model_iterator, 
		typename dynamic_iterator, 
		typename transformation_iterator>
	entities_base(
		const model_iterator models_begin,
		const model_iterator models_end,
		const dynamic_iterator dynamics_begin,
		const dynamic_iterator dynamics_end,
		const transformation_iterator transformations_begin,
		const transformation_iterator transformations_end)
		: size{models_end - models_begin}
		, models{std::make_unique<model_type[]>(size)}
		, dynamics{std::make_unique<dynamic_type[]>(size)}
		, transformations{std::make_unique<transformation_type[]>(size)}
	{
		assert(dynamics_end - dynamics_begin == size);
		assert(transformations_end - transformations_begin == size);

		// Copy input ranges into the corresponding containers
		std::copy(models_begin, models_end, models.get());
		std::copy(dynamics_begin, dynamics_end, dynamics.get());
		std::copy(transformations_begin, transformations_end, transformations.get());
	}

	template<
		typename model_container, 
		typename dynamic_container, 
		typename transformation_container>
	entities_base(
		const model_container& models,
		const dynamic_container& dynamics,
		const transformation_container& transformations)
		: entities_base(
		std::cbegin(models), 
		std::cend(models),
		std::cbegin(dynamics),
		std::cend(dynamics),
		std::cbegin(transformations),
		std::cend(transformations))
	{}

	entities_base(
		const std::initializer_list<model_type> models,
		const std::initializer_list<dynamic_type> dynamics,
		const std::initializer_list<transformation_type> transformations)
		: entities_base(
		std::cbegin(models),
		std::cend(models),
		std::cbegin(dynamics),
		std::cend(dynamics),
		std::cbegin(transformations),
		std::cend(transformations))
	{}

	iterator begin() { return iterator{this, 0}; }
	iterator end() { return iterator{this, size}; }
	const_iterator begin() const { return const_iterator{ this, 0 }; }
	const_iterator end() const { return const_iterator{ this, size }; }
	const_iterator cbegin() const { return const_iterator{ this, 0 }; }
	const_iterator cend() const { return const_iterator{ this, size }; }

	void sort_by_model() // TODO: Performance optimizations (avoid allocations)
	{
		// Generate and sort indices
		auto indices = std::make_unique<size_type[]>(size);
		{
			size_type start_index = 0;
			std::generate(begin(indices), end(indices), [&] { return start_index++; });
		}
		std::sort(begin(indices), end(indices), [&] ( size_type rhs, size_type lhs ) 
			{ return models[rhs] < models[lhs]; });

		// Swap element according to the sorted indices
		std::vector<model_type> models_copy{begin(models), end(models)};
		std::vector<dynamic_type> dynamics_copy{begin(dynamics), end(dynamics)};
		std::vector<transformation_type> transformations_copy{begin(transformations), end(transformations)};
		for (int i = 0; size > i; ++i)
		{
			models[i] = models_copy[indices[i]];
			dynamics[i] = dynamics_copy[indices[i]];
			transformations[i] = transformations_copy[indices[i]];
		}
	}
	
	

	size_type size;

	std::unique_ptr<model_type[]> models;
	std::unique_ptr<dynamic_type[]> dynamics;
	std::unique_ptr<transformation_type[]> transformations;



private:
	template<typename T>
	T* begin( std::unique_ptr<T[]>& container ) { return container.get(); }
	template<typename T>
	T* end( std::unique_ptr<T[]>& container ) { return &container[size]; }
};



////////////////////////////////////////////////////////////////////////////////
/// Canonical Types
////////////////////////////////////////////////////////////////////////////////
using entities = entities_base<boost::filesystem::path, boost::filesystem::path, glm::mat4>;
using group = std::vector<std::shared_ptr<entities>>;
using const_group = std::vector<std::shared_ptr<const entities>>;

inline const_group const_group_cast( const group& original )
{
	const_group result(original.size());
	std::transform(std::cbegin(original), std::cend(original), std::begin(result),  
		[] ( const std::shared_ptr<entities>& entities_ ) { return std::const_pointer_cast<const entities>(entities_); });
	return std::move(result);
}

} // namespace world
} // namespace black_label



#endif
