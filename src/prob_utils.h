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
		return ygl::next_rand1f(rng) <= p;
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

	/**
	 * Returns a random value in [min,max) with uniform distribution
	 */
	float uniform(ygl::rng_pcg32& rng, float min, float max) {
		return ygl::next_rand1f(rng)*(max - min) + min;
	}

	/**
	 * Generates a random value with normal distribution
	 */
	float gaussian(ygl::rng_pcg32& rng, float mu, float sigma) {
		// Box-Muller transform's polar form.
		// See
		//     G.E.P. Box, M.E. Muller
		//     "A Note on the Generation of Random Normal Deviates", 
		//     The Annals of Mathematical Statistics (1958), Vol. 29, No. 2 pp. 610–611
		// TODO?:
		//     Box-Muller suffers from tail-truncation.
		//     Methods such as Inverse CDF, Ziggurat and Ratio of uniforms
		//     may be better

		// This a simple implementation from the original Box and Muller's paper
		// as no better license-able version was found

		float u1 = ygl::next_rand1f(rng);
		float u2 = ygl::next_rand1f(rng);
		
		float x1 = sqrt(-2.f*log(u1))*cos(2.f*pi*u2);
		return x1 * sigma + mu;
	}

	/**
	 * Returns a random int in [0, weights.size()), with integer i
	 * having a probability of being chosen of weights[i]/(sum_j weights[j])
	 */
	int random_weighted(const std::vector<float>& weights, ygl::rng_pcg32& rng) {
		float weights_total = 0.f;
		for (auto w : weights) weights_total += w;
		float r = ygl::next_rand1f(rng, 0.f, weights_total);
		int which = 0;
		while (r > weights[which]) {
			r -= weights[which];
			which++;
		}
		return which;
	}

	/**
	 * Choose a random element from a vector
	 */
	template<typename T>
	T choose_random(const std::vector<T>& v, ygl::rng_pcg32& rng) {
		return v[ygl::next_rand1i(rng, v.size())];
	}

	/**
	 * Randomly picks an element from a vector; element v[i] is chosen
	 * with probability weights[i]/(sum_j weights[j])
	 */
	template<typename T>
	T choose_random_weighted(
		const std::vector<T>& v,
		const std::vector<float> weights, 
		ygl::rng_pcg32& rng
	) {
		if (v.size() != weights.size()) {
			throw std::exception("v and weights must have equal size");
		}
		if (v.size() == 0) {
			throw std::exception("Must pick from at least one element");
		}
		return v[random_weighted(weights, rng)];
	}
}

#endif // PROB_UTILS_H