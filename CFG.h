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
            cout << "\033[0;31mError: No CFG found!\033[0m" << endl;
            return;
        }

        // using a set takay no repeatitions
        set<string> terminals, non_terminals;
        for (const auto& entry : parsing_table) {
            non_terminals.insert(entry.first.first);  // row headers pr Non-terminals
            terminals.insert(entry.first.second);    // column headers pr Terminals
        }
    
        // sabki width takay beautiful table
        unordered_map<string, size_t> col_width;
        int min_width = 12; // min width for columns
    
        for (const string& terminal : terminals) {
            col_width[terminal] = terminal.size() + 3; 
        }
    
        for (const auto& entry : parsing_table) {
            string production = entry.first.first + " -> ";
            for (const string& symbol : entry.second) {
                production += symbol + " ";
            }
            col_width[entry.first.second] = max(col_width[entry.first.second], production.size() + 2);
        }

        // total width takay dash dash print ho
        int total_width = min_width + 4;
        for (const string& terminal : terminals) {
            total_width += col_width[terminal] + 2;
        }
        cout << string(total_width / 2 - 11, '=') << "  LL(1) Parsing Table " << string(total_width / 2 - 11, '=') << "\n";
    
        // column headerssss
        cout << setw(min_width) << left << "| Non-Terminal |";
        for (const string& terminal : terminals) {
            cout << " " << setw(col_width[terminal] - 1) << left << terminal << " |";
        }
        cout << "\n";
    
        cout << string(total_width, '=') << "\n";
    
        // this is the actual printing part
        for (const string& row_header : non_terminals) {
            cout << "| " << setw(min_width) << left << row_header << " |";
    
            for (const string& terminal : terminals) {

                auto key = make_pair(row_header, terminal);

                if (parsing_table.find(key) != parsing_table.end()) {
                    string production = row_header + " -> ";
                    for (const string& symbol : parsing_table.at(key)) {
                        production += symbol + " ";
                    }
                    cout << setw(col_width[terminal]) << left << production << " |";
                } 
                else {
                    cout << setw(col_width[terminal]) << left << "-" << " |";
                }

            }
            cout << "\n";
        }
    
        // table ka bottom border
        cout << string(total_width, '=') << "\n";
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