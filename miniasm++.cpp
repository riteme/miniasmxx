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

///////////////////////////////
// HELPER FUNCTIONS & MARCOS //
///////////////////////////////

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

/////////////////
// MEMORY POOL //
/////////////////

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

///////////
// VALUE //
///////////

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

/////////////////
// INSTRUCTION //
/////////////////

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

/////////////
// PROGRAM //
/////////////

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

///////////
// TOKEN //
///////////

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

    bool is_comment() const {
        return size > 0 && lexeme[0] == '#';
    }

    long long as_long_long() const {
        return atoll(lexeme);
    }

    size_t size;
    char *lexeme;
};  // struct Token

///////////////
// TOKENIZER //
///////////////

class Tokenizer {
 public:
    list<Token> tokenize(const char *buffer) const {
        enum TokenType { UNKNOWN, ALPHAS, SIGNS, DIGITS };

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
            else if (c == '*' || c == '#')
                type = SIGNS;
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

/////////////////////////////////
// INSTRUCTION IMPLEMENTATIONS //
/////////////////////////////////

#define GET(name) args->name.get(&env->memory)
#define IMPLEMENT_BASIS(args_type)                              \
    virtual void delete_args(const void *_args) {               \
        auto args = reinterpret_cast<const args_type *>(_args); \
        delete args;                                            \
    }                                                           \
    typedef args_type ArgsType;

class NopInstruction final : public Instruction {
 public:
    struct NopArgs {};  // struct NopArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const NopArgs *>(_args);

        return 0;
    }

    IMPLEMENT_BASIS(NopArgs)
};  // class NopInstruction

class TaggedNopInstruction final : public Instruction {
 public:
    struct TaggedNopArgs {
        Value index;
    };  // struct TaggedNopArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const TaggedNopArgs *>(_args);

        env->memory[GET(index)] = env->current;

        return 0;
    }

    IMPLEMENT_BASIS(TaggedNopArgs)
};  // class TaggedNopInstruction

class MemInstruction final : public Instruction {
 public:
    struct MemArgs {
        Value value;
    };  // struct MemArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const MemArgs *>(_args);

        env->memory.resize(GET(value));

        return 0;
    }

    IMPLEMENT_BASIS(MemArgs)
};  // class MemInstruction

class InInstruction final : public Instruction {
 public:
    struct InArgs {
        Value index;
    };  // struct InArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const InArgs *>(_args);

        int result;
        scanf("%d", &result);
        env->memory[GET(index)] = result;

        return 0;
    }

    IMPLEMENT_BASIS(InArgs)
};  // class InInstruction

class OutInstruction final : public Instruction {
 public:
    struct OutArgs {
        Value value;
    };  // struct OutArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const OutArgs *>(_args);

        printf("%d\n", GET(value));
        return 0;
    }

    IMPLEMENT_BASIS(OutArgs)
};  // class OutInstruction

class SetInstruction final : public Instruction {
 public:
    struct SetArgs {
        Value value;
        Value index;
    };  // struct SetArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const SetArgs *>(_args);

        env->memory[GET(index)] = GET(value);

        return 0;
    }

    IMPLEMENT_BASIS(SetArgs)
};  // class SetInstruction

class AddInstruction final : public Instruction {
 public:
    struct AddArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct AddArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const AddArgs *>(_args);

        env->memory[GET(index)] = GET(value1) + GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(AddArgs)
};  // class AddInstruction

class SubInstruction final : public Instruction {
 public:
    struct SubArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct SubArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const SubArgs *>(_args);

        env->memory[GET(index)] = GET(value1) - GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(SubArgs)
};  // class SubInstruction

class MulInstruction final : public Instruction {
 public:
    struct MulArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct MulArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const MulArgs *>(_args);

        env->memory[GET(index)] = GET(value1) * GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(MulArgs)
};  // class MulInstruction

class DivInstruction final : public Instruction {
 public:
    struct DivArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct DivArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const DivArgs *>(_args);

        env->memory[GET(index)] = GET(value1) / GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(DivArgs)
};  // class DivInstruction

class ModInstruction final : public Instruction {
 public:
    struct ModArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct ModArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const ModArgs *>(_args);

        env->memory[GET(index)] = GET(value1) % GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(ModArgs)
};  // class ModInstruction

