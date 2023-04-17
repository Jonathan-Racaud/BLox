#include "compiler.h"
#include "scanner.h"
#include "log.h"

// Private functions forward declarations

void parserAdvance(Parser *parser);
void expression(Parser *parser, Chunk *chunk);
void grouping(Parser *parser, Chunk *chunk);
void unary(Parser *parser, Chunk *chunk);
void binary(Parser *parser, Chunk *chunk);
void consume(Parser *parser, TokenType type, const char * message);

Token scanToken(Parser *parser);

void parsePrecedence(Parser *parser, Chunk *chunk, Precedence precedence);
ParseRule* getRule(TokenType type);

void errorAt(Parser *parser, Token *token, const char *message);
void errorAtCurrent(Parser *parser);

void emitInstruction(Parser *parser, Chunk *chunk, uint32_t instruction);
void emitInstructions(Parser *parser, Chunk *chunk, uint32_t instruction1, uint32_t instruction2);
void emitReturn(Parser *parser, Chunk *chunk);
void endCompiler(Parser *parser, Chunk *chunk);

// Public functions implementations

bool Compile(const char *source, Chunk *chunk) {
    Scanner *scanner = ScannerNew();
    ScannerInit(scanner, source);

    Parser parser;
    ParserInit(&parser);

    parserAdvance(&parser);
    expression(&parser, chunk);
    consume(&parser, TOKEN_EOF, "Expected end of expression");

    ScannerDelete(scanner);

    endCompiler(&parser, chunk);
    return !parser.hadError;
}

void parserAdvance(Parser *parser) {
    parser->previous = parser->current;

    for (;;) {
        parser->current = scanToken(parser);
        if (parser->current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser);
    }
}

void expression(Parser *parser, Chunk *chunk) {
    parsePrecedence(parser, chunk, PREC_ASSIGNMENT);
}

void grouping(Parser *parser, Chunk *chunk) {
    expression(parser, chunk);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

void unary(Parser *parser, Chunk *chunk) {
  TokenType operatorType = parser->previous.type;

  // Compile the operand.
  parsePrecedence(parser, chunk, PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_MINUS: emitInstruction(parser, chunk, OP_NEG); break;
    default: return; // Unreachable.
  }
}

void binary(Parser *parser, Chunk *chunk) {
    TokenType operatorType = parser->previous.type;

    ParseRule *rule = getRule(operatorType);
    parsePrecedence(parser, chunk, (Precedence)(rule->precedence + 1));

    switch (operatorType)
    {
    case TOKEN_PLUS:    emitInstruction(parser, chunk, OP_ADD); break;
    case TOKEN_MINUS:   emitInstruction(parser, chunk, OP_SUB); break;
    case TOKEN_SLASH:   emitInstruction(parser, chunk, OP_DIV); break;
    case TOKEN_STAR:    emitInstruction(parser, chunk, OP_MUL); break;
    case TOKEN_PERCENT: emitInstruction(parser, chunk, OP_MOD); break;
    default: return;
    }
}

void consume(Parser *parser, TokenType type, const char * message) {
    if (parser->current.type == type) {
        parserAdvance(parser);
        return;
    }

    errorAtCurrent(parser);
}

uint32_t makeConstant(Chunk *chunk, Value value) {
    int constant = ChunkAddConstant(chunk, value);
    
    if (constant > UINT32_MAX) {
        LogError("Too many constants in one chunk.");
        return 0;
    }

    return constant;
}

void emitInstruction(Parser *parser, Chunk *chunk, uint32_t instruction) {
    ChunkWriteCode(chunk, instruction, parser->previous.line);
}

void emitInstructions(Parser *parser, Chunk *chunk, uint32_t instruction1, uint32_t instruction2) {
    ChunkWriteCode(chunk, instruction1, parser->previous.line);
    ChunkWriteCode(chunk, instruction2, parser->previous.line);
}

void emitConstant(Parser *parser, Chunk *chunk, Value value) {
    emitInstructions(parser, chunk, OP_CONSTANT, makeConstant(chunk, value));
}

void emitReturn(Parser* parser, Chunk *chunk) {
    ChunkWriteCode(chunk, OP_RETURN, parser->previous.line);
}

void endCompiler(Parser *parser, Chunk *chunk) {
    emitReturn(parser, chunk);
}

void number(Parser *parser, Chunk *chunk) {
    Value value = strtod(parser->previous.start, NULL);
    emitConstant(parser, chunk, value);
}

Token scanToken(Parser *parser) {
    Token token;

    return token;
}

void parsePrecedence(Parser *parser, Chunk *chunk, Precedence precedence) {
    parserAdvance(parser);

    ParseFunc prefixRule = getRule(parser->previous.type)->prefix;

    if (prefixRule == NULL) {
        LogError("Expected expression");
        return;
    }

    prefixRule(parser, chunk);

    while (precedence <= getRule(parser->current.type)->precedence) {
        parserAdvance(parser);
        ParseFunc infixRule = getRule(parser->previous.type)->infix;
        infixRule(parser, chunk);
    }
}

void errorAt(Parser *parser, Token *token, const char *message) {
    if (parser->panicMode) return;

    parser->panicMode = true;
    
    LogError("line %d", token->line);

    if (token->type == TOKEN_EOF) {
        LogError(" at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        LogError(" at '%.*s'", token->length, token->start);
    }

    LogError(": %s\n", message);
    
    parser->hadError = true;
}

void errorAtCurrent(Parser *parser) {
    errorAt(parser, &parser->previous, parser->current.start);
}

// Parser public functions implementations

void ParserInit(Parser *parser) {
    parser->panicMode = false;
    parser->hadError = false;
}

/**
 * @brief Rules for parsing based on the TokenType.
 * 
 * @note
 * TokenType | Prefix ParseFunc | Infix ParseFunc | Precedence
 */
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_GREATER]       = {NULL,     NULL,   PREC_NONE},
  [TOKEN_GREATER_EQUAL] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LESS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LESS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRUCT]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUNC]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

ParseRule* getRule(TokenType type) {
  return &rules[type];
}