#include "parser/Parser.h"
#include "parser/ExpressionParser.h"
#include "modules/ModuleManager.h"


#define m_Stream  m_Parser.m_Stream
#define make_node m_Parser.m_Module->makeNode
#define intern_arr m_Parser.m_Module->internArray
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
            while (m_Stream.CurTok.tokenid == Token::STRING) {  // concatenate adjacent string literals
                content += m_Stream.CurTok.value;
                m_Parser.forwardStream();
            }

            auto str = make_node<StrLit>(internString(content));
            str->location.from = cur_location;
            str->location.to = m_Stream.CurTok.location;
            return str;
        }

        case CHAR: {
            char content{};
            if (m_Stream.CurTok.value.empty()) {
                m_Parser.reportError(ErrCode::CHAR_LIT_EMPTY);
            } else if (m_Stream.CurTok.value.size() > 1) {
                m_Parser.reportError(ErrCode::CHAR_LIT_TOO_LONG);
            } else {
                content = m_Stream.CurTok.value[0];
            }

            auto ret = make_node<CharLit>(content);
            SET_NODE_ATTRS(ret);

            m_Parser.forwardStream();
            return ret;
        }

        case IDENT: {
            auto id = m_Parser.parseIdent();
            SET_NODE_ATTRS(id);
            assert(!id->full_qualification.empty());

            if (m_Stream.CurTok.tokenid == Token::PUNC_LPAREN) {
                // `id` HAS BEEN MOVED!
                auto call_node = m_Parser.parseCall(id);
                return call_node;
            } return id;
        }

        case KEYWORD: {
            if (m_Stream.CurTok.is(Token::KW_TRUE, Token::KW_FALSE)) {
                auto ret = make_node<BoolLit>(m_Stream.CurTok.tokenid == Token::KW_TRUE);
                SET_NODE_ATTRS(ret);
                m_Parser.forwardStream();
                return ret;
            }

            if (m_Stream.CurTok.tokenid == Token::KW_UNDEFINED) {
                auto ret = make_node<UndefinedValue>();
                SET_NODE_ATTRS(ret);
                m_Parser.forwardStream();
                return ret;
            }
        }

        case PUNC: {
            if (m_Stream.CurTok.value == "@")
                return m_Parser.parseIntrinsic();

            if (m_Stream.CurTok.tokenid == Token::PUNC_LBRACKET) {
                auto arr_node = make_node<ArrayLit>();
                SET_NODE_ATTRS(arr_node);
                m_Parser.forwardStream();  // skip the '['
                std::vector<Expression*> elements;

                while (m_Stream.CurTok.tokenid != Token::PUNC_RBRACKET) {
                    elements.push_back(make_node<Expression>(parseExpr()));

                    if (m_Stream.CurTok.tokenid == Token::PUNC_COMMA) {
                        m_Parser.forwardStream();
                        continue;
                    }
                    if (m_Stream.peek().tokenid != Token::PUNC_RBRACKET)
                        m_Parser.reportError(ErrCode::COMMA_SEP_REQUIRED);
                } m_Parser.forwardStream();

                arr_node->elements = intern_arr<Expression*>(elements);
                return arr_node;
            }

            if (m_Stream.CurTok.tokenid == Token::PUNC_LPAREN) {
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
        auto op = make_node<Op>(m_Stream.CurTok.tokenid, 1);
        SET_NODE_ATTRS(op);
        m_Parser.forwardStream();

        std::vector<Node*> operands;

        if (m_Stream.CurTok.tokenid == Token::KW_MUT) {
            if (op->op_type != Op::ADDRESS_TAKING) {
                m_Parser.reportError(ErrCode::SYNTAX_ERROR, {
                    .msg = "`mut` can only appear after the `&` operator."
                });
            } else op->is_mutable = true;
            m_Parser.forwardStream();
        }

        auto rhs = parseExpr(Op::getPBPFor(op->op_type));

        operands.push_back(make_node<Expression>(std::move(rhs)));
        op->operands = intern_arr<Node*>(operands);
        return op;
    }
    return parseComponent();
}


/// This method utilizes `Pratt-Parsing`.
Expression ExpressionParser::parseExpr(const int rbp) {
    // left-denotation, used to parse an operator when there's something to it's left
    auto led = [this](Node* left) -> Node* {
        auto op_tok = m_Parser.forwardStream();
        auto op = make_node<Op>(op_tok.tokenid, 2);
        SET_NODE_ATTRS(op);

        Expression right;
        std::vector<Node*> operands;

        switch (op->op_type) {
            case Op::INDEXING_OP:
                right = parseExpr(Op::getRBPFor(Op::INDEXING_OP));
                m_Parser.ignoreButExpect(Token::PUNC_RBRACKET);
                break;
            case Op::CAST_OP: {
                const auto dummy_node = m_Parser.parseType();
                right = Expression::makeExpression(dummy_node);
                break;
            }
            default:
                right = parseExpr(Op::getRBPFor(op->op_type));
        }

        operands.push_back(left);
        operands.push_back(right.expr);
        op->operands = intern_arr<Node*>(operands);
        return op;
    };

    auto ret = Expression::makeExpression(m_Stream.CurTok.type == OP ? parsePrefix() : parseComponent());

    while (true) {
        // whether to continue with the expression
        if (m_Stream.CurTok.type != OP && m_Stream.CurTok.tokenid != Token::PUNC_LBRACKET)
            break;

        if (m_Stream.eof()) {
            m_Parser.reportError(ErrCode::EXPECTED_EXPRESSION);
            break;
        }

        // fetch the left binding power (LBP) and compare it against the right binding power (RBP)
        if (const int lbp = Op::getLBPFor(Op::getTagFor(m_Stream.CurTok.tokenid, 2)); rbp >= lbp)
            break;

        ret = Expression::makeExpression(led(ret.expr));
    }

    return ret;
}