class IncInstruction final : public Instruction {
 public:
    struct IncArgs {
        Value value;
        Value index;
    };  // struct IncArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const IncArgs *>(_args);

        env->memory[GET(index)] = GET(value) + 1;

        return 0;
    }

    IMPLEMENT_BASIS(IncArgs)
};  // class IncInstruction

class DecInstruction final : public Instruction {
 public:
    struct DecArgs {
        Value value;
        Value index;
    };  // struct DecArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const DecArgs *>(_args);

        env->memory[GET(index)] = GET(value) - 1;

        return 0;
    }

    IMPLEMENT_BASIS(DecArgs)
};  // class DecInstruction

class NecInstruction final : public Instruction {
 public:
    struct NecArgs {
        Value value;
        Value index;
    };  // struct NecArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const NecArgs *>(_args);

        env->memory[GET(index)] = -GET(value);

        return 0;
    }

    IMPLEMENT_BASIS(NecArgs)
};  // class NecInstruction

class AndInstruction final : public Instruction {
 public:
    struct AndArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct AndArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const AndArgs *>(_args);

        env->memory[GET(index)] = GET(value1) & GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(AndArgs)
};  // class AndInstruction

class OrInstruction final : public Instruction {
 public:
    struct OrArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct OrArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const OrArgs *>(_args);

        env->memory[GET(index)] = GET(value1) | GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(OrArgs)
};  // class OrInstruction

class XorInstruction final : public Instruction {
 public:
    struct XorArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct XorArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const XorArgs *>(_args);

        env->memory[GET(index)] = GET(value1) ^ GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(XorArgs)
};  // class XorInstruction

class FlipInstruction final : public Instruction {
 public:
    struct FlipArgs {
        Value value;
        Value index;
    };  // struct FlipArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const FlipArgs *>(_args);

        env->memory[GET(index)] = ~(GET(value));

        return 0;
    }

    IMPLEMENT_BASIS(FlipArgs)
};  // class FlipInstruction

class NotInstruction final : public Instruction {
 public:
    struct NotArgs {
        Value value;
        Value index;
    };  // struct NotArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const NotArgs *>(_args);

        env->memory[GET(index)] = !GET(value);

        return 0;
    }

    IMPLEMENT_BASIS(NotArgs)
};  // class NotInstruction

class ShlInstruction final : public Instruction {
 public:
    struct ShlArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct ShlArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const ShlArgs *>(_args);

        env->memory[GET(index)] = GET(value1) << GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(ShlArgs)
};  // class ShlInstruction

class ShrInstruction final : public Instruction {
 public:
    struct ShrArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct ShrArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const ShrArgs *>(_args);

        env->memory[GET(index)] = GET(value1) >> GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(ShrArgs)
};  // class ShrInstruction

#define INT_HIGHBIT (sizeof(int) * 8 - 1)

class RolInstruction final : public Instruction {
 public:
    struct RolArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct RolArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const RolArgs *>(_args);

        int v = GET(value1);
        int t = GET(value2) & INT_HIGHBIT;
        for (int i = 0; i < t; i++)
            v = (v << 1) | (v >> INT_HIGHBIT);
        env->memory[GET(index)] = v;

        return 0;
    }

    IMPLEMENT_BASIS(RolArgs)
};  // class RolInstruction

class RorInstruction final : public Instruction {
 public:
    struct RorArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct RorArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const RorArgs *>(_args);

        int v = GET(value1);
        int t = GET(value2) & INT_HIGHBIT;
        for (int i = 0; i < t; i++)
            v = (v >> 1) | (v & (1 << INT_HIGHBIT));
        env->memory[GET(index)] = v;

        return 0;
    }

    IMPLEMENT_BASIS(RorArgs)
};  // class RorInstruction

class EquInstruction final : public Instruction {
 public:
    struct EquArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct EquArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const EquArgs *>(_args);

        env->memory[GET(index)] = GET(value1) == GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(EquArgs)
};  // class EquInstruction

class GterInstruction final : public Instruction {
 public:
    struct GterArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct GterArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const GterArgs *>(_args);

        env->memory[GET(index)] = GET(value1) > GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(GterArgs)
};  // class GterInstruction

