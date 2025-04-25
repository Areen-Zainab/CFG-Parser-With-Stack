#include "Parser.h"

Parser::Parser(CFG* grammar) {
    cfg = grammar;
    // Assuming the first non-terminal in the grammar is the start symbol
    startSymbol = cfg->getStartSymbol();
    errorCount = 0;
}

void Parser::parseFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "\033[31mError: Unable to open input file!\033[0m" << endl;
        return;
    }

    string line;
    int lineNum = 1;

    cout << "\n\033[1;36m========== PARSING INPUT STRINGS ==========\033[0m\n";
    
    while (getline(file, line)) {
        cout << "\n\033[1;33mParsing Line " << lineNum << ": \"" << line << "\"\033[0m\n";
        parseString(line, lineNum);
        lineNum++;
    }

    cout << "\n\033[1;36m========== PARSING SUMMARY ==========\033[0m\n";
    if (errorCount == 0) {
        cout << "\033[1;32mParsing completed successfully with no errors.\033[0m\n";
    } else {
        cout << "\033[1;31mParsing completed with " << errorCount << " error(s).\033[0m\n";
    }

    file.close();
}

void Parser::parseString(const string& input, int lineNum) {
    istringstream iss(input);
    vector<string> tokens;
    string token;

    // Tokenize input string
    while (iss >> token) {
        tokens.push_back(token);
    }
    tokens.push_back("$"); // Add end marker

    // Initialize stack with end marker and start symbol
    stack<string> parsingStack;
    parsingStack.push("$");
    parsingStack.push(startSymbol);

    int inputPos = 0;
    bool hasError = false;
    
    // Print table header for parsing steps
    cout << "\n\033[1;34m+-----------------------+----------------------+------------------------+\033[0m";
    cout << "\n\033[1;34m| Stack                 | Current Input        | Action                 |\033[0m";
    cout << "\n\033[1;34m+-----------------------+----------------------+------------------------+\033[0m";

    // Parsing algorithm
    while (!parsingStack.empty() && inputPos < tokens.size()) {
        string currentInput = tokens[inputPos];
        
        // Print current stack contents
        string stackContent = getStackContents(parsingStack);
        
        // Format and print current state
        cout << "\n\033[0m| " << left << setw(21) << stackContent;
        cout << "| " << left << setw(20) << currentInput;
        
        string top = parsingStack.top();
        parsingStack.pop();
        
        // Case 1: Top is end marker
        if (top == "$") {
            if (currentInput == "$") {
                cout << "| Accept                 |";
                break;
            } else {
                cout << "| \033[31mError: Expected end of input\033[0m |";
                errorCount++;
                hasError = true;
                break;
            }
        }
        
        // Case 2: Top is a terminal
        else if (cfg->isTerminal(top)) {
            if (top == currentInput) {
                cout << "| Match and advance       |";
                inputPos++;
            } else {
                cout << "| \033[31mError: Expected '" << top << "'\033[0m |";
                // Error recovery: Skip current input token
                errorCount++;
                hasError = true;
                parsingStack.push(top); // Put back the token
                inputPos++; // Skip the problematic input token
            }
        }
        
        // Case 3: Top is a non-terminal
        else {
            vector<string> production = cfg->getParsingTableEntry(top, currentInput);
            
            if (production.empty()) {
                cout << "| \033[31mError: No production for (" << top << ", " << currentInput << ")\033[0m |";
                // Error recovery: Skip the problematic non-terminal
                errorCount++;
                hasError = true;
                inputPos++; // Skip input token
            } else {
                // Format production for display
                string productionStr = top + " -> ";
                for (const auto& symbol : production) {
                    productionStr += symbol + " ";
                }
                cout << "| Apply: " << left << setw(14) << productionStr << "|";
                
                // Push production in reverse order
                if (production.size() == 1 && production[0] == "ε") {
                    // If production is epsilon, don't push anything
                } else {
                    for (int i = production.size() - 1; i >= 0; i--) {
                        parsingStack.push(production[i]);
                    }
                }
            }
        }
    }
    
    cout << "\n\033[1;34m+-----------------------+----------------------+------------------------+\033[0m\n";
    
    // Final result for this line
    if (hasError) {
        cout << "\033[31mLine " << lineNum << ": Parsing failed with errors.\033[0m\n";
    } else if (parsingStack.top() == "$" && tokens[inputPos] == "$") {
        cout << "\033[32mLine " << lineNum << ": Parsing successful!\033[0m\n";
    } else {
        errorCount++;
        cout << "\033[31mLine " << lineNum << ": Parsing failed. ";
        
        if (!parsingStack.empty() && parsingStack.top() != "$") {
            cout << "Unexpected end of input. Expected: " << parsingStack.top() << "\033[0m\n";
        } else if (inputPos < tokens.size() - 1) {
            cout << "Extra input after parsing completed.\033[0m\n";
        } else {
            cout << "Unknown error.\033[0m\n";
        }
    }
}

string Parser::getStackContents(stack<string> stk) {
    string result = "";
    vector<string> items;
    
    // Extract items from stack
    while (!stk.empty()) {
        items.push_back(stk.top());
        stk.pop();
    }
    
    // Reconstruct the stack content string (in correct order from bottom to top)
    for (int i = items.size() - 1; i >= 0; i--) {
        result += items[i];
        if (i > 0) result += " ";
    }
    
    return result;
}