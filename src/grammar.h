#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <vector>
#include <map>

namespace rekt {

using std::vector;
using std::map;

template<typename T>
class grammar {
    T S;
    map<T, vector<vector<T>>> prods;

public:
    grammar(const T& S) : S(S) {}

    /**
     * Adds a new production rule
     */
    void add_rule(const T& L, const vector<T>& R) {
		prods[L].push_back(R);
	}

    /**
     * TODO: docs
     */
    void add_rules(const T& L, const vector<vector<T>>& R) {
		for (auto& r : R) {
			add_rule(L, r);
		}
	}

    /**
     * Returns a random word from the grammar by using
     * leftmost derivation.
	 * 
	 * TODO: vector -> list
     */
    vector<T> produce() {
		vector<T> word;
		word.push_back(S);

		for (auto i = 0u; i < word.size();) {
			auto& c = word[i]; // current symbol
			auto prod = prods.find(c);
			if (prod != prods.end()) { // c is a variable
				auto& prod_rules = (*prod).second;
				auto& substitute = prod_rules[rand() % prod_rules.size()];
				word.erase(word.begin() + i);
				word.insert(word.begin() + i, substitute.begin(), substitute.end());
			}
			else { // c is a term
				i++;
			}
		}

		return word;
	}
};

}

#endif // GRAMMAR_H