class LessInstruction final : public Instruction {
 public:
    struct LessArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct LessArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const LessArgs *>(_args);

        env->memory[GET(index)] = GET(value1) < GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(LessArgs)
};  // class LessInstruction

class GeqInstruction final : public Instruction {
 public:
    struct GeqArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct GeqArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const GeqArgs *>(_args);

        env->memory[GET(index)] = GET(value1) >= GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(GeqArgs)
};  // class GeqInstruction

class LeqInstruction final : public Instruction {
 public:
    struct LeqArgs {
        Value value1;
        Value value2;
        Value index;
    };  // struct LeqArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const LeqArgs *>(_args);

        env->memory[GET(index)] = GET(value1) <= GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(LeqArgs)
};  // class LeqInstruction

class JmpInstruction final : public Instruction {
 public:
    struct JmpArgs {
        Value value;
    };  // struct JmpArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const JmpArgs *>(_args);

        env->current = GET(value);

        return 0;
    }

    IMPLEMENT_BASIS(JmpArgs)
};  // class JmpInstruction

class JmovInstruction final : public Instruction {
 public:
    struct JmovArgs {
        Value value;
    };  // struct JmovArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const JmovArgs *>(_args);

        env->current += GET(value);

        return 0;
    }

    IMPLEMENT_BASIS(JmovArgs)
};  // class JmovInstruction

class JifInstruction final : public Instruction {
 public:
    struct JifArgs {
        Value value1;
        Value value2;
    };  // struct JifArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const JifArgs *>(_args);

        if (GET(value1))
            env->current = GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(JifArgs)
};  // class JifInstruction

class JifmInstruction final : public Instruction {
 public:
    struct JifmArgs {
        Value value1;
        Value value2;
    };  // struct JifmArgs

    virtual size_t execute(const void *_args) {
        auto env = Instruction::env;
        auto args = reinterpret_cast<const JifmArgs *>(_args);

        if (GET(value1))
            env->current += GET(value2);

        return 0;
    }

    IMPLEMENT_BASIS(JifmArgs)
};  // class JifmInstruction

#undef GET
#undef IMPLEMENT_BASIS

