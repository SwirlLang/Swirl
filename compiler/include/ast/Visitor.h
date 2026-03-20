#pragma once

struct Node;
struct Expression;
struct IntLit;
struct FloatLit;
struct Op;
struct Var;
struct StrLit;
struct FuncCall;
struct Ident;
struct Function;
struct ReturnStatement;
struct Condition;
struct WhileLoop;
struct Struct;
struct ImportNode;
struct ArrayLit;
struct TypeWrapper;
struct BoolLit;
struct Scope;
struct BreakStmt;
struct ContinueStmt;
struct Intrinsic;


class Visitor {
public:
    virtual void visit(Node*)              = 0;
    virtual void visit(Expression*)        = 0;

    virtual void visit(IntLit*)            = 0;
    virtual void visit(FloatLit*)          = 0;
    virtual void visit(StrLit*)            = 0;
    virtual void visit(BoolLit*)           = 0;
    virtual void visit(ArrayLit*)          = 0;

    virtual void visit(Op*)                = 0;
    virtual void visit(FuncCall*)          = 0;
    virtual void visit(Ident*)             = 0;

    virtual void visit(Var*)               = 0;
    virtual void visit(Function*)          = 0;
    virtual void visit(Struct*)            = 0;
    virtual void visit(TypeWrapper*)       = 0;

    virtual void visit(ReturnStatement*)   = 0;
    virtual void visit(Condition*)         = 0;
    virtual void visit(WhileLoop*)         = 0;
    virtual void visit(BreakStmt*)         = 0;
    virtual void visit(ContinueStmt*)      = 0;

    virtual void visit(Scope*)             = 0;
    virtual void visit(ImportNode*)        = 0;
    virtual void visit(Intrinsic*)         = 0;

    virtual ~Visitor() = default;
};