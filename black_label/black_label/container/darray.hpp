#ifndef BLACK_LABEL_CONTAINER_DARRAY_HPP
#define BLACK_LABEL_CONTAINER_DARRAY_HPP

#include <algorithm>
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
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;



////////////////////////////////////////////////////////////////////////////////
/// Iterator
////////////////////////////////////////////////////////////////////////////////
	template<typename value>
	class iterator_base : public boost::iterator_facade<
		iterator_base<value>, 
		value, 
		boost::random_access_traversal_tag,
		value&,
		difference_type>
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
		template<typename other_value> friend class iterator_base;

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
	
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;



////////////////////////////////////////////////////////////////////////////////
/// darray
////////////////////////////////////////////////////////////////////////////////
	friend void swap( darray& rhs, darray& lhs )
	{
		using std::swap;
		swap(rhs.capacity_, lhs.capacity_);
		swap(rhs.data_, lhs.data_);
	}

	darray() : capacity_(0), data_(nullptr) {}
	darray( darray&& other ) : capacity_(0), data_(nullptr) { swap(*this, other); }
	darray( const darray& other ) 
		: capacity_(other.capacity_)
	{ 
		if (!other.data_) { data_ = nullptr; return; }; 
		data_ = new T[other.capacity_];
		std::copy(other.cbegin(), other.cend(), data_); 
	}
	darray( size_type capacity ) 
		: capacity_(capacity)
		, data_(new T[capacity]) {}
	template<typename input_iterator>
	darray( input_iterator first, input_iterator last )
		: capacity_(last - first)
		, data_((0 < capacity_) ? new T[capacity_] : nullptr)
	{ std::copy(first, last, data_); }
	~darray() { delete[] data_; }

	darray& operator=( darray other ) { swap(*this, other); return *this; }

	reference operator[]( size_type i ) { return data_[i]; }
	const_reference operator[]( size_type i ) const { return data_[i]; }

	size_type capacity() const { return capacity_; }
	pointer data() { return data_; }
	const_pointer data() const { return data_; }

	bool empty() const { return size_type(0) == capacity_; }

	iterator begin() { return iterator(data_); }
	iterator end() { return iterator(&data_[capacity_]); }
	const_iterator cbegin() const { return const_iterator(data_); }
	const_iterator cend() const { return const_iterator(&data_[capacity_]); }

	reference front() { return data_[0]; }
	reference back() { return data_[capacity_ - 1]; }
	const_reference front() const { return data_[0]; }
	const_reference back() const { return data_[capacity_ - 1]; }

protected:
	size_type capacity_;
	T* data_;
};

} // namespace container
} // namespace black_label



#endif
