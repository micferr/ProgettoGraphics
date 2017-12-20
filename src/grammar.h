#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <vector>
#include <map>
#include <list>

namespace rekt {

using std::vector;
using std::map;
using std::list;

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
     * Adds multiple production rules
     */
    void add_rules(const T& L, const vector<vector<T>>& R) {
		for (auto& r : R) {
			add_rule(L, r);
		}
	}

    /**
     * Returns a random word from the grammar by using
     * leftmost derivation.
     */
    vector<T> produce() {
		list<T> word;
		word.push_back(S);

		for (auto it = word.begin(); it != word.end();) {
			auto& c = *it; // current symbol
			auto prod = prods.find(c);
			if (prod != prods.end()) { // c is a variable
				auto& prod_rules = (*prod).second;
				auto& substitute = prod_rules[rand() % prod_rules.size()];
				it = word.erase(it);
				it = word.insert(it, substitute.begin(), substitute.end());
			}
			else { // c is a term
				it++;
			}
		}

		return vector<T>(word.begin(), word.end());
	}
};

}

#endif // GRAMMAR_H
