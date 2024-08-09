#include "stdafx.h"

#include <iostream>
#include <stack>
#include <vector>
#include <cstddef>
#include <memory>
#include <string>
#include "VMState.h"

VMState::StaticVariable::StaticVariable(uint64_t value, bool initialized, const std::wstring& name)
{
    this->value = value;
    this->initialized = initialized;
    this->name = std::wstring(name);
}

VMState::VMState(std::vector<std::wstring>& variable_names)
{
    for (int i = 0; i < variable_names.size(); ++i)
    {
        static_RAS.push_back(StaticVariable(0xCDCDCDCDCDCDCDCD, false, variable_names[i]));
    }
    relative_RAS.reserve(1024);
    relative_RAS_start = variable_names.size();
    relative_RAS_idx = relative_RAS_start;
}

void VMState::printstatics() const
{
    for (int i = 0; i < static_RAS.size(); ++i)
    {
        printf("%ls", static_RAS[i].name.c_str());
        if (static_RAS[i].initialized)
        {
            printf(" = %llu;\n", static_RAS[i].value);
        }
        else
        {
            printf(";\n");
        }
    }
}

std::wstring& VMState::printstatics(std::wstring& buf) const
{
    for (int i = 0; i < static_RAS.size(); ++i)
    {
        buf += static_RAS[i].name;
        if (static_RAS[i].initialized)
        {
            buf += L" = ";
            buf += std::to_wstring((int32_t)static_RAS[i].value);
        }
        buf += L";\n";
    }

    buf += L"\n";
    return buf;
}

