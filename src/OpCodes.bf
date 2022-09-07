using System;
namespace ijo;

enum OpCode : uint16
{
    case Constant;
    case String;
    case Symbol;

    case Add;
    case Subtract;
    case Multiply;
    case Divide;
    case Modulo;

    case True;
    case False;
    case Undefined;

    case Equal;
    case Greater;
    case GreaterThan;
    case Less;
    case LessThan;

    case Not;
    case Negate;
    case Opposite;

    case Break;
    case Return;

    public static operator uint16(OpCode code)
    {
        return code.Underlying;
    }

    public static operator OpCode(uint16 code)
    {
        return (OpCode)code;
    }

    public StringView Str
    {
        get
        {
            switch (this)
            {
            case Constant: return "OP_CONST";
            case String: return "OP_STR";
            case Symbol: return "OP_SYM";

            case Add: return "OP_ADD";
            case Subtract: return "OP_SUB";
            case Multiply: return "OP_MUL";
            case Divide: return "OP_DIV";
            case Modulo: return "OP_MOD";

            case True: return "OP_TRUE";
            case False: return "OP_FALSE";
            case Undefined: return "OP_UNDEFINED";

            case Equal: return "OP_EQ";
            case Greater: return "OP_GT";
            case GreaterThan: return "OP_GTEQ";
            case Less: return "OP_LESS";
            case LessThan: return "OP_LESSEQ";

            case Not: return "OP_NOT";
            case Negate: return "OP_NEG";
            case Opposite: return "OP_OPP";

            case Break: return "OP_BREAK";
            case Return: return "OP_RET";
            default: return "OP_UNKNOWN";
            }
        }
    }
}