#include "parser/Parser.h"
#include "parser/ExpressionParser.h"
#include "modules/ModuleManager.h"


#define m_Stream  m_Parser.m_Stream
#define make_node m_Parser.m_Module->makeNode
#define SET_NODE_ATTRS(x) Parser::NodeAttrHelper GET_UNIQUE_NAME(attr_setter_){x, m_Parser}


ExpressionParser::ExpressionParser(Parser& parser):
    m_Parser(parser) {}


std::string_view ExpressionParser::internString(const std::string_view str) const {
    return m_Parser.m_StringPool.intern(str);
}


Node* ExpressionParser::parseComponent() {
    // this helper returns a packaged element-access operator if it detects its presence
    switch (m_Stream.CurTok.type) {
        case NUMBER: {
            if (m_Stream.CurTok.tokenid == Token::NUM_FLOAT) {
                auto ret = make_node<FloatLit>(internString(m_Stream.CurTok.value));
                SET_NODE_ATTRS(ret);
                m_Parser.forwardStream();
                return ret;
            }

            auto ret = make_node<IntLit>(internString(m_Stream.CurTok.value));
            SET_NODE_ATTRS(ret);
            m_Parser.forwardStream();
            return ret;
        }

        case STRING: {
            std::string content = m_Stream.CurTok.value;

            auto cur_location = m_Stream.CurTok.location;
            cur_location.Col -= m_Stream.CurTok.value.size();

            m_Parser.forwardStream();
            while (m_Stream.CurTok.type == STRING) {  // concatenate adjacent string literals
                content += m_Stream.CurTok.value;
                m_Parser.forwardStream();
            }

            auto str = make_node<StrLit>(internString(content));
            str->location.from = cur_location;
            str->location.to = m_Stream.CurTok.location;
            return str;
        }

        case IDENT: {
            auto id = m_Parser.parseIdent();
            SET_NODE_ATTRS(id);
            assert(!id->full_qualification.empty());

            if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "(") {
                // `id` HAS BEEN MOVED!
                auto call_node = m_Parser.parseCall(id);
                return call_node;
            } return id;
        }

        case KEYWORD: {
            if (m_Stream.CurTok.value == "true" || m_Stream.CurTok.value == "false") {
                auto ret = make_node<BoolLit>(m_Stream.CurTok.value == "true");
                SET_NODE_ATTRS(ret);
                m_Parser.forwardStream();
                return ret;
            }

            if (m_Stream.CurTok.value == "undefined") {
                auto ret = make_node<UndefinedValue>();
                SET_NODE_ATTRS(ret);
                m_Parser.forwardStream();
                return ret;
            }
        }

        case PUNC: {
            if (m_Stream.CurTok.value == "@")
                return m_Parser.parseIntrinsic();

            if (m_Stream.CurTok.value == "[") {
                auto arr_node = make_node<ArrayLit>();
                SET_NODE_ATTRS(arr_node);
                m_Parser.forwardStream();  // skip the '['

                while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "]")) {
                    arr_node->elements.push_back(make_node<Expression>(parseExpr()));

                    if (m_Stream.CurTok.type == PUNC) {
                        if (m_Stream.CurTok.value == ",") {
                            m_Parser.forwardStream();
                            continue;
                        }
                    }
                    if (m_Stream.peek().type != PUNC && m_Stream.peek().value != "]")
                        m_Parser.reportError(ErrCode::COMMA_SEP_REQUIRED);
                } m_Parser.forwardStream();

                return arr_node;
            }

            if (m_Stream.CurTok.value == "(") {
                m_Parser.forwardStream();
                auto ret = make_node<Expression>(parseExpr());
                SET_NODE_ATTRS(ret);
                m_Parser.forwardStream();
                return ret;
            }

            m_Parser.forwardStream();
            m_Parser.reportError(ErrCode::EXPECTED_EXPRESSION);
            return nullptr;
        }

        default: {
            m_Parser.forwardStream();
            m_Parser.reportError(ErrCode::EXPECTED_EXPRESSION);
            return nullptr;
        }
    }
}


Node* ExpressionParser::parsePrefix() {
    // assumption: current token is an OP
    if (m_Stream.CurTok.type == OP) {
        auto op = make_node<Op>(internString(m_Stream.CurTok.value), 1);
        SET_NODE_ATTRS(op);
        m_Parser.forwardStream();

        if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "mut") {
            if (op->op_type != Op::ADDRESS_TAKING) {
                m_Parser.reportError(ErrCode::SYNTAX_ERROR, {
                    .msg = "`mut` can only appear after the `&` operator."
                });
            } else op->is_mutable = true;
            m_Parser.forwardStream();
        }

        auto rhs = parseExpr(Op::getPBPFor(Op::getTagFor(op->value, 1)));
        op->operands.push_back(make_node<Expression>(std::move(rhs)));
        return op;
    }
    return parseComponent();
}


/// This method utilizes `Pratt-Parsing`.
Expression ExpressionParser::parseExpr(const int rbp) {
    // left-denotation, used to parse an operator when there's something to it's left
    auto led = [this](Node* left) -> Node* {
        auto op_str = m_Parser.forwardStream().value;
        auto op = make_node<Op>(internString(op_str), 2);
        SET_NODE_ATTRS(op);

        Expression right;

        switch (op->op_type) {
            case Op::INDEXING_OP:
                right = parseExpr(Op::getRBPFor(Op::INDEXING_OP));
                m_Parser.ignoreButExpect({PUNC, "]"});
                break;
            case Op::CAST_OP: {
                const auto dummy_node = m_Parser.parseType();
                right = Expression::makeExpression(dummy_node);
                break;
            }
            default:
                right = parseExpr(Op::getRBPFor(op->op_type));
        }

        op->operands.push_back(left);
        op->operands.push_back(right.expr);

        return op;
    };

    auto ret = Expression::makeExpression(m_Stream.CurTok.type == OP ? parsePrefix() : parseComponent());

    while (true) {
        // whether to continue with the expression
        if (m_Stream.CurTok.type != OP && !(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "["))
            break;

        if (m_Stream.eof()) {
            m_Parser.reportError(ErrCode::EXPECTED_EXPRESSION);
            break;
        }

        // fetch the left binding power (LBP) and compare it against the right binding power (RBP)
        if (const int lbp = Op::getLBPFor(Op::getTagFor(m_Stream.CurTok.value, 2)); rbp >= lbp)
            break;

        ret = Expression::makeExpression(led(ret.expr));
    }

    return ret;
}


struct SaveStreamState {
    explicit SaveStreamState(TokenStream& stream): stream(stream) {
        stream.setReturnPoint();
    }

    ~SaveStreamState() {
        stream.restoreCache();
    }

private:
    TokenStream& stream;
};
