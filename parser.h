#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <stack>
#include "CFG.h"

using namespace std;

class Parser {
private:
    CFG* cfg;
    string startSymbol;
    int errorCount;

    string getStackContents(stack<string> stk);

public:
    Parser(CFG* grammar);
    void parseFile(const string& filename);
    void parseString(const string& input, int lineNum);
};

#endif // PARSER_H