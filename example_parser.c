#include "arena.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Example: Parser with Arena Allocations
// Demonstrates the classic use case: parsing with temporary and permanent allocations

typedef enum {
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_OPERATOR,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EOF
} TokenType;

typedef struct Token {
    TokenType type;
    char *text;
    int line;
    int column;
} Token;

typedef struct TokenList {
    Token *tokens;
    int count;
    int capacity;
} TokenList;

// Lexer: Tokenize input string
// Uses scratch for temporary work, outputs to result_arena
TokenList lex(const char *input, Arena *result_arena) {
    // Use scratch for temporary allocations during lexing
    Arena *conflicts[] = {result_arena};
    ArenaTemp scratch = scratch_begin(conflicts, 1);

    // Temporary token buffer (we don't know final count yet)
    int capacity = 64;
    Token *temp_tokens = arena_alloc(scratch.arena, sizeof(Token) * capacity);
    int count = 0;

    const char *p = input;
    int line = 1, column = 1;

    while (*p) {
        // Skip whitespace
        while (*p && isspace(*p)) {
            if (*p == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            p++;
        }

        if (!*p) break;

        // Resize if needed
        if (count >= capacity) {
            capacity *= 2;
            Token *new_tokens = arena_alloc(scratch.arena, sizeof(Token) * capacity);
            memcpy(new_tokens, temp_tokens, sizeof(Token) * count);
            temp_tokens = new_tokens;
        }

        Token *tok = &temp_tokens[count++];
        tok->line = line;
        tok->column = column;

        // Number
        if (isdigit(*p)) {
            const char *start = p;
            while (isdigit(*p)) p++;
            size_t len = p - start;

            tok->type = TOKEN_NUMBER;
            tok->text = arena_alloc(result_arena, len + 1);
            memcpy(tok->text, start, len);
            tok->text[len] = '\0';

            column += len;
        }
        // Identifier
        else if (isalpha(*p) || *p == '_') {
            const char *start = p;
            while (isalnum(*p) || *p == '_') p++;
            size_t len = p - start;

            tok->type = TOKEN_IDENTIFIER;
            tok->text = arena_alloc(result_arena, len + 1);
            memcpy(tok->text, start, len);
            tok->text[len] = '\0';

            column += len;
        }
        // Operator
        else if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
            tok->type = TOKEN_OPERATOR;
            tok->text = arena_alloc(result_arena, 2);
            tok->text[0] = *p;
            tok->text[1] = '\0';
            p++;
            column++;
        }
        // Parentheses
        else if (*p == '(') {
            tok->type = TOKEN_LPAREN;
            tok->text = arena_alloc(result_arena, 2);
            strcpy(tok->text, "(");
            p++;
            column++;
        }
        else if (*p == ')') {
            tok->type = TOKEN_RPAREN;
            tok->text = arena_alloc(result_arena, 2);
            strcpy(tok->text, ")");
            p++;
            column++;
        }
        else {
            // Unknown character, skip
            p++;
            column++;
            count--; // Don't count this token
        }
    }

    // EOF token
    if (count >= capacity) {
        capacity++;
        Token *new_tokens = arena_alloc(scratch.arena, sizeof(Token) * capacity);
        memcpy(new_tokens, temp_tokens, sizeof(Token) * count);
        temp_tokens = new_tokens;
    }
    temp_tokens[count].type = TOKEN_EOF;
    temp_tokens[count].text = NULL;
    temp_tokens[count].line = line;
    temp_tokens[count].column = column;
    count++;

    // Copy final tokens to result arena
    TokenList result;
    result.count = count;
    result.capacity = count;
    result.tokens = arena_alloc(result_arena, sizeof(Token) * count);
    memcpy(result.tokens, temp_tokens, sizeof(Token) * count);

    scratch_end(scratch);

    return result;
}

// AST Nodes
typedef enum {
    AST_NUMBER,
    AST_IDENTIFIER,
    AST_BINARY_OP
} ASTNodeType;

typedef struct ASTNode ASTNode;
struct ASTNode {
    ASTNodeType type;
    union {
        int number_value;
        char *identifier;
        struct {
            char op;
            ASTNode *left;
            ASTNode *right;
        } binary_op;
    };
};

// Parser: Build AST from tokens
// Simple expression parser: supports +, -, *, / with precedence
typedef struct {
    Token *tokens;
    int pos;
    int count;
} Parser;

static ASTNode *parse_primary(Parser *parser, Arena *ast_arena);
static ASTNode *parse_expr(Parser *parser, Arena *ast_arena, int min_prec);

