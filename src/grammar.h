#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <vector>
#include <map>
#include <queue>

#include "node.h"

namespace rekt {

	using std::string;
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

	class attributes {
		map<string, int> attribs_i;
		map<string, float> attribs_f;
		map<string, string> attribs_s;

		bool is_attribi(const std::string& name) {
			return attribs_i.count(name);
		}

		bool is_attribf(const std::string& name) {
			return attribs_f.count(name);
		}

		bool is_attribs(const std::string& name) {
			return attribs_s.count(name);
		}

		bool is_attrib(const string& name) {
			return is_attribi(name) || is_attribf(name) || is_attribs(name);
		}

		template<typename T>
		void check_not_attrib(const string& name, const map<string, T>& attribs) {
			if (attribs.count(name)) {
				throw std::runtime_error(name + " is already an attribute");
			}
		}

		void check_not_attrib(const string& name) {
			check_not_attrib(name, attribs_i);
			check_not_attrib(name, attribs_f);
			check_not_attrib(name, attribs_s);
		}

		template<typename T>
		void check_is_attrib(const string& name, const map<string, T>& attribs) {
			if (!attribs.count(name)) {
				throw std::runtime_error(name + " is not an attribute");
			}
		}
	public:
		// Set attribute
		void set_attribi(const string& name, int value) {
			check_not_attrib(name);
			attribs_i[name] = value;
		}

		void set_attribf(const string& name, float value) {
			check_not_attrib(name);
			attribs_f[name] = value;
		}

		void set_attribs(const string& name, const string& value) {
			check_not_attrib(name);
			attribs_s[name] = value;
		}

		// Get attribute
		int get_attribi(const string& name) {
			check_is_attrib(name, attribs_i);
			return attribs_i[name];
		}

		float get_attribf(const string& name) {
			check_is_attrib(name, attribs_f);
			return attribs_f[name];
		}

		string get_attribs(const string& name) {
			check_is_attrib(name, attribs_s);
			return attribs_s[name];
		}

		// Get attribute, with default value if the attribute is not present
		int get_attribi(const string& name, int default_value) {
			return is_attribi(name) ? attribs_i[name] : default_value;
		}

		float get_attribf(const string& name, float default_value) {
			return is_attribf(name) ? attribs_f[name] : default_value;
		}

		string get_attribs(const string& name, const string& default_value) {
			return is_attribs(name) ? attribs_s[name] : default_value;
		}
	};
}

#endif // GRAMMAR_H
