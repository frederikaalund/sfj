#ifndef BLACK_LABEL_UTILITY_RANGE_HPP
#define BLACK_LABEL_UTILITY_RANGE_HPP

#include <iterator>



namespace black_label {
namespace utility {

////////////////////////////////////////////////////////////////////////////////
/// Const Pointer Range
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class const_pointer_range
{
public:
	using const_iterator = const T*;

	template<typename container>
	static const_iterator pointer_cbegin( container& container_ )
	{ return &*std::cbegin(container_); }
	template<typename container>
	static const_iterator pointer_cend( container& container_ )
	{ return std::next(&*std::prev(std::cend(container_))); }

	const_pointer_range() : begin_{nullptr}, end_{nullptr} {}
	template<typename container>
	const_pointer_range( container& container_ )
		: begin_{pointer_cbegin(container_)}
		, end_{pointer_cend(container_)}
	{}
	const_pointer_range( std::initializer_list<T> list )
		: begin_{pointer_cbegin(list)}, end_{pointer_cend(list)} {}
	template<typename iterator>
	const_pointer_range( iterator begin, iterator end )
		: begin_{begin}, end_{end} {}
	template<typename container>
	const_pointer_range( typename container::const_iterator begin )
		: begin_{begin}, end_{nullptr} {}

	const_iterator begin() const { return begin_; }
	const_iterator end() const { return end_; }
	const_iterator cbegin() const { return begin_; }
	const_iterator cend() const { return end_; }

	bool is_empty() const { return begin_ == end_; }
	bool is_open_ended() const { return nullptr == end_; }

private:
	const_iterator begin_, end_;
};

} // namespace utility
} // namespace black_label



#endif
