# LL(1) Parser with Stack Implementation

## Approach

For this assignment, I extended the Context-Free Grammar (CFG) parser from Assignment 2 to include an LL(1) parser that processes input strings using a parsing stack. The implementation follows these key steps:

### 1. Integration with Existing CFG Class

I modified the CFG class to expose necessary methods for the parser:
- `getStartSymbol()`: Provides the start symbol for the grammar
- `getParsingTableEntry()`: Retrieves productions from the parsing table
- `isTerminal()`: Determines if a symbol is a terminal

### 2. Parser Class Design

The `Parser` class is responsible for:
- Initializing the parsing stack with the start symbol and end marker ($)
- Reading input strings from a file
- Tokenizing the input strings
- Performing the parsing steps according to LL(1) principles
- Detecting and reporting syntax errors
- Displaying the stack contents at each step

### 3. Parsing Algorithm

The parsing algorithm implemented follows standard LL(1) parsing principles:
1. Initialize stack with $ and start symbol
2. For each step:
   - If top of stack is a terminal:
     - If it matches current input, pop and advance input
     - Otherwise, report error and attempt recovery
   - If top of stack is a non-terminal:
     - Look up production in parsing table
     - Replace non-terminal with the right side of production (in reverse order)
   - If top of stack is $:
     - If current input is also $, accept
     - Otherwise, report error

### 4. Error Handling

The parser implements basic error handling strategies:
- For terminal mismatches: Skip the current input token
- For missing parsing table entries: Skip the current input token
- Clear error reporting with line numbers and specific error messages
- Error summary at the end of parsing

### 5. Output Formatting

The implementation uses formatted table output to clearly show:
- The stack contents at each step
- The current input token being processed
- The action taken (match, apply production, error)
- Success or failure messages for each line
- Summary of errors encountered

## Challenges Faced

1. **Stack Representation**: Displaying the stack contents while maintaining its integrity was challenging. I solved this by creating a helper function to duplicate the stack for display purposes.

2. **Error Recovery**: Implementing graceful error recovery without getting stuck in infinite loops required careful consideration. I opted for a simple approach of skipping problematic tokens.

3. **Synchronization Between CFG and Parser**: Ensuring the parser correctly accesses the parsing table entries from the CFG class required proper interface design between the classes.

## Verification

I verified the correctness of the implementation through testing with different grammar and input combinations:

1. **Syntactically Correct Inputs**: Verified that the parser correctly accepts valid input strings according to the grammar.

2. **Error Detection**: Tested various error scenarios to ensure the parser correctly identifies and reports syntax errors:
   - Missing symbols (e.g., missing closing parenthesis)
   - Unexpected tokens (e.g., unmatched terminals)
   - Invalid productions (e.g., using terminals not in grammar)

3. **Error Recovery**: Confirmed that the parser attempts to continue parsing after encountering errors.

4. **Edge Cases**: Tested with empty input, input with only whitespace, and very long productions to ensure robustness.

The implementation successfully parses input strings according to the LL(1) grammar and provides detailed feedback on the parsing process and any errors encountered.