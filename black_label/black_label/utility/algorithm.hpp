#ifndef BLACK_LABEL_UTILITY_ALGORITHM_HPP
#define BLACK_LABEL_UTILITY_ALGORITHM_HPP

#include <algorithm>
#include <iterator>
#include <random>



namespace black_label {
namespace utility {



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
