#pragma once
#include <string>
#include <optional>

enum TokenType
{
    //keywords
    RETURN,
    LET,
    FN,
    EXTERN,

    // type keyword
    BuiltinType,

    //literals
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,


    //symbols
    LParan,
    RParan,
    LCURLY,
    RCURLY,
    SEMICOLON,
    COLON,
    ARROW,
    EQUALS,
    EQUALITY,
    PLUS,
    MINUS,
    FORWARD_SLASH,
    STAR,
    MODULUS,
    ELLIPSIS,
    COMMA,

    //MISC.
    IDENT,
    _EOF
};

enum PrimitiveDataType
{
    EMPTY,
    VOID,

    // Unsigned Int
    u8,
    u16,
    u32,
    u64,

    // Pointer Types
    u8ptr,
    u16ptr,
    u32ptr,
    u64ptr,

    //Signed Int
    i8,
    i16,
    i32,
    i64,

    // Pointer Type
    i8ptr,
    i16ptr,
    i32ptr,
    i64ptr,

    // Float
    f32,
    f64,

    f32ptr,
    f64ptr,

    // Mega Types
    u128,
    i128,

    str
};

struct Token
{
	TokenType type;
	std::string value;
    PrimitiveDataType dataType; /*Only used when token is a BuiltIn dataType*/
    size_t lineNum;

    Token(TokenType _type, std::string& _val, size_t line, PrimitiveDataType dt = PrimitiveDataType::EMPTY)
        : type(_type), lineNum(line), dataType(dt)
    {
        if (_val != "")
            value = std::move(_val);
    }
};