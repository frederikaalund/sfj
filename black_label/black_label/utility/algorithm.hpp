#ifndef BLACK_LABEL_UTILITY_ALGORITHM_HPP
#define BLACK_LABEL_UTILITY_ALGORITHM_HPP

#include <algorithm>
#include <array>
#include <iterator>
#include <random>



namespace black_label {
namespace utility {



////////////////////////////////////////////////////////////////////////////////
/// Forward As
/// 
/// Forwards u of type U as T would be forwarded. I.e., u is forwarded
/// like some t would be forwarded by std::forward<T>(t). The cv-qualifiers of 
/// U are preserved. E.g., forward_as<T, std::string>(u) casts u to:
///
///  (1) std::string& if T is int&
///  (2) std::string& if T is const int&
///  (3) std::string&& if T is int&&
///
/// Analogously, forward_as<T, const std::string>(u) casts u to:
///
///  (1) const std::string& if T is int&
///  (2) const std::string& if T is const int&
///  (3) const std::string&& if T is int&&
/// 
////////////////////////////////////////////////////////////////////////////////
template<typename T, typename U>
auto forward_as( U&& u ) -> std::conditional_t<
		std::is_reference<T>::value, 
		std::remove_reference_t<U>&, 
		std::remove_reference_t<U>&&>
{
	return static_cast<std::conditional_t<
		std::is_reference<T>::value, 
		std::remove_reference_t<U>&, 
		std::remove_reference_t<U>&&>>(u);
}



// Construct a range from a single element
template<typename T>
auto make_range( T& value )
{ return std::array<std::reference_wrapper<T>, 1>{value}; }

template<typename T>
auto make_const_range( const T& value )
{ return std::array<std::reference_wrapper<const T>, 1>{value}; }



// Implemented via relaxation dart throwing
template<typename Iterator>
void generate_poisson_disc_samples( Iterator first, Iterator last )
{
	using namespace std;
	using value_type = iterator_traits<Iterator>::value_type;
	random_device random_device;
	mt19937 engine{random_device()};
	uniform_real_distribution<float> distribution{-1.0f, 1.0f};
	float distance_threshold{0.5f};
	int accepted_samples{0}, rejects{0};
	static const int rejects_before_relaxation{10};
	static const float relaxation_coefficient{0.9f};
	Iterator current = first;

	while (last != current) {
		// Generate sample in unit disc
		value_type sample;
		do {
			sample.x = distribution(engine);
			sample.y = distribution(engine);
		} while (glm::length(sample) > 1.0f); // TODO: Abstract glm usage away

		// Reject sample if it is too close to any of the accepted samples
		auto reject = any_of(first, current, [distance_threshold, sample] ( auto accepted_sample )
			{ return glm::distance(sample, accepted_sample) < distance_threshold; }); // TODO: Abstract glm usage away

		if (reject) {
			if (++rejects >= rejects_before_relaxation)  {
				rejects = 0;
				// Relax the distance threshold
				distance_threshold *= relaxation_coefficient;
			}
			continue; 
		}

		// Accept the sample
		*current++ = sample;
	}
}



} // namespace utility
} // namespace black_label



#endif