void VMState::interpret(unsigned char *code_p)
{
    while (true)
    {
        int sizeflags = *code_p & 0xC0;
        int op = *code_p & 0x3F;
        ++code_p;

        switch (op)
        {
            case 0x00: // No Operation
                break;
            case 0x01: // Convert type of operand and store on RAS
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int32_t value = stack.top(); stack.pop();
                int32_t RAS_idx = stack.top(); stack.pop();
                stack.push(value);
                int typeflags = *code_p;
                ++code_p;

                switch (typeflags)
                {
                    case 0b00: // int to int
                    case 0b11: // float to float
                        if (RAS_idx >= relative_RAS_start)
                            relative_RAS[RAS_idx] = value;
                        else
                        {
                            static_RAS[RAS_idx].value = value;
                            static_RAS[RAS_idx].initialized = true;
                        }
                        break;
                    case 0b01: // float to int
                    {
                        float f;
                        memcpy(&f, &value, sizeof(float));
                        int i = f;
                        if (RAS_idx >= relative_RAS_start)
                            relative_RAS[RAS_idx] = i;
                        else
                        {
                            static_RAS[RAS_idx].value = i;
                            static_RAS[RAS_idx].initialized = true;
                        }
                        break;
                    }
                    case 0b10: // int to float
                    {
                        float f = value;
                        int i;
                        memcpy(&i, &f, sizeof(float));
                        if (RAS_idx >= relative_RAS_start)
                            relative_RAS[RAS_idx] = i;
                        else
                        {
                            static_RAS[RAS_idx].value = i;
                            static_RAS[RAS_idx].initialized = true;
                        }
                        break;
                    }
                    default:
                        throw BVM_unimplemented_exception();
                }
                break;
            }
            case 0x02: // Pop element off stack
                stack.pop();
                break;
            case 0x03: // Copy element to stack
            {
                if (!stack.size()) throw BVM_stack_exception();
                int value = stack.top();
                stack.push(value);
                break;
            }
            case 0x04: // Add two elements
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int lhs = stack.top(); stack.pop();
                int rhs = stack.top(); stack.pop();
                int result;
                float lhs_f;
                float rhs_f;
                float result_f;
                int typeflags = *code_p;
                ++code_p;

                switch (typeflags)
                {
                    case 0b00: // int + int
                        result = lhs + rhs;
                        break;
                    case 0b01: // int + float
                        memcpy(&rhs_f, &rhs, sizeof(float));
                        result_f = lhs + rhs_f;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    case 0b10: // float + int
                        memcpy(&lhs_f, &lhs, sizeof(float));
                        result_f = lhs_f + rhs;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    case 0b11: // float + float
                        memcpy(&lhs_f, &lhs, sizeof(float));
                        memcpy(&rhs_f, &rhs, sizeof(float));
                        result_f = lhs_f + rhs_f;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    default:
                        throw BVM_unimplemented_exception();
                }
                stack.push(result);
                break;
            }
            case 0x05: // Subtract two elements
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int lhs = stack.top(); stack.pop();
                int rhs = stack.top(); stack.pop();
                int result;
                float lhs_f;
                float rhs_f;
                float result_f;
                int typeflags = *code_p;
                ++code_p;

                switch (typeflags)
                {
                    case 0b00: // int - int
                        result = lhs - rhs;
                        break;
                    case 0b01: // int - float
                        memcpy(&rhs_f, &rhs, sizeof(float));
                        result_f = lhs - rhs_f;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    case 0b10: // float - int
                        memcpy(&lhs_f, &lhs, sizeof(float));
                        result_f = lhs_f + rhs;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    case 0b11: // float - float
                        memcpy(&lhs_f, &lhs, sizeof(float));
                        memcpy(&rhs_f, &rhs, sizeof(float));
                        result_f = lhs_f + rhs_f;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    default:
                        throw BVM_unimplemented_exception();
                }
                stack.push(result);
                break;
            }
            case 0x06: // Multiply two elements
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int lhs = stack.top(); stack.pop();
                int rhs = stack.top(); stack.pop();
                int result;
                float lhs_f;
                float rhs_f;
                float result_f;
                int typeflags = *code_p;
                ++code_p;

                switch (typeflags)
                {
                    case 0b00: // int * int
                        result = lhs * rhs;
                        break;
                    case 0b01: // int * float
                        memcpy(&rhs_f, &rhs, sizeof(float));
                        result_f = lhs * rhs_f;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    case 0b10: // float * int
                        memcpy(&lhs_f, &lhs, sizeof(float));
                        result_f = lhs_f * rhs;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    case 0b11: // float * float
                        memcpy(&lhs_f, &lhs, sizeof(float));
                        memcpy(&rhs_f, &rhs, sizeof(float));
                        result_f = lhs_f * rhs_f;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    default:
                        throw BVM_unimplemented_exception();
                }
                stack.push(result);
                break;
            }
            case 0x07: // Divide two elements
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int lhs = stack.top(); stack.pop();
                int rhs = stack.top(); stack.pop();
                int result;
                float lhs_f;
                float rhs_f;
                float result_f;
                int typeflags = *code_p;
                ++code_p;

                switch (typeflags)
                {
                    case 0b00: // int / int
                        result = lhs / rhs;
                        break;
                    case 0b01: // int / float
                        memcpy(&rhs_f, &rhs, sizeof(float));
                        result_f = lhs / rhs_f;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    case 0b10: // float / int
                        memcpy(&lhs_f, &lhs, sizeof(float));
                        result_f = lhs_f / rhs;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    case 0b11: // float / float
                        memcpy(&lhs_f, &lhs, sizeof(float));
                        memcpy(&rhs_f, &rhs, sizeof(float));
                        result_f = lhs_f / rhs_f;
                        memcpy(&result, &result_f, sizeof(float));
                        break;
                    default:
                        throw BVM_unimplemented_exception();
                }
                stack.push(result);
                break;
            }
            case 0x08: // Modulo two elements
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int lhs = stack.top(); stack.pop();
                int rhs = stack.top(); stack.pop();
                stack.push(lhs % rhs);
                break;
            }
            case 0x09: // Bitwise Negation of top stack element
            {
                if (!stack.size()) throw BVM_stack_exception();
                int typeflags = *code_p;
                ++code_p;

                switch (typeflags)
                {
                    case 0x00: // int
                    {
                        int i = stack.top();
                        stack.pop();
                        stack.push(-i);
                        break;
                    }
                    case 0x01: // float
                    {
                        float f;
                        int value = stack.top();
                        memcpy(&f, &value, sizeof(float));
                        f = -f;
                        memcpy(&value, &f, sizeof(int));
                        stack.pop();
                        stack.push(value);
                        break;
                    }
                    default:
                        throw BVM_unimplemented_exception();
                }
                break;
            }
            case 0x0A: // Increment element
            {
                if (!stack.size()) throw BVM_stack_exception();
                int typeflags = *code_p;
                ++code_p;

                if (typeflags | 2) // float
                {
                    int value = stack.top(); stack.pop();
                    float f;
                    memcpy(&f, &value, sizeof(float));
                    ++f;
                    memcpy(&value, &f, sizeof(float));
                    stack.push(value);
                }
                else // int
                {
                    int value = stack.top(); stack.pop();
                    stack.push(++value);
                }
                break;
            }
            case 0x0B: // Decrement element
            {
                if (!stack.size()) throw BVM_stack_exception();
                int typeflags = *code_p;
                ++code_p;

                if (typeflags | 2) // float
                {
                    int value = stack.top(); stack.pop();
                    float f;
                    memcpy(&f, &value, sizeof(float));
                    --f;
                    memcpy(&value, &f, sizeof(float));
                    stack.push(value);
                }
                else // int
                {
                    int value = stack.top(); stack.pop();
                    stack.push(--value);
                }
                break;
            }
            case 0x0C: // [stack-1] >> [stack]
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int rhs = stack.top(); stack.pop();
                int lhs = stack.top(); stack.pop();
                stack.push(lhs >> rhs);
                break;
            }
            case 0x0D: // [stack-1] << [stack]
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int rhs = stack.top(); stack.pop();
                int lhs = stack.top(); stack.pop();
                stack.push(lhs << rhs);
                break;
            }
            case 0x0E: // [stack-1] & [stack]
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int rhs = stack.top(); stack.pop();
                int lhs = stack.top(); stack.pop();
                stack.push(lhs & rhs);
                break;
            }
            case 0x0F: // [stack-1] | [stack]
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int rhs = stack.top(); stack.pop();
                int lhs = stack.top(); stack.pop();
                stack.push(lhs | rhs);
                break;
            }
            case 0x10: // [stack-1] ^ [stack]
            {
                if (stack.size() < 2) throw BVM_stack_exception();
                int rhs = stack.top(); stack.pop();
                int lhs = stack.top(); stack.pop();
                stack.push(lhs ^ rhs);
                break;
            }
            case 0x11: // [stack] = ~[stack]
            {
                if (!stack.size()) throw BVM_stack_exception();
                int value = stack.top(); stack.pop();
                stack.push(~value);
                break;
            }
            case 0x12: // Convert float to int
            {
                float f;
                int value = stack.top(); stack.pop();
                memcpy(&f, &value, sizeof(float));
                stack.push(f);
                break;
            }
            case 0x13: // Convert int to float
            {
                int value = stack.top(); stack.pop();
                float f = value;
                memcpy(&value, &f, sizeof(float));
                stack.push(value);
                break;
            }
            case 0x14: //Load element from RAS
            {
                int idx = read4_zero(&code_p, sizeflags);
                int value;

                if (idx < relative_RAS_start)
                {
                    value = static_RAS[idx].value;
                }
                else
                {
                    value = relative_RAS[idx];
                }
                stack.push(value);
                break;
            }
            case 0x15: // Read number into stack
            {
                stack.push(read4_sign(&code_p, sizeflags));
                break;
            }
            case 0x16: // Store stack element into RAS
            {
                if (!stack.size()) throw BVM_stack_exception();

                int idx = read4_zero(&code_p, sizeflags);
                int value = stack.top();
                stack.pop();

                if (idx < relative_RAS_start)
                {
                    static_RAS[idx].initialized = true;
                    static_RAS[idx].value = value;
                }
                else
                {
                    relative_RAS[idx] = value;
                }
                break;
            }
            case 0x30: // Stop execution
                return;
            default:
                throw BVM_unimplemented_exception();
        }
    }
}

int VMState::read4_sign(unsigned char **data_p, int sizeflags) const
{
    int result;
    switch (sizeflags)
    {
        case 0x00:
            return 0;
        case 0x40:
            memcpy(&result, *data_p, 1);
            result <<= 24;
            result >>= 24;
            *data_p += 1;
            return result;
        case 0x80:
            memcpy(&result, *data_p, 2);
            result <<= 16;
            result >>= 16;
            *data_p += 2;
            return result;
        case 0xC0:
            memcpy(&result, *data_p, 4);
            *data_p += 4;
            return result;
        default:
            return 0;
    }
}

uint32_t VMState::read4_zero(unsigned char **data_p, int sizeflags) const
{
    uint32_t result = 0;
    switch (sizeflags)
    {
        case 0x00:
            return 0;
        case 0x40:
            memcpy(&result, *data_p, 1);
            *data_p += 1;
            return result;
        case 0x80:
            memcpy(&result, *data_p, 2);
            *data_p += 2;
            return result;
        case 0xC0:
            memcpy(&result, *data_p, 4);
            *data_p += 4;
            return result;
        default:
            return 0;
    }
}