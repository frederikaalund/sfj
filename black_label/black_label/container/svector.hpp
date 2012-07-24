#ifndef BLACK_LABEL_CONTAINER_SVECTOR_HPP
#define BLACK_LABEL_CONTAINER_SVECTOR_HPP

#include <black_label/container/darray.hpp>



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

	iterator end() { return iterator(&data_[size_]); }
	const_iterator cend() const { return const_iterator(&data_[size_]); }

	void push_back( const T& value ) { data_[size_++] = value; }
	void push_back( T&& value ) { data_[size_++] = std::move(value); }



protected:
	size_type size_;
};

} // namespace container
} // namespace black_label



#endif
