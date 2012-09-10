#ifndef BLACK_LABEL_CONTAINER_SVECTOR_HPP
#define BLACK_LABEL_CONTAINER_SVECTOR_HPP

#include <black_label/container/darray.hpp>

#include <type_traits>



namespace black_label
{
namespace container
{

////////////////////////////////////////////////////////////////////////////////
/// svector
/// 
/// Vector of static size. Functions like a std::vector without the automatic 
/// resizing and bounds checking.
////////////////////////////////////////////////////////////////////////////////

template<typename T>
class svector : public darray<T>
{
public:
	friend void swap( svector& rhs, svector& lhs )
	{
		using std::swap;
		swap(rhs.capacity_, lhs.capacity_);
		swap(rhs.data_, lhs.data_);
		swap(rhs.size_, lhs.size_);
	}

	svector() : darray(), size_(0) {}
	svector( svector&& other ) : darray(), size_(0) { swap(*this, other); }
	svector( const svector& other ) 
		: darray(other)
		, size_(other.size_)
	{ std::copy(other.cbegin(), other.cend(), data_); }
	svector( size_type capacity ) 
		: darray(capacity)
		, size_(0) {}

	svector& operator=( svector other ) { swap(*this, other); return *this; }

	size_type size() const { return size_; }
	bool empty() const { return 0 == size_; }

	iterator end() { return iterator(&data_[size_]); }
	const_iterator cend() const { return const_iterator(&data_[size_]); }

	reference back() { return data_[size_ - 1]; }
	const_reference back() const { return data_[size_ - 1]; }

	void push_back( const T& value ) { data_[size_++] = value; }
	void push_back( T&& value ) { data_[size_++] = std::move(value); }

	void pop_back() { pop_back<T>(*this); }
	
	void clear() { clear_<T>(*this); }

	template<typename input_iterator>
	iterator insert( const_iterator position, input_iterator first, input_iterator last )
	{ 
		assert(cend() == position); // TODO: Implement insert according to spec.

		if (first == last) return end(); // TODO should return the element position points to, but this is also .end() in the current non-spec implementation.

		for (; last != first; ++first) 
			push_back(*first);

		return end()-(last-first)-1;
	}



protected:
	size_type size_;



private:
	template<typename T_> static
	typename std::enable_if<!std::has_trivial_destructor<T_>::value>::type
	pop_back_( svector<T_>& sv )
	{ sv.back().~T_(); --sv.size_; }

	template<typename T_> static
	typename std::enable_if<std::has_trivial_destructor<T_>::value>::type
	pop_back_( svector<T_>& sv ) { --sv.size_; } 


	template<typename T_> static
	typename std::enable_if<!std::has_trivial_destructor<T_>::value>::type
	clear_( svector<T_>& sv )
	{
		for (auto i = sv.begin(); sv.end() != i; ++i)
			i->~T_();
		sv.size_ = svector<T_>::size_type(0);
	}

	template<typename T_> static
	typename std::enable_if<std::has_trivial_destructor<T_>::value>::type
	clear_( svector<T_>& sv ) { sv.size_ = svector<T_>::size_type(0); } 
};

} // namespace container
} // namespace black_label



#endif
