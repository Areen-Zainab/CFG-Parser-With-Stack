#ifndef CFG_H
#define CFG_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <numeric>
using namespace std;

class CFG {
private:
    map<string, vector<vector<string>>> productions;  
    map<string, set<string>> firstSets; 
    map<string, set<string>> followSets; 
    map<pair<string, string>, vector<string>> parsing_table;
    string start_symbol;

public:
    CFG(const string& filename) {
        read_from_file(filename);
        if (!productions.empty()) {
            start_symbol = begin(productions)->first; // Use the first non-terminal as start symbol
        }
    }

    string getStartSymbol() const {
        return start_symbol;
    }

    vector<string> getParsingTableEntry(const string& nonTerminal, const string& terminal) const {
        auto key = make_pair(nonTerminal, terminal);
        if (parsing_table.find(key) != parsing_table.end()) {
            return parsing_table.at(key);
        }
        return {}; // Return empty vector if no entry found
    }

    bool isTerminal(const string& symbol) const {
        return productions.find(symbol) == productions.end() && symbol != "ε";
    }

    void read_from_file(const string& filename) {
        ifstream file(filename);
        string line;

        while (getline(file, line)) {
            istringstream read(line);
            string nonTerminal, arrow, production;
            read >> nonTerminal >> arrow; // read the shuro ka "S ->"

            vector<vector<string>> rules;
            vector<string> currentRule;
            while (read >> production) {
                if (production == "|") {    // '|' will mean that a new production rule has started --> save the old one
                    rules.push_back(currentRule);
                    currentRule.clear();
                } 
                else {
                    currentRule.push_back(production);
                }
            }
            if (!currentRule.empty()) {
                rules.push_back(currentRule);
            }

            productions[nonTerminal] = rules;
        }

        file.close();
    }

    void print() const {
        if (productions.empty()) {
            cout << "\033[0;31mError: No CFG found!\033[0m" << endl;
            return;
        }
        for (const auto& rule : productions) {
            cout << rule.first << " -> ";
            for (int i = 0; i < rule.second.size(); i++) {
                for (const auto& symbol : rule.second[i]) {
                    cout << symbol << " ";
                }
                if (i != rule.second.size() - 1) {
                    cout << "| ";
                }
            }
            cout << endl;
        }
    }

    void LeftRecursion() {
        if (productions.empty()) {
            return;
        }
        map<string, vector<vector<string>>> naya_cfg;

        for (auto& rule : productions) {
            string non_terminal = rule.first; // rule.first main non-terminal, rule.second main uski productions
            vector<vector<string>> recusrive_prod, normal;

            for (auto& prod : rule.second) {
                if (!prod.empty() && prod[0] == non_terminal) { // if left recursion ---> save to recusrive_prod
                    recusrive_prod.push_back(vector<string>(prod.begin() + 1, prod.end()));
                } else {
                    normal.push_back(prod);
                }
            }

            if (recusrive_prod.empty()) {
                naya_cfg[non_terminal] = rule.second;
            } 
            else {
                string new_nonTerminal = non_terminal + "'";

                for (auto& prod : normal) {
                    prod.push_back(new_nonTerminal);
                    naya_cfg[non_terminal].push_back(prod);
                }

                for (auto& prod : recusrive_prod) {
                    prod.push_back(new_nonTerminal);
                    naya_cfg[new_nonTerminal].push_back(prod);
                }
                naya_cfg[new_nonTerminal].push_back({"ε"});
            }
        }

        productions = naya_cfg;
    }

    void LeftFactoring() {
        if (productions.empty()) {
            return;
        }
        map<string, vector<vector<string>>> naya_cfg;
        int new_nonterm_count = 1;

        for (const auto& rule : productions) {
            string non_terminal = rule.first;
            const vector<vector<string>>& prods = rule.second;
            map<string, vector<vector<string>>> grouped_prods;

            for (int i = 0; i < prods.size(); i++) {
                string prefix = prods[i][0];

                for (int j = i + 1; j < prods.size(); j++) {
                    if (prods[j][0] == prefix) {
                        grouped_prods[prefix].push_back(prods[j]); // if productions have the same lhs, add to grouped
                    }
                }
                grouped_prods[prefix].push_back(prods[i]);
            }

            for (const auto& group : grouped_prods) {
                string prefix = group.first;    // the common lhs wala part
                const vector<vector<string>>& groupProds = group.second;    // the productions that share it

                if (groupProds.size() == 1) {
                    naya_cfg[non_terminal].push_back(groupProds[0]);    
                } 
                else {
                    string factored_nt = non_terminal + "_F" + to_string(new_nonterm_count++);
                    naya_cfg[non_terminal].push_back({prefix, factored_nt});

                    for (const vector<string>& prod : groupProds) {
                        vector<string> suffix(prod.begin() + 1, prod.end());
                        if (suffix.empty()) {
                            suffix.push_back("ε");
                        }
                        naya_cfg[factored_nt].push_back(suffix);
                    }
                }
            }
        }

        productions = naya_cfg;
    }

