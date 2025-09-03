#pragma once
#include <string_view>
#include <algorithm>
#include <print>

#include "ErrorManager.h"


/// This class allows the decoupling of the ErrorManager from error-formatting and output details.
/// For custom behavior, one can inherit this class, override the desired methods and pass the pointer
/// of the custom class instance to CompilerInst::setErrorPipeline.
class ErrorPipeline {
public:
    /// Responsible for buffering the raw error message and error context.
    virtual void write(const std::string_view message, const ErrorContext& ctx) {
        std::string backticks;
        backticks.resize(std::format("{} ", ctx.location->Line).size());
        std::fill(backticks.begin(), backticks.end(), ' ');

        backticks.append("|\t");
        for (std::size_t i = 0; i < ctx.location->Col; i++) {
            backticks.append(" ");
        } backticks.append("^");

        m_ErrorBuffer += std::format(
            "At {}:{}:{}\n"
            "\n{} |\t{}"  // line no., line
            "{}\n"        // spaces and the `^`
            "\n\tError: {}\n\n",
            ctx.src_man->getSourcePath().string(),
            ctx.location->Line,
            ctx.location->Col,
            ctx.location->Line,
            ctx.src_man->getLineAt(ctx.location->Line),
            backticks,
            message
        );

        m_ErrorCounter += 1;
    }


    /// Responsible for flushing all the collected errors.
    virtual void flush() {
        std::println(
            stderr,
            "{}\n{} error(s) in total reported.\n",
            m_ErrorBuffer,
            m_ErrorCounter
            );
        m_ErrorBuffer.clear();
    }


    /// Returns whether any error has been reported
    virtual bool errorOccurred() {
        return !m_ErrorBuffer.empty();
    }


    virtual ~ErrorPipeline() = default;


private:
    std::string m_ErrorBuffer;
    uint32_t    m_ErrorCounter = 0;
};
