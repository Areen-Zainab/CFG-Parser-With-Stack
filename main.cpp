#include <iostream>
#include <string> 
#include "CFG.h"
#include "Parser.h"

using namespace std;

int main() {
    string grammar_file, input_file;
    
    cout << "Enter the grammar file path: ";
    cin >> grammar_file;
    
    cout << "Enter the input file path for parsing: ";
    cin >> input_file;

    CFG cfg(grammar_file);

    cout << "\033[32m\nOriginal CFG:\033[0m\n";
    cfg.print();

    cout << "\033[32m\nStep 1: Applying Left Factoring\033[0m\n";
    cfg.LeftFactoring();
    cfg.print();

    cout << "\033[32m\nStep 2: Removing Left Recursion\033[0m\n";
    cfg.LeftRecursion();
    cfg.print();

    cout << "\033[32m\nStep 3: Computing FIRST Sets\033[0m\n";
    cfg.computeFirstSets();
    cfg.printFirstSets();

    cout << "\033[32m\nStep 4: Computing FOLLOW Sets\033[0m\n";
    cfg.computeFollowSets();
    cfg.printFollowSets();

    cout << "\033[32m\nStep 5: Constructing LL(1) Parsing Table\033[0m\n";
    cfg.constructParsingTable();
    cfg.printParsingTable();

    // Create parser and parse input file
    Parser parser(&cfg);
    parser.parseFile(input_file);

    return 0;
}