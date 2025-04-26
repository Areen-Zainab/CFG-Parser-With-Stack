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
    vector<string> nonTerminalOrder;  // Track the original order of non-terminals

public:
    CFG(const string& filename) {
        read_from_file(filename);
        if (!productions.empty()) {
            start_symbol = nonTerminalOrder[0]; // Use the first non-terminal from original order as start symbol
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
            read >> nonTerminal >> arrow; // read the non-terminal and arrow "S ->"

            // Add to order list if not already there
            if (find(nonTerminalOrder.begin(), nonTerminalOrder.end(), nonTerminal) == nonTerminalOrder.end()) {
                nonTerminalOrder.push_back(nonTerminal);
            }

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
        // Print in original order using nonTerminalOrder
        for (const auto& nonTerminal : nonTerminalOrder) {
            if (productions.find(nonTerminal) != productions.end()) {
                cout << nonTerminal << " -> ";
                const auto& rules = productions.at(nonTerminal);
                for (int i = 0; i < rules.size(); i++) {
                    for (const auto& symbol : rules[i]) {
                        cout << symbol << " ";
                    }
                    if (i != rules.size() - 1) {
                        cout << "| ";
                    }
                }
                cout << endl;
            }
        }
    }

    void LeftRecursion() {
        if (productions.empty()) {
            return;
        }
        map<string, vector<vector<string>>> naya_cfg;
        vector<string> new_order;  // New order of non-terminals

        for (const auto& non_terminal : nonTerminalOrder) {
            new_order.push_back(non_terminal);  // Keep original non-terminal in order
            
            auto& rule = productions.at(non_terminal);
            vector<vector<string>> recusrive_prod, normal;

            for (auto& prod : rule) {
                if (!prod.empty() && prod[0] == non_terminal) { // if left recursion ---> save to recusrive_prod
                    recusrive_prod.push_back(vector<string>(prod.begin() + 1, prod.end()));
                } else {
                    normal.push_back(prod);
                }
            }

            if (recusrive_prod.empty()) {
                naya_cfg[non_terminal] = rule;
            } 
            else {
                string new_nonTerminal = non_terminal + "'";
                new_order.push_back(new_nonTerminal);  // Add new non-terminal to order

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
        nonTerminalOrder = new_order;  // Update the order
    }

    void LeftFactoring() {
        if (productions.empty()) {
            return;
        }
        map<string, vector<vector<string>>> naya_cfg;
        vector<string> new_order;  // New order of non-terminals
        int new_nonterm_count = 1;

        for (const auto& non_terminal : nonTerminalOrder) {
            new_order.push_back(non_terminal);  // Keep original non-terminal in order
            
            const vector<vector<string>>& prods = productions.at(non_terminal);
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
                    new_order.push_back(factored_nt);  // Add new factored non-terminal to order
                    
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
        nonTerminalOrder = new_order;  // Update the order
    }

    void computeFirstSets() {
        if (productions.empty()) {
            return;
        }

        // Initialize FIRST sets for all terminals and non-terminals
        for (const auto& rule : productions) {
            string non_terminal = rule.first;
            firstSets[non_terminal] = {};
            
            // Check for empty productions and add epsilon
            for (const auto& production : rule.second) {
                if (production.size() == 1 && production[0] == "ε") {
                    firstSets[non_terminal].insert("ε");
                    break;
                }
            }
        }

        bool changed = true;

        while (changed) {
            changed = false;

            for (const auto& rule : productions) {
                string non_terminal = rule.first;

                for (const auto& production : rule.second) {
                    // Handle empty production explicitly
                    if (production.size() == 1 && production[0] == "ε") {
                        if (firstSets[non_terminal].insert("ε").second)
                            changed = true;
                        continue;
                    }

                    // Handle regular productions
                    bool allDeriveEpsilon = true;
                    for (size_t i = 0; i < production.size(); i++) {
                        string symbol = production[i];
                        
                        if (isTerminal(symbol)) {
                            if (firstSets[non_terminal].insert(symbol).second)
                                changed = true;
                            allDeriveEpsilon = false;
                            break;
                        } 
                        else { // Symbol is a non-terminal
                            // Add all non-epsilon symbols from FIRST(symbol) to FIRST(non_terminal)
                            for (const auto& first : firstSets[symbol]) {
                                if (first != "ε") {
                                    if (firstSets[non_terminal].insert(first).second)
                                        changed = true;
                                }
                            }
                            
                            // If this symbol doesn't derive epsilon, stop here
                            if (firstSets[symbol].count("ε") == 0) {
                                allDeriveEpsilon = false;
                                break;
                            }
                            
                            // If this is the last symbol and it derives epsilon, add epsilon to FIRST(non_terminal)
                            if (i == production.size() - 1 && firstSets[symbol].count("ε") > 0) {
                                if (firstSets[non_terminal].insert("ε").second)
                                    changed = true;
                            }
                        }
                    }
                    
                    // If all symbols in the production derive epsilon, add epsilon to FIRST(non_terminal)
                    if (allDeriveEpsilon && production.size() > 0) {
                        if (firstSets[non_terminal].insert("ε").second)
                            changed = true;
                    }
                }
            }
        }
    }

    void computeFollowSets() {
        if (productions.empty()) {
            return;
        }
        
        // Initialize all FOLLOW sets
        for (const auto& rule : productions) {
            followSets[rule.first] = {};
        }
        
        // Add $ to FOLLOW(start_symbol)
        followSets[start_symbol].insert("$");

        bool changed = true;
        while (changed) {
            changed = false;

            for (const auto& rule : productions) {
                string nonTerminal = rule.first;

                for (const auto& production : rule.second) {
                    // Skip epsilon productions
                    if (production.size() == 1 && production[0] == "ε") {
                        continue;
                    }
                    
                    for (int i = 0; i < production.size(); i++) {
                        string symbol = production[i];

                        // Only interested in non-terminals
                        if (!isTerminal(symbol)) {
                            // Case 1: symbol is followed by another symbol
                            if (i + 1 < production.size()) {
                                string nextSymbol = production[i + 1];
                                
                                if (isTerminal(nextSymbol)) {
                                    // If nextSymbol is a terminal, add it to FOLLOW(symbol)
                                    if (followSets[symbol].insert(nextSymbol).second)
                                        changed = true;
                                } 
                                else {
                                    // If nextSymbol is a non-terminal, add FIRST(nextSymbol) - {ε} to FOLLOW(symbol)
                                    for (const auto& first : firstSets[nextSymbol]) {
                                        if (first != "ε") {
                                            if (followSets[symbol].insert(first).second)
                                                changed = true;
                                        }
                                    }
                                    
                                    // If nextSymbol can derive epsilon, need to consider what comes after it
                                    if (firstSets[nextSymbol].count("ε") > 0) {
                                        // Add FOLLOW(nonTerminal) to FOLLOW(symbol)
                                        for (const auto& follow : followSets[nonTerminal]) {
                                            if (followSets[symbol].insert(follow).second)
                                                changed = true;
                                        }
                                    }
                                }
                            } 
                            // Case 2: symbol is at the end of the production
                            else {
                                // Add FOLLOW(nonTerminal) to FOLLOW(symbol)
                                for (const auto& follow : followSets[nonTerminal]) {
                                    if (followSets[symbol].insert(follow).second)
                                        changed = true;
                                }
                            }
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
        
        // Clear the parsing table first
        parsing_table.clear();
        
        // For each production rule
        for (const auto& rule : productions) {
            string nonTerminal = rule.first;

            for (const auto& production : rule.second) {
                // Handle epsilon production specially
                if (production.size() == 1 && production[0] == "ε") {
                    // For each terminal in FOLLOW(nonTerminal), add this epsilon production
                    for (const string& follow : followSets[nonTerminal]) {
                        parsing_table[{nonTerminal, follow}] = {"ε"};
                    }
                    continue;
                }
                
                // For non-epsilon productions, compute FIRST set of the production
                set<string> prodFirst = getProductionFirstSet(production);
                
                // For each terminal in FIRST(production), add this production
                for (const string& terminal : prodFirst) {
                    if (terminal != "ε") {
                        parsing_table[{nonTerminal, terminal}] = production;
                    }
                }
                
                // If FIRST(production) contains epsilon, add this production for each terminal in FOLLOW(nonTerminal)
                if (prodFirst.count("ε") > 0) {
                    for (const string& follow : followSets[nonTerminal]) {
                        parsing_table[{nonTerminal, follow}] = production;
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
        
        // Empty production directly yields epsilon
        if (production.empty() || (production.size() == 1 && production[0] == "ε")) {
            result.insert("ε");
            return result;
        }

        // Calculate FIRST set for the production
        bool allCanDeriveEpsilon = true;
        
        for (const string& symbol : production) {
            // If it's a terminal, add it and we're done
            if (isTerminal(symbol)) {
                result.insert(symbol);
                allCanDeriveEpsilon = false;
                break;
            } 
            else {
                // Add all non-epsilon symbols from FIRST(symbol)
                for (const auto& first : firstSets[symbol]) {
                    if (first != "ε") {
                        result.insert(first);
                    }
                }
                
                // If this symbol doesn't derive epsilon, we stop here
                if (firstSets[symbol].count("ε") == 0) {
                    allCanDeriveEpsilon = false;
                    break;
                }
            }
        }
        
        // If all symbols can derive epsilon, add epsilon to the result
        if (allCanDeriveEpsilon) {
            result.insert("ε");
        }

        return result;
    }
};

#endif // CFG_H