static int get_precedence(char op) {
    switch (op) {
        case '+': case '-': return 1;
        case '*': case '/': return 2;
        default: return 0;
    }
}

static ASTNode *parse_primary(Parser *parser, Arena *ast_arena) {
    Token *tok = &parser->tokens[parser->pos];

    if (tok->type == TOKEN_NUMBER) {
        parser->pos++;
        ASTNode *node = arena_alloc(ast_arena, sizeof(ASTNode));
        node->type = AST_NUMBER;
        node->number_value = atoi(tok->text);
        return node;
    }
    else if (tok->type == TOKEN_IDENTIFIER) {
        parser->pos++;
        ASTNode *node = arena_alloc(ast_arena, sizeof(ASTNode));
        node->type = AST_IDENTIFIER;
        node->identifier = tok->text; // Points to token arena
        return node;
    }
    else if (tok->type == TOKEN_LPAREN) {
        parser->pos++;
        ASTNode *expr = parse_expr(parser, ast_arena, 0);
        parser->pos++; // skip )
        return expr;
    }

    return NULL;
}

static ASTNode *parse_expr(Parser *parser, Arena *ast_arena, int min_prec) {
    ASTNode *left = parse_primary(parser, ast_arena);

    while (parser->pos < parser->count) {
        Token *tok = &parser->tokens[parser->pos];
        if (tok->type != TOKEN_OPERATOR) break;

        int prec = get_precedence(tok->text[0]);
        if (prec < min_prec) break;

        char op = tok->text[0];
        parser->pos++;

        ASTNode *right = parse_expr(parser, ast_arena, prec + 1);

        ASTNode *binary = arena_alloc(ast_arena, sizeof(ASTNode));
        binary->type = AST_BINARY_OP;
        binary->binary_op.op = op;
        binary->binary_op.left = left;
        binary->binary_op.right = right;

        left = binary;
    }

    return left;
}

ASTNode *parse(TokenList tokens, Arena *ast_arena) {
    Parser parser = {
        .tokens = tokens.tokens,
        .pos = 0,
        .count = tokens.count
    };

    return parse_expr(&parser, ast_arena, 0);
}

// Evaluator
int eval(ASTNode *node) {
    switch (node->type) {
        case AST_NUMBER:
            return node->number_value;
        case AST_IDENTIFIER:
            return 0; // For simplicity
        case AST_BINARY_OP: {
            int left = eval(node->binary_op.left);
            int right = eval(node->binary_op.right);
            switch (node->binary_op.op) {
                case '+': return left + right;
                case '-': return left - right;
                case '*': return left * right;
                case '/': return right ? left / right : 0;
            }
        }
    }
    return 0;
}

// Pretty printer
void print_ast(ASTNode *node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case AST_NUMBER:
            printf("Number: %d\n", node->number_value);
            break;
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->identifier);
            break;
        case AST_BINARY_OP:
            printf("BinaryOp: %c\n", node->binary_op.op);
            print_ast(node->binary_op.left, indent + 1);
            print_ast(node->binary_op.right, indent + 1);
            break;
    }
}

int main(void) {
    scratch_init(1024 * 1024);

    printf("=== Parser Example: Arena-Based Expression Parser ===\n\n");

    // Test cases
    const char *test_inputs[] = {
        "3 + 4 * 2",
        "(10 - 5) * 3",
        "100 / 2 + 50",
        "a + b * c",
    };

    for (int i = 0; i < 4; i++) {
        printf("Input: %s\n", test_inputs[i]);

        // Create arenas for different lifetime phases
        Arena token_arena = arena_create(4096);  // Lives for lexing + parsing
        Arena ast_arena = arena_create(4096);    // Lives for AST operations

        // Phase 1: Lexing
        TokenList tokens = lex(test_inputs[i], &token_arena);

        printf("Tokens (%d):\n", tokens.count);
        for (int j = 0; j < tokens.count; j++) {
            const char *type_names[] = {"NUM", "ID", "OP", "(", ")", "EOF"};
            printf("  [%s", type_names[tokens.tokens[j].type]);
            if (tokens.tokens[j].text) {
                printf(": %s", tokens.tokens[j].text);
            }
            printf("]\n");
        }

        // Phase 2: Parsing
        ASTNode *ast = parse(tokens, &ast_arena);

        printf("AST:\n");
        print_ast(ast, 1);

        // Phase 3: Evaluation
        int result = eval(ast);
        printf("Result: %d\n", result);

        // Cleanup: token arena no longer needed
        arena_destroy(&token_arena);

        // We could keep AST arena for later use, but destroy for demo
        arena_destroy(&ast_arena);

        printf("\n");
    }

    scratch_cleanup();
    return 0;
}
