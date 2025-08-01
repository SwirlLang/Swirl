#include <parser/Parser.h>
#include <parser/ExpressionParser.h>

#define m_Stream m_Parser.m_Stream
using SwNode = std::unique_ptr<Node>;

ExpressionParser::ExpressionParser(Parser& parser): m_Parser(parser) {}


std::unique_ptr<Node> ExpressionParser::parseComponent() {
    // this helper returns a packaged element-access operator if it detects its presence
    switch (m_Stream.CurTok.type) {
        case NUMBER: {
            if (m_Stream.CurTok.meta == CT_FLOAT) {
                auto ret = std::make_unique<FloatLit>(m_Stream.CurTok.value);
                m_Parser.forwardStream();
                return std::move(ret);
            }

            auto ret = std::make_unique<IntLit>(m_Stream.CurTok.value);

            m_Parser.forwardStream();
            return std::move(ret);
        }
        case STRING: {
            auto str = std::make_unique<StrLit>(m_Stream.CurTok.value);
            while (m_Stream.CurTok.type == STRING) {  // concatenation of adjacent string literals
                str->value += m_Stream.CurTok.value;
                m_Parser.forwardStream();
            } return std::move(str);
        }
        case IDENT: {
            auto id = m_Parser.parseIdent();
            if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "(") {
                auto call_node = m_Parser.parseCall(std::move(id));
                return std::move(call_node);
            }

            auto id_node = std::make_unique<Ident>(std::move(id));
            return std::move(id_node);
        }
        case PUNC: {
            if (m_Stream.CurTok.value == "[") {
                ArrayNode arr;
                arr.location = m_Stream.getStreamState();
                m_Parser.forwardStream();  // skip the '['

                while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "]")) {
                    arr.elements.push_back(parseExpr());

                    if (m_Stream.CurTok.type == PUNC) {
                        if (m_Stream.CurTok.value == ",") {
                            m_Parser.forwardStream();
                            continue;
                        }
                    }
                    if (m_Stream.peek().type != PUNC && m_Stream.peek().value != "]")
                        m_Parser.reportError(ErrCode::COMMA_SEP_REQUIRED);
                } m_Parser.forwardStream();

                auto arr_node = std::make_unique<ArrayNode>(std::move(arr));
                return std::move(arr_node);
            }

            if (m_Stream.CurTok.value == "(") {
                m_Parser.forwardStream();
                auto ret = std::make_unique<Expression>(parseExpr());
                ret->location = m_Stream.getStreamState();
                m_Parser.forwardStream();
                return std::move(ret);
            } return m_Parser.dispatch();
        }

        default: {
            m_Parser.reportError(ErrCode::EXPECTED_EXPRESSION);
            return nullptr;
        }
    }
}


std::unique_ptr<Node> ExpressionParser::parsePrefix() {
    // assumption: current token is an OP
    if (m_Stream.CurTok.type == OP) {
        auto op = std::make_unique<Op>(m_Stream.CurTok.value, 1);
        op->location = m_Stream.getStreamState();
        m_Parser.forwardStream();

        auto rhs = parseExpr(Op::getPBPFor(Op::getTagFor(op->value, 1)));
        op->operands.push_back(std::make_unique<Expression>(std::move(rhs)));
        return std::move(op);
    }
    return parseComponent();
}


/// This method utilizes `Pratt-Parsing`.
Expression ExpressionParser::parseExpr(const int rbp) {
    // left-denotation, used to parse an operator when there's something to it's left
    auto led = [this](SwNode left) -> SwNode {
        auto op_str = m_Parser.forwardStream().value;
        const auto tag = Op::getTagFor(op_str, 2);
        auto right = parseExpr(Op::getLBPFor(tag));

        if (op_str == "[") {
            m_Parser.ignoreButExpect({PUNC, "]"});
        }

        auto op = std::make_unique<Op>(op_str, 2);

        op->operands.push_back(std::move(left));
        op->operands.push_back(std::move(right.expr.front()));
        op->location = m_Stream.getStreamState();

        return std::move(op);
    };

    auto ret = Expression::makeExpression(m_Stream.CurTok.type == OP ? parsePrefix() : parseComponent());
    ret.location = m_Stream.getStreamState();

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

        ret = Expression::makeExpression(led(std::move(ret.expr.front())));
    }

    ret.location = m_Stream.getStreamState();
    return std::move(ret);
}
