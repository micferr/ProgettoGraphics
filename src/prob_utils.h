#ifndef PROB_UTILS_H
#define PROB_UTILS_H

#include <limits>
#include <ctime>

#include "yocto\yocto_gl.h"

namespace rekt {

	/**
	 * Returns a random boolean; true is returned with p probability
	 */
	bool bernoulli(float p, ygl::rng_pcg32& rng) {
		if (p < 0 || p > 1) throw std::exception("Invalid probability value.");
		return ygl::next_rand1f(rng) > p;
	}

	/**
	 * Counts the number of consecutive failures before a success of
	 * a Bernoulli random variable with success probability p.
	 * 
	 * Note that the expected runtime is O(max-min)
	 */
	int geometric(
		float p,
		ygl::rng_pcg32& rng,
		unsigned min = 0,
		unsigned max = std::numeric_limits<unsigned>::max()
	) {
		if (max < min) throw std::exception("Invalid min-max range");
		int n = min;
		while (n < max && !bernoulli(p, rng)) n++;
		return n;
	}

	/**
	 * A geometric random variable's value represents the number of failures
	 * before the first success.
	 * This function is an utility around the definition of the geometric r.v.,
	 * and returns the number of successes before the first failure (i.e. it
	 * computes the value of a geometric r.v. with parameter p' = 1-p)
	 */
	int consecutive_bernoulli_successes(
		float p,
		ygl::rng_pcg32& rng,
		unsigned min = 0,
		unsigned max = std::numeric_limits<unsigned>::max()
	) {
		return geometric(1.f - p, rng, min, max);
	}

	/**
	 * Generates a sequence of n random booleans, each of which is a Bernoulli
	 * random variable with success probability p
	 */
	vector<bool> bernoulli_seq(unsigned n, float p, ygl::rng_pcg32& rng) {
		vector<bool> v;
		while (n--) v.emplace_back(bernoulli(p, rng));
		return v;
	}

	/**
	 * Given the expected value of a geometric random variable, 
	 * returns the success probability of each Bernoulli trial
	 */
	float bernoulli_prob_from_geometric_expected_value(float n) {
		// Expected(p) = E = (1-p)/p
		// E*p = 1-p (p != 0)
		// E*p + p = 1
		// p(E+1) = 1
		// p = 1/(E+1)
		if (n <= 0.f) throw std::exception("Invalid expected value for geometric r.v.");
		return 1.f / (n + 1.f);
	}

}

#endif // PROB_UTILS_H