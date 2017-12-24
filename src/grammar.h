#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <vector>
#include <map>
#include <queue>

#include "node.h"

namespace rekt {

	using std::vector;
	using std::map;
	using std::queue;

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

		bool is_terminal(const T& value) {
			return prods.count(value) == 0;
		}

		bool is_variable(const T& value) {
			return !is_terminal(value);
		}

		/**
		 * Returns a random word from the grammar by using
		 * leftmost derivation.
		 * The word symbols are the leaves of the returned tree, in the
		 * order determined by a DFS, minus variables V involved in a
		 * V -> <empty> production.
		 *
		 * TODO: multithreaded expansion
		 */
		node<T>* produce() {
			node<T>* root = new node<T>(S);
			queue<node<T>*> nodes;
			nodes.push(root);
			while (!nodes.empty()) {
				node<T>* n = nodes.front();
				nodes.pop();

				//if (prod != prods.end()) {
				if (is_variable(n->value)) {
					auto& prod = prods.find(n->value);
					auto& prod_rules = (*prod).second;
					auto& substitution = prod_rules[rand() % prod_rules.size()];
					for (auto& child_value : substitution) {
						n->add_child(child_value);
						nodes.push(n->children.back());
					}
				}
			}
			return root;
		}
	};

}

#endif // GRAMMAR_H
