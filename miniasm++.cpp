//
// Copyright 2016 riteme
//

#include <cassert>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstring>

#include <iterator>
#include <list>
#include <random>
#include <utility>
#include <vector>

using namespace std;

/**
 * Assertion
 * @param  expr    Bool expression
 * @param  message The message that will be printed to screen via `printf` when
 * the expression is false
 * @return         No return
 */
#define ASSERT(expr, message)            \
    if (!(expr)) {                       \
        printf("(ERROR) %s\n", message); \
        exit(-1);                        \
    }

/**
 * 1 for enabling friendly mode to users
 * 0 otherwise
 */
#define FRIENDLY_MODE 0

/**
 * Generate a  random integer
 * @return Random integer
 */
inline int randint() {
    static random_device rd;

    return rd();
}

/**
 * Store ints
 */
class MemoryPool {
 public:
    /**
     * The maximum size of int array
     */
    constexpr static size_t MaxMemorySize = 10000000;

    MemoryPool() : _size(0), _mem(nullptr) {}

    MemoryPool(const size_t size) : _mem(nullptr) {
        resize(size);
    }

    ~MemoryPool() {
        if (_mem)
            delete _mem;
    }

    /**
     * Access the element in the int array
     * @param  pos Index
     * @return     int &
     */
    int &operator[](const size_t pos) {
        ASSERT(0 <= pos && pos < _size, "Memory index error");

        return _mem[pos];
    }

    /**
     * Resize  the int array
     * @param  size New size
     * @remark If succeeded, the original int array will be deleted and new
     * array is initialized again
     */
    void resize(const size_t size) {
        ASSERT(size <= MaxMemorySize, "Memory limit exceeded");

        if (_mem)
            delete _mem;

        _size = size;
        _mem = new int[size];

#if FRIENDLY_MODE
        memset(_mem, 0, sizeof(int) * size);
#else
        for (size_t i = 0; i < _size; i++)
            _mem[i] = randint();
#endif  // IF FRIENDLY_MODE
    }

 private:
    size_t _size;
    int *_mem;
};  // class MemoryPool

/**
 * Self dereferencing int value
 */
class Value {
 public:
    constexpr static size_t MaxReferenceRecursive = 256;

    Value() : _recur(0) {
#if FRIENDLY_MODE
        _value = 0;
#else
        _value = randint();
#endif  // IF FRIENDLY_MODE
    }

    /**
     * Set value
     * @param value Literal value
     * @param recur Number of dereference recursive
     */
    void set(const int value, const size_t recur) {
        _value = value;
        _recur = recur;
    }

    /**
     * Return the real value in specified memory pool
     * @param  memory Memory pool
     * @return        Real value
     */
    int get(MemoryPool *memory) const {
        ASSERT(memory != nullptr, "(internal) NULL memory pool received");
        ASSERT(_recur <= MaxReferenceRecursive, "References overflow");

        int result = _value;

        for (size_t i = 0; i < _recur; i++)
            result = (*memory)[result];

        return result;
    }

 private:
    int _value;
    size_t _recur;
};  // class Value

class Program;

class Instruction {
 public:
    static Program *env;

    virtual ~Instruction();

    /**
     * `execute` interface
     * @param  _args Command arguments
     * @return       Used time
     */
    virtual size_t execute(const void *_args) = 0;

    virtual void delete_args(const void *_args) = 0;
};  // class Instruction

Program *Instruction::env;

Instruction::~Instruction() = default;

class Program {
 public:
    /**
     * Max time count
     */
    constexpr static size_t Timelimit = 50000000;

    struct Command {
        Instruction *instruction;
        void *args;

        bool is_valid() const {
            return instruction != nullptr && args != nullptr;
        }
    };  // struct Command

    Program() : current(0), _timer(0) {}

    ~Program() {
        for (auto &e : _commands) {
            ASSERT(e.args != nullptr, "Argument missing");

            e.instruction->delete_args(e.args);
            delete e.instruction;
        }  // foreach in _commands
    }

    /**
     * Indicate that whether the program has exited
     * @return Bool
     */
    bool exited() const {
        return current >= _commands.size();
    }

    /**
     * Return the timer
     * @return size_t
     */
    size_t passed_time() const {
        return _timer;
    }

    /**
     * Append new command to the end of program
     * @param command New command
     */
    void append(const Command &command) {
        ASSERT(command.is_valid(), "(internal) NULL command received");

        _commands.push_back(command);
    }

    void make_environment() {
        Instruction::env = this;
    }

    /**
     * Run program until exited or exceeded the time limit
     */
    void run() {
        while (!exited()) {
            ASSERT(_timer <= Timelimit, "Time limit exceeded");
            ASSERT(0 <= current && current < _commands.size(),
                   "Invalid position");

            Command &comm = _commands[current];
            current++;

            ASSERT(comm.instruction != nullptr, "Invalid instruction");
            ASSERT(comm.args != nullptr, "Arguments missing");
            _timer += comm.instruction->execute(comm.args);
        }  // while
    }

