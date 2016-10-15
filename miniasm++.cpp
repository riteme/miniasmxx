//
// Copyright 2016 riteme
//

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <climits>

#include <vector>
#include <random>

#define ASSERT(expr, message)            \
    if (!expr) {                         \
        printf("(ERROR) %s\n", message); \
        exit(-1);                        \
    }

#define FRIENDLY_MODE 0

inline int randint() {
    static random_device rd;

    return rd();
}

class MemoryPool {
 public:
    MemoryPool() : _size(size), _mem(nullptr) {}

    MemoryPool(const size_t size) : _size(size), _mem(nullptr) {
        resize(size);
    }

    ~MemoryPool() {
        if (_mem)
            delete _mem;
    }

    int &operator[](const size_t pos) {
        ASSERT(0 <= pos && pos < _size, "Memory index error");

        return _mem[pos];
    }

    void resize(const size_t size) {
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

class Instruction;

class Program {
 public:
    typedef pair<Instruction *, void *> Command;

    Program() : _current(0), _timer(0) {}

    ~Program() {
        for (auto &e : _commands) {
            delete e.first;
            delete e.second;
        }  // foreach in _commands
    }

    bool exited() const {
        return _current >= _commands.size();
    }

    size_t passed_time() const {
        return _timer;
    }

    void append(const Command &command) {
        _commands.push_back(command);
    }

    void run() {
        while (!exited) {
            ASSERT(0 <= _current && _current < _commands.size(),
                   "Mess instructions");

            Command &comm = _commands[_current];
            _current++;

            ASSERT(comm.first != nullptr, "Invalid instruction");
            ASSERT(comm.second != nullptr, "Arguments missing");
            _timer += comm.first->execute(comm.second, this);
        }  // while
    }

    MemoryPool _memory;
    size_t _current;

 private:
    size_t _timer;
    vector<Command> _commands;
};  // class Program

class Value {
 public:
    constexpr size_t MaxReferenceRecursive = 256;

    Value() : _recur(0) {
#if FRIENDLY_MODE
        _value = 0;
#else
        _value = randint();
#endif  // IF FRIENDLY_MODE
    }

    void set(const int value, const size_t recur) {
        _value = value;
        _recur = recur;
    }

    int get(MemoryPool *memory) const {
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

class Instruction {
 public:
    virtual ~Instruction() = default;

    virtual int execute(const void *_args, Program *env) = 0;
};  // class Instruction

struct PrintArgs {
    Value value;
};  // struct PrintArgs

class PrintInstruction : Instruction {
 public:
    virtual int execute(const void *_args, Program *env) {
        auto args = reinterpret_cast<PrintArgs *>(_args);

        printf("%d\n", args->value.get());
    }
};  // class PrintInstruction

class Parser {
 public:
    constexpr size_t MaxIntegerLength = 10;

    int scan_references(const char *line, size_t &pos) const {
        while (!isgraph(line[pos]))
            pos++;

        int cnt = 0;
        while (line[pos] == '*') {
            cnt++;
            pos++;
        }

        return cnt;
    }

    int scan_int(const char *line, size_t &pos) const {
        while (!isgraph(line[pos]))
            pos++;

        size_t start = pos;
        while (isdigit(line[pos]))
            pos++;

        ASSERT(pos - start <= MaxIntegerLength, "Integer too long");

        char tmp = line[pos];
        line[pos] = '\0';
        long long value = atoll(line + start);
        line[pos] = tmp;
    }

    Program::Command parse_print(const char *line) const {
        Program::Command comm;
        comm.first = new PrintInstruction;

        int value;
        size_t current comm.second
    }

    Program::Command parse(const char *line) const {
        size_t left = 0;
        while (!isalpha(line[left]))
            left++;
        for (size_t i = left; isalpha(line[i]); i++)
            line[i] = toupper(line[i]);

        if (strcmp("PRINT", line) == 0)
            return parse_print(line + left + 5);
        else
            ASSERT(false, "Unknown instruction");
    }
};  // class Parser
