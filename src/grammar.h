#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <vector>
#include <map>

namespace rekt {

using std::vector;
using std::map;
using std::string;

class grammar {
    string S;
    map<string, vector<vector<string>>> prods;

public:
    grammar(const string& S);

    /**
     * Adds a new production rule
     */
    void add_rule(const string& L, const vector<string>& R);

    /**
     * TODO: docs
     */
    void add_rules(const string& L, const vector<vector<string>>& R);

    /**
     * Returns a random word from the grammar by using
     * leftmost derivation.
     */
    vector<string> produce();
};

}

#endif // GRAMMAR_H