    MemoryPool memory;
    size_t current;

 private:
    size_t _timer;
    vector<Command> _commands;
};  // class Program

struct PrintArgs {
    Value value;
};  // struct PrintArgs

class PrintInstruction final : public Instruction {
 public:
    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const PrintArgs *>(_args);

        printf("%d\n", args->value.get(&env->memory));
        return 1;
    }

    virtual void delete_args(const void *_args) {
        auto args = reinterpret_cast<const PrintArgs *>(_args);

        delete args;
    }
};  // class PrintInstruction

struct Token {
    constexpr static size_t MaxLexemeLength = 4096;

    Token() : size(0), lexeme(nullptr) {}

    Token(const char *buffer) : lexeme(nullptr) {
        store(buffer);
    }

    Token(const char *buffer, const size_t beg, const size_t end)
            : lexeme(nullptr) {
        store(buffer, beg, end);
    }

    Token(const Token &b) {
        store(b.lexeme);
    }

    Token(Token &&b) : size(b.size), lexeme(b.lexeme) {
        b.size = 0;
        b.lexeme = nullptr;
    }

    Token &operator=(const Token &b) {
        this->store(b.lexeme);

        return *this;
    }

    Token &operator=(Token &&b) {
        size = b.size;
        lexeme = b.lexeme;
        b.size = 0;
        b.lexeme = nullptr;

        return *this;
    }

    ~Token() {
        if (lexeme)
            delete lexeme;
    }

    void store(const char *buffer,
               const size_t beg = 0,
               const size_t end = UINT_MAX) {
        if (end < UINT_MAX)
            size = end - beg;
        else
            size = strlen(buffer + beg);

        ASSERT(size <= MaxLexemeLength, "Lexeme too loog");

        if (lexeme)
            delete lexeme;

        lexeme = new char[size];
        memcpy(lexeme, buffer + beg, sizeof(char) * size);
    }

    bool equal_to(const char *str) const {
        return equal(str, str + strlen(str), lexeme, lexeme + size);
    }

    bool is_int() const {
        return size > 0 && isdigit(lexeme[0]);
    }

    int as_int() const {
        return atoi(lexeme);
    }

    long as_long() const {
        return atol(lexeme);
    }

    long long as_long_long() const {
        return atoll(lexeme);
    }

    size_t size;
    char *lexeme;
};  // struct Token

class Tokenizer {
 public:
    list<Token> tokenize(const char *buffer) const {
        enum TokenType { UNKNOWN, ALPHAS, STARS, DIGITS };

        TokenType mode = UNKNOWN;
        size_t lastpos = 0;
        list<Token> tokens;
        for (size_t pos = 0; buffer[pos]; pos++) {
            char c = buffer[pos];

            TokenType type;
            if (isalpha(c))
                type = ALPHAS;
            else if (isdigit(c))
                type = DIGITS;
            else if (c == '*')
                type = STARS;
            else
                type = UNKNOWN;

            if (type == UNKNOWN) {
                if (mode != UNKNOWN) {
                    mode = UNKNOWN;
                    tokens.push_back(Token(buffer, lastpos, pos));
                }
            } else {
                if (mode == UNKNOWN) {
                    mode = type;
                    lastpos = pos;
                } else if (mode != type) {
                    mode = UNKNOWN;
                    tokens.push_back(Token(buffer, lastpos, pos));
                    pos--;
                }
            }
        }  // for

        return tokens;
    }
};  // class Tokenizer

class Parser {
 public:
    constexpr static size_t MaxIntegerLength = 10;

    typedef Program::Command Command;
    typedef list<Token> TokenList;

    template <typename TIterator>
    void read_value(TIterator &beg, const TIterator end, Value &target) const {
        int value;
        size_t recur = 0;
        while (true) {
            ASSERT(beg != end, "Invalid value");

            if (beg->is_int()) {
                ASSERT(beg->size <= MaxIntegerLength, "Integer too long");

                value = beg->as_int();
                break;
            } else
                recur += beg->size;

            beg++;
        }  // while

        target.set(value, recur);
    }

    Command parse_print(const TokenList &tokens) const {
        auto instruction = new PrintInstruction;
        auto args = new PrintArgs;
        auto beg = std::next(tokens.begin());
        read_value(beg, tokens.end(), args->value);

        return { instruction, args };
    }

    Command parse(const char *line) const {
        TokenList tokens = _tokenizer.tokenize(line);

        if (tokens.empty())
            return { nullptr, nullptr };

        if (tokens.front().equal_to("PRINT"))
            return parse_print(tokens);
        else
            ASSERT(false, "Unknown instruction");
    }

 private:
    Tokenizer _tokenizer;
};  // class Parser

int main() {
    freopen("test.asm", "r", stdin);

    Parser parser;
    Program program;
    program.make_environment();

    char buffer[2048];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        auto command = parser.parse(buffer);

        if (command.is_valid())
            program.append(command);
    }  // while

    program.run();

    return 0;
}  // function main
