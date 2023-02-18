#pragma once
#include <iostream>
#include <stack>
#include <vector>
#include <cstddef>

class BVM_unimplemented_exception : public std::exception
{
    virtual const char* what() const throw()
    {
        return "Attempted to execute an unimplemented operation or attempted an operation with malformed arguments!";
    }
};

class BVM_stack_exception : std::exception
{
    virtual const char* what() const throw()
    {
        return "Attempted to pop an empty BVM stack!";
    }
};

class VMState
{

public:
    struct StaticVariable
    {
        uint64_t value;
        bool initialized;
        std::wstring name;

        StaticVariable(uint64_t value, bool initialized, const std::wstring& name);
    };

    std::vector<StaticVariable> static_RAS;
    std::vector<uint64_t> relative_RAS;
    size_t relative_RAS_start;
    size_t relative_RAS_idx;
    std::stack<uint64_t> stack;

    VMState(std::vector<std::wstring>& variable_names);

    void printstatics() const;
    std::wstring& printstatics(std::wstring& buf) const;
    void interpret(unsigned char *code_p);

private:
    int read4_sign(unsigned char **data_p, int sizeflags) const;
    uint32_t read4_zero(unsigned char **data_p, int sizeflags) const;
};