#ifndef BLACK_LABEL_CONTAINER_DARRAY_HPP
#define BLACK_LABEL_CONTAINER_DARRAY_HPP

#include <cstddef>
#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>



namespace black_label
{
namespace container
{

////////////////////////////////////////////////////////////////////////////////
/// darray
/// 
/// Fixed-sized array based on dynamic storage.
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class darray
{
public:
	typedef T value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef T& reference;
	typedef const T& const_reference;



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
	typedef iterator_base<T> iterator;
	typedef iterator_base<const T> const_iterator;



////////////////////////////////////////////////////////////////////////////////
/// darray
////////////////////////////////////////////////////////////////////////////////
	friend void swap( darray& rhs, darray& lhs )
	{
		using std::swap;
		swap(rhs.capacity_, lhs.capacity_);
		swap(rhs.data_, lhs.data_);
	}

	darray() : data_(nullptr) {}
	darray( darray&& other ) : data_(nullptr) { swap(*this, other); }
	darray( const darray& other ) 
		: capacity_(other.capacity_)
		, data_(new T[other.capacity_]) 
	{ std::copy(other.cbegin(), other.cend(), data_); }
	darray( size_type capacity ) 
		: capacity_(capacity)
		, data_(new T[capacity]) {}
	~darray() { delete[] data_; }

	darray& operator=( darray other ) { swap(*this, other); return *this; }

	reference operator[]( size_type i ) { return data_[i]; }
	const_reference operator[]( size_type i ) const { return data_[i]; }

	size_type capacity() const { return capacity_; }
	T* data() { return data_; }
	const T* data() const { return data_; }

	iterator begin() { return iterator(data_); }
	iterator end() { return iterator(&data_[capacity_]); }
	const_iterator cbegin() const { return const_iterator(data_); }
	const_iterator cend() const { return const_iterator(&data_[capacity_]); }



protected:
	size_type capacity_;
	T* data_;
};

} // namespace container
} // namespace black_label



#endif
