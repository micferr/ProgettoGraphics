#include <vector>
#include <map>

#include "grammar.h"

using std::vector;
using std::map;
using std::string;

namespace rekt {

grammar::grammar(const string& S) : S(S) {}

void grammar::add_rule(const string& L, const vector<string>& R) {
    prods[L].push_back(R);
}

void grammar::add_rules(const string& L, const vector<vector<string>>& R) {
    for (auto& r : R) {
        add_rule(L, r);
    }
}

/**
 * TODO: list
 */
vector<string> grammar::produce() {
    vector<string> word;
    word.push_back(S);

    for (auto i = 0u; i < word.size();) {
        auto& c = word[i]; // current symbol
        auto prod = prods.find(c);
        if (prod != prods.end()) { // c is a variable
            auto& prod_rules = (*prod).second;
            auto& substitute = prod_rules[rand()%prod_rules.size()];
            word.erase(word.begin()+i);
            word.insert(word.begin()+i, substitute.begin(), substitute.end());
        } else { // c is a term
            i++;
        }

        return word;
    }
}