////////////
// PARSER //
////////////

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
                beg++;
                break;
            } else
                recur += beg->size;

            beg++;
        }  // while

        target.set(value, recur);
    }

    Command parse_nop(const TokenList &tokens) const {
        if (tokens.back().is_int()) {
            auto instruction = new TaggedNopInstruction;
            auto args = new TaggedNopInstruction::TaggedNopArgs;
            auto beg = std::next(tokens.begin());

            read_value(beg, tokens.end(), args->index);

            return { instruction, args };
        } else {
            auto instruction = new NopInstruction;
            auto args = new NopInstruction::NopArgs;

            return { instruction, args };
        }
    }

    template <typename TInstruction>
    Command parse_i(const TokenList &tokens) const {
        auto instruction = new TInstruction;
        auto args = new typename TInstruction::ArgsType;
        auto beg = std::next(tokens.begin());

        read_value(beg, tokens.end(), args->index);

        return { instruction, args };
    }

    template <typename TInstruction>
    Command parse_v(const TokenList &tokens) const {
        auto instruction = new TInstruction;
        auto args = new typename TInstruction::ArgsType;
        auto beg = std::next(tokens.begin());

        read_value(beg, tokens.end(), args->value);

        return { instruction, args };
    }

    template <typename TInstruction>
    Command parse_vv(const TokenList &tokens) const {
        auto instruction = new TInstruction;
        auto args = new typename TInstruction::ArgsType;
        auto beg = std::next(tokens.begin());

        read_value(beg, tokens.end(), args->value1);
        read_value(beg, tokens.end(), args->value2);

        return { instruction, args };
    }

    template <typename TInstruction>
    Command parse_vi(const TokenList &tokens) const {
        auto instruction = new TInstruction;
        auto args = new typename TInstruction::ArgsType;
        auto beg = std::next(tokens.begin());

        read_value(beg, tokens.end(), args->value);
        read_value(beg, tokens.end(), args->index);

        return { instruction, args };
    }

    template <typename TInstruction>
    Command parse_vvi(const TokenList &tokens) const {
        auto instruction = new TInstruction;
        auto args = new typename TInstruction::ArgsType;
        auto beg = std::next(tokens.begin());

        read_value(beg, tokens.end(), args->value1);
        read_value(beg, tokens.end(), args->value2);
        read_value(beg, tokens.end(), args->index);

        return { instruction, args };
    }

    Command parse(const char *line) const {
        TokenList tokens = _tokenizer.tokenize(line);

        if (tokens.empty() || tokens.front().is_comment())
            return { nullptr, nullptr };

        if (tokens.front().equal_to("NOP"))  // NOP and Tagger NOP
            return parse_nop(tokens);
        else if (tokens.front().equal_to("MEM"))
            return parse_v<MemInstruction>(tokens);
        else if (tokens.front().equal_to("IN"))
            return parse_i<InInstruction>(tokens);
        else if (tokens.front().equal_to("OUT"))
            return parse_v<OutInstruction>(tokens);
        else if (tokens.front().equal_to("SET"))
            return parse_vi<SetInstruction>(tokens);
        else if (tokens.front().equal_to("ADD"))
            return parse_vvi<AddInstruction>(tokens);
        else if (tokens.front().equal_to("SUB"))
            return parse_vvi<SubInstruction>(tokens);
        else if (tokens.front().equal_to("MUL"))
            return parse_vvi<MulInstruction>(tokens);
        else if (tokens.front().equal_to("DIV"))
            return parse_vvi<DivInstruction>(tokens);
        else if (tokens.front().equal_to("MOD"))
            return parse_vvi<ModInstruction>(tokens);
        else if (tokens.front().equal_to("INC"))
            return parse_vi<IncInstruction>(tokens);
        else if (tokens.front().equal_to("DEC"))
            return parse_vi<DecInstruction>(tokens);
        else if (tokens.front().equal_to("NEC"))
            return parse_vi<NecInstruction>(tokens);
        else if (tokens.front().equal_to("AND"))
            return parse_vvi<AndInstruction>(tokens);
        else if (tokens.front().equal_to("OR"))
            return parse_vvi<OrInstruction>(tokens);
        else if (tokens.front().equal_to("XOR"))
            return parse_vvi<XorInstruction>(tokens);
        else if (tokens.front().equal_to("FLIP"))
            return parse_vi<FlipInstruction>(tokens);
        else if (tokens.front().equal_to("NOT"))
            return parse_vi<NotInstruction>(tokens);
        else if (tokens.front().equal_to("SHL"))
            return parse_vvi<ShlInstruction>(tokens);
        else if (tokens.front().equal_to("SHR"))
            return parse_vvi<ShrInstruction>(tokens);
        else if (tokens.front().equal_to("ROL"))
            return parse_vvi<RolInstruction>(tokens);
        else if (tokens.front().equal_to("ROR"))
            return parse_vvi<RorInstruction>(tokens);
        else if (tokens.front().equal_to("EQU"))
            return parse_vvi<EquInstruction>(tokens);
        else if (tokens.front().equal_to("GTER"))
            return parse_vvi<GterInstruction>(tokens);
        else if (tokens.front().equal_to("LESS"))
            return parse_vvi<LessInstruction>(tokens);
        else if (tokens.front().equal_to("GEQ"))
            return parse_vvi<GeqInstruction>(tokens);
        else if (tokens.front().equal_to("LEQ"))
            return parse_vvi<LeqInstruction>(tokens);
        else if (tokens.front().equal_to("JMP"))
            return parse_v<JmpInstruction>(tokens);
        else if (tokens.front().equal_to("JMOV"))
            return parse_v<JmovInstruction>(tokens);
        else if (tokens.front().equal_to("JIF"))
            return parse_vv<JifInstruction>(tokens);
        else if (tokens.front().equal_to("JIFM"))
            return parse_vv<JifmInstruction>(tokens);
        else
            ASSERT(false, "Unknown instruction");
    }

 private:
    Tokenizer _tokenizer;
};  // class Parser

///////////////////
// MAIN FUNCTION //
///////////////////

int main() {
    FILE *in = nullptr;

    in = fopen("test.asm", "r");
    if (!in)
        ASSERT(false, "No ASM file found.");

    Parser parser;
    Program program;
    program.make_environment();

    char buffer[2048];
    while (fgets(buffer, sizeof(buffer), in)) {
        auto command = parser.parse(buffer);

        if (command.is_valid())
            program.append(command);
    }  // while

    program.run();

    return 0;
}  // function main