    void computeFirstSets() {
        if (productions.empty()) {
            return;
        }

        bool changed = true;

        while (changed) {
            changed = false;

            for (const auto& rule : productions) {
                string non_terminal = rule.first;

                for (const auto& production : rule.second) {
                    string pehla_symbol = production[0];

                    if (isTerminal(pehla_symbol)) { // agar terminal hai tou add to the first set
                        if (firstSets[non_terminal].insert(pehla_symbol).second)
                            changed = true;
                    } 
                    else {
                        for (const string& symbol : production) {   
                            if (!isTerminal(symbol)) {
                                int curr_size = firstSets[non_terminal].size();
                                firstSets[non_terminal].insert(firstSets[symbol].begin(), firstSets[symbol].end());

                                if (firstSets[symbol].count("ε") == 0)
                                    break;

                                if (firstSets[non_terminal].size() > curr_size)
                                    changed = true;
                            } 
                            else {
                                if (firstSets[non_terminal].insert(symbol).second)
                                    changed = true;

                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    void computeFollowSets() {
        if (productions.empty()) {
            return;
        }
        followSets[begin(productions)->first].insert("$"); // add the dolla dolla for every start symbol '$'

        bool changed = true;
        while (changed) {
            changed = false;

            for (const auto& rule : productions) {
                string nonTerminal = rule.first;

                for (const auto& production : rule.second) {
                    for (int i = 0; i < production.size(); i++) {
                        string symbol = production[i];

                        if (!isTerminal(symbol)) {
                            set<string> followToAdd;

                            if (i + 1 < production.size()) {
                                string nextSymbol = production[i + 1];

                                if (isTerminal(nextSymbol)) {
                                    followToAdd.insert(nextSymbol);
                                } 
                                else {
                                    followToAdd.insert(firstSets[nextSymbol].begin(), firstSets[nextSymbol].end());
                                    followToAdd.erase("ε");

                                    if (firstSets[nextSymbol].count("ε")) {
                                        followToAdd.insert(followSets[nonTerminal].begin(), followSets[nonTerminal].end());
                                    }
                                }
                            } 
                            else {
                                followToAdd.insert(followSets[nonTerminal].begin(), followSets[nonTerminal].end());
                            }

                            int curr_size = followSets[symbol].size();
                            followSets[symbol].insert(followToAdd.begin(), followToAdd.end());

                            if (followSets[symbol].size() > curr_size)
                                changed = true;
                        }
                    }
                }
            }
        }
    }

    void constructParsingTable() {
        if (productions.empty()) {
            return;
        }
        for (const auto& rule : productions) {
            string nonTerminal = rule.first;

            for (const auto& production : rule.second) {
                set<string> prod_fset = getProductionFirstSet(production); 

                for (const string& terminal : prod_fset) {
                    if (terminal != "ε") {
                        parsing_table[{nonTerminal, terminal}] = production;
                    }
                }

                if (prod_fset.count("ε")) {
                    for (const string& terminal : followSets[nonTerminal]) {
                        parsing_table[{nonTerminal, terminal}] = production;
                    }
                }
            }
        }
    }

    void printFirstSets() {
        if (productions.empty()) {
            cout << "\033[0;31mError: No CFG found!\033[0m" << endl;
            return;
        }
        for (const auto& entry : firstSets) {
            cout << "FIRST(" << entry.first << ") = { ";
            int count = 0;
            int setSize = entry.second.size();
            
            for (const auto& symbol : entry.second) {
                cout << symbol;
                if (++count < setSize) { // comma for sab except the last element
                    cout << ", ";
                }
            }
            cout << "}\n";
        }
    }

    void printFollowSets() {
        if (productions.empty()) {
            cout << "\033[0;31mError: No CFG found!\033[0m" << endl;
            return;
        }
        for (const auto& entry : followSets) {
            cout << "FOLLOW(" << entry.first << ") = { ";
            int count = 0;
            int setSize = entry.second.size();
            
            for (const auto& symbol : entry.second) {
                cout << symbol;
                if (++count < setSize) { 
                    cout << ", ";
                }
            }
            cout << "}\n";
        }
    }
  
    void printParsingTable() const {
        if (productions.empty()) {
            cout << "Error: No CFG found!" << endl;
            return;
        }
    
        // Collect all non-terminals and terminals
        set<string> terminals, non_terminals;
        for (const auto& entry : parsing_table) {
            non_terminals.insert(entry.first.first);
            terminals.insert(entry.first.second);
        }
    
        // Calculate appropriate column width
        const int nonterm_width = 15;
        const int term_width = 20;
        
        // Total width calculation
        int total_width = nonterm_width + (term_width * terminals.size()) + terminals.size() + 1;
        
        // Print header
        cout << string(total_width, '=') << endl;
        cout << "LL(1) PARSING TABLE" << endl;
        cout << string(total_width, '=') << endl;
        
        // Print column headers
        cout << left << setw(nonterm_width) << "NON-TERMINAL";
        cout << "|";
        for (const string& terminal : terminals) {
            cout << setw(term_width) << terminal << "|";
        }
        cout << endl;
        
        // Print separator
        cout << string(nonterm_width, '-');
        for (size_t i = 0; i < terminals.size(); i++) {
            cout << "+" << string(term_width, '-');
        }
        cout << "+" << endl;
        
        // Print table contents
        for (const string& nt : non_terminals) {
            cout << left << setw(nonterm_width) << nt << "|";
            
            for (const string& t : terminals) {
                auto key = make_pair(nt, t);
                
                if (parsing_table.find(key) != parsing_table.end()) {
                    string production = nt + " -> ";
                    for (const string& symbol : parsing_table.at(key)) {
                        production += symbol + " ";
                    }
                    cout << setw(term_width) << production;
                } else {
                    cout << setw(term_width) << " ";
                }
                cout << "|";
            }
            cout << endl;
        }
        
        // Print footer
        cout << string(total_width, '=') << endl;
    }
    
    private:
    set<string> getProductionFirstSet(const vector<string>& production) {
        set<string> result;

        for (const string& symbol : production) {
            // agar terminal tou usko add
            if (isTerminal(symbol)) {
                result.insert(symbol);
                break;
            } 
            else {
                // agar non-terminal tou uske first ko add
                result.insert(firstSets[symbol].begin(), firstSets[symbol].end());

                if (firstSets[symbol].count("ε") == 0)
                    break;
            }
        }

        return result;
    }
};

#endif // CFG_H