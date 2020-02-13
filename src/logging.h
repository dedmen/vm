#pragma once
#include <ostream>
#include <mutex>
#include <utility>
#include <vector>
#include <string_view>
#include <memory>
#include <array>
#include "type.h"

using namespace std::string_view_literals;

enum class loglevel {
    fatal,
    error,
    warning,
    info,
    verbose,
    trace
};

namespace sqf {
    class instruction;
    namespace parse {
		struct astnode;
		class preprocessorfileinfo;
		class position_info;
    }
}

class LogLocationInfo {
public:
    LogLocationInfo() = default;
    LogLocationInfo(const sqf::parse::preprocessorfileinfo&);
    LogLocationInfo(const sqf::parse::astnode&);
    LogLocationInfo(const sqf::parse::position_info&);
    LogLocationInfo(const sqf::instruction&);

    std::string path;
    size_t line;
    size_t col;

    [[nodiscard]] std::string format() const;
};


class LogMessageBase {
public:
    LogMessageBase(loglevel level, size_t code) : level(level), errorCode(code) {}
    virtual ~LogMessageBase() = default;

    [[nodiscard]] virtual std::string formatMessage() const = 0;
    virtual loglevel getLevel() {
        return level;
    }
    virtual size_t getErrorCode() {
        return errorCode;
    }
    operator LogMessageBase*(){
        return this;
    }
protected:
    loglevel level = loglevel::verbose;
    size_t errorCode;
};
class Logger {
protected:
	std::vector<bool> enabledWarningLevels;
public:
	[[nodiscard]] bool isEnabled(loglevel level) const {
		return enabledWarningLevels[static_cast<size_t>(level)];
	}
	void setEnabled(loglevel level, bool isEnabled) {
		enabledWarningLevels[static_cast<size_t>(level)] = isEnabled;
	}
	virtual void log(loglevel, std::string_view message) = 0;
	static std::string_view loglevelstring(loglevel level)
	{
		switch (level) {
		case loglevel::fatal: return "[FAT]"sv;
		case loglevel::error: return "[ERR]"sv;
		case loglevel::warning: return "[WRN]"sv;
		case loglevel::info: return "[INF]"sv;
		case loglevel::verbose: return "[VBS]"sv;
		case loglevel::trace: return "[TRC]"sv;
		default: return "[???]"sv;
		}
	}
};
class StreamLogger : public Logger {
	std::ostream& logTarget;
	std::mutex streamLock;

public:
	StreamLogger(std::ostream& target);

	void log(loglevel, std::string_view message);
};
class StdOutLogger : public Logger {
public:
	StdOutLogger() {}

	void log(loglevel, std::string_view message);
};

//Classes that can log, inherit from this
class CanLog {
    Logger& m_logger;
protected:
	Logger& get_logger() { return m_logger; }
    void log(LogMessageBase& message) const;
    void log(LogMessageBase&& message) const;
public:
    CanLog(Logger& logger) : m_logger(logger) {}
};


namespace logmessage {

    namespace preprocessor {
        class PreprocBase : public LogMessageBase {
        public:
            PreprocBase(loglevel level, size_t errorCode, LogLocationInfo location) : 
            LogMessageBase(level, errorCode), location(std::move(location)) {}
        protected:
            LogLocationInfo location;
        };

        class ArgCountMissmatch : public PreprocBase {
            static const loglevel level = loglevel::error;
			static const size_t errorCode = 10001;
        public:
            ArgCountMissmatch(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class UnexpectedDataAfterInclude : public PreprocBase {
            static const loglevel level = loglevel::warning;
            static const size_t errorCode = 10002;
        public:
            UnexpectedDataAfterInclude(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class RecursiveInclude : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10003;
            std::string includeTree;
        public:
            RecursiveInclude(LogLocationInfo loc, std::string includeTree) :
                PreprocBase(level, errorCode, std::move(loc)), includeTree(std::move(includeTree)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class IncludeFailed : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10004;
            std::string_view line;
            const std::runtime_error& exception;
        public:
            IncludeFailed(LogLocationInfo loc, std::string_view line, const std::runtime_error& exception) :
                PreprocBase(level, errorCode, std::move(loc)), line(line), exception(exception) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class MacroDefinedTwice : public PreprocBase {
            static const loglevel level = loglevel::warning;
            static const size_t errorCode = 10005;
            std::string_view macroname;
        public:
            MacroDefinedTwice(LogLocationInfo loc, std::string_view macroname) :
                PreprocBase(level, errorCode, std::move(loc)), macroname(macroname) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class MacroNotFound : public PreprocBase {
            static const loglevel level = loglevel::warning;
            static const size_t errorCode = 10006;
            std::string_view macroname;
        public:
            MacroNotFound(LogLocationInfo loc, std::string_view macroname) :
                PreprocBase(level, errorCode, std::move(loc)), macroname(macroname) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class UnexpectedIfdef : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10007;
        public:
            UnexpectedIfdef(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class UnexpectedIfndef : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10008;
        public:
            UnexpectedIfndef(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class UnexpectedElse : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10009;
        public:
            UnexpectedElse(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

		class UnexpectedEndif : public PreprocBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 10010;
		public:
			UnexpectedEndif(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};

		class MissingEndif : public PreprocBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 10011;
		public:
			MissingEndif(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};

        class UnknownInstruction : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10012;
            std::string_view instruction;
        public:
            UnknownInstruction(LogLocationInfo loc, std::string_view instruction) :
                PreprocBase(level, errorCode, std::move(loc)), instruction(instruction) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

    }
	namespace assembly
	{
		class AssemblyBase : public LogMessageBase {
		public:
			AssemblyBase(loglevel level, size_t errorCode, LogLocationInfo location) :
				LogMessageBase(level, errorCode), location(std::move(location)) {}
		protected:
			LogLocationInfo location;
		};

		class ExpectedSemicolon : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20001;
		public:
			ExpectedSemicolon(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativeInstructions : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20002;
		public:
			NoViableAlternativeInstructions(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativeArg : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20003;
		public:
			NoViableAlternativeArg(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedEndStatement : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20004;
		public:
			ExpectedEndStatement(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedCallNular : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20005;
		public:
			ExpectedCallNular(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedNularOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20006;
		public:
			ExpectedNularOperator(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class UnknownNularOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20007;
			std::string_view operator_name;
		public:
			UnknownNularOperator(LogLocationInfo loc, std::string_view operator_name) :
				AssemblyBase(level, errorCode, std::move(loc)), operator_name(operator_name) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedCallUnary : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20008;
		public:
			ExpectedCallUnary(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedUnaryOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20009;
		public:
			ExpectedUnaryOperator(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class UnknownUnaryOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20010;
			std::string_view operator_name;
		public:
			UnknownUnaryOperator(LogLocationInfo loc, std::string_view operator_name) :
				AssemblyBase(level, errorCode, std::move(loc)), operator_name(operator_name) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedCallBinary : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20011;
		public:
			ExpectedCallBinary(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedBinaryOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20012;
		public:
			ExpectedBinaryOperator(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class UnknownBinaryOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20013;
			std::string_view operator_name;
		public:
			UnknownBinaryOperator(LogLocationInfo loc, std::string_view operator_name) :
				AssemblyBase(level, errorCode, std::move(loc)), operator_name(operator_name) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedAssignTo : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20014;
		public:
			ExpectedAssignTo(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedVariableName : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20015;
		public:
			ExpectedVariableName(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedAssignToLocal : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20016;
		public:
			ExpectedAssignToLocal(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedGetVariable : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20017;
		public:
			ExpectedGetVariable(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedMakeArray : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20018;
		public:
			ExpectedMakeArray(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedInteger : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20019;
		public:
			ExpectedInteger(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedPush : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20020;
		public:
			ExpectedPush(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedTypeName : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20021;
		public:
			ExpectedTypeName(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NumberOutOfRange : public AssemblyBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 20022;
		public:
			NumberOutOfRange(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
	}
	namespace sqf
	{
		class SqfBase : public LogMessageBase {
		public:
			SqfBase(loglevel level, size_t errorCode, LogLocationInfo location) :
				LogMessageBase(level, errorCode), location(std::move(location)) {}
		protected:
			LogLocationInfo location;
		};

		class ExpectedStatementTerminator : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30001;
		public:
			ExpectedStatementTerminator(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativeStatement : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30002;
		public:
			NoViableAlternativeStatement(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingUnderscoreOnPrivateVariable : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30003;
			std::string_view m_variable_name;
		public:
			MissingUnderscoreOnPrivateVariable(LogLocationInfo loc, std::string_view variable_name) :
				SqfBase(level, errorCode, std::move(loc)),
			m_variable_name(variable_name) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedBinaryExpression : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30004;
		public:
			ExpectedBinaryExpression(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingRightArgument : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30005;
			std::string_view m_operator_name;
		public:
			MissingRightArgument(LogLocationInfo loc, std::string_view operator_name) :
				SqfBase(level, errorCode, std::move(loc)),
				m_operator_name(operator_name) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingRoundClosingBracket : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30006;
		public:
			MissingRoundClosingBracket(LogLocationInfo loc) :
				SqfBase(level, errorCode, std::move(loc)){ }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingCurlyClosingBracket : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30007;
		public:
			MissingCurlyClosingBracket(LogLocationInfo loc) :
				SqfBase(level, errorCode, std::move(loc)){ }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingSquareClosingBracket : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30008;
		public:
			MissingSquareClosingBracket(LogLocationInfo loc) :
				SqfBase(level, errorCode, std::move(loc)){ }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativePrimaryExpression : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30009;
		public:
			NoViableAlternativePrimaryExpression(LogLocationInfo loc) :
				SqfBase(level, errorCode, std::move(loc)){ }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class EmptyNumber : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30010;
		public:
			EmptyNumber(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedSQF : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30011;
		public:
			ExpectedSQF(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class EndOfFile : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30012;
		public:
			EndOfFile(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
	}
	namespace config
	{
		class ConfigBase : public LogMessageBase {
		public:
			ConfigBase(loglevel level, size_t errorCode, LogLocationInfo location) :
				LogMessageBase(level, errorCode), location(std::move(location)) {}
		protected:
			LogLocationInfo location;
		};
		class ExpectedStatementTerminator : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40001;
		public:
			ExpectedStatementTerminator(LogLocationInfo loc) : ConfigBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativeNode : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40002;
		public:
			NoViableAlternativeNode(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedIdentifier : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40003;
		public:
			ExpectedIdentifier(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingRoundClosingBracket : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40004;
		public:
			MissingRoundClosingBracket(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingCurlyOpeningBracket : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40005;
		public:
			MissingCurlyOpeningBracket(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingCurlyClosingBracket : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40006;
		public:
			MissingCurlyClosingBracket(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingSquareClosingBracket : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40007;
		public:
			MissingSquareClosingBracket(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingEqualSign : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40008;
		public:
			MissingEqualSign(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArray : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40009;
		public:
			ExpectedArray(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedValue : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40010;
		public:
			ExpectedValue(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativeValue : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40011;
		public:
			NoViableAlternativeValue(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class EndOfFileNotReached : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40012;
		public:
			EndOfFileNotReached(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
	}
	namespace linting
	{
		class LintingBase : public LogMessageBase {
		public:
			LintingBase(loglevel level, size_t errorCode, LogLocationInfo location) :
				LogMessageBase(level, errorCode), location(std::move(location)) {}
		protected:
			LogLocationInfo location;
		};
		class UnassignedVariable : public LintingBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 50001;
			std::string_view m_variable_name;
		public:
			UnassignedVariable(LogLocationInfo loc, std::string_view variable_name) :
				LintingBase(level, errorCode, std::move(loc)),
				m_variable_name(variable_name) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
	}
	namespace runtime
	{
		class RuntimeBase : public LogMessageBase {
		public:
			RuntimeBase(loglevel level, size_t errorCode, LogLocationInfo location) :
				LogMessageBase(level, errorCode), location(std::move(location)) {}
		protected:
			LogLocationInfo location;
		};
		class Stacktrace : public RuntimeBase {
			static const loglevel level = loglevel::fatal;
			static const size_t errorCode = 60001;
			std::string m_stacktrace;
		public:
			Stacktrace(LogLocationInfo loc, std::string stacktrace) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_stacktrace(stacktrace)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MaximumInstructionCountReached : public RuntimeBase {
			static const loglevel level = loglevel::fatal;
			static const size_t errorCode = 60002;
			size_t m_maximum_instruction_count;
		public:
			MaximumInstructionCountReached(LogLocationInfo loc, size_t maximum_instruction_count) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_maximum_instruction_count(maximum_instruction_count)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArraySizeMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60003;
			size_t m_expected_min;
			size_t m_expected_max;
			size_t m_got;
		public:
			ExpectedArraySizeMissmatch(LogLocationInfo loc, size_t expected, size_t got) :
				ExpectedArraySizeMissmatch(loc, expected, expected, got)
			{}
			ExpectedArraySizeMissmatch(LogLocationInfo loc, size_t expected_min, size_t expected_max, size_t got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected_min(expected_min),
				m_expected_max(expected_max),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArraySizeMissmatchWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60004;
			size_t m_expected_min;
			size_t m_expected_max;
			size_t m_got;
		public:
			ExpectedArraySizeMissmatchWeak(LogLocationInfo loc, size_t expected, size_t got) :
				ExpectedArraySizeMissmatchWeak(loc, expected, expected, got)
			{}
			ExpectedArraySizeMissmatchWeak(LogLocationInfo loc, size_t expected_min, size_t expected_max, size_t got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected_min(expected_min),
				m_expected_max(expected_max),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedMinimumArraySizeMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60005;
			size_t m_expected;
			size_t m_got;
		public:
			ExpectedMinimumArraySizeMissmatch(LogLocationInfo loc, size_t expected, size_t got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected(expected),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedMinimumArraySizeMissmatchWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60006;
			size_t m_expected;
			size_t m_got;
		public:
			ExpectedMinimumArraySizeMissmatchWeak(LogLocationInfo loc, size_t expected, size_t got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected(expected),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArrayTypeMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60007;
			size_t m_position;
			std::vector<::sqf::type> m_expected;
			::sqf::type m_got;
		public:
			ExpectedArrayTypeMissmatch(LogLocationInfo loc, size_t position, std::vector<::sqf::type> expected, ::sqf::type got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_position(position),
				m_expected(expected),
				m_got(got)
			{}
			ExpectedArrayTypeMissmatch(LogLocationInfo loc, size_t position, ::sqf::type expected, ::sqf::type got) :
				ExpectedArrayTypeMissmatch(loc, position, std::array<::sqf::type, 1> { expected }, got)
			{}
			template<size_t size>
			ExpectedArrayTypeMissmatch(LogLocationInfo loc, size_t position, std::array<::sqf::type, size> expected, ::sqf::type got) :
				ExpectedArrayTypeMissmatch(loc, position, std::vector(expected.begin(), expected.end), got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArrayTypeMissmatchWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60008;
			size_t m_position;
			std::vector<::sqf::type> m_expected;
			::sqf::type m_got;
		public:
			ExpectedArrayTypeMissmatchWeak(LogLocationInfo loc, size_t position, std::vector<::sqf::type> expected, ::sqf::type got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_position(position),
				m_expected(expected),
				m_got(got)
			{}
			ExpectedArrayTypeMissmatchWeak(LogLocationInfo loc, size_t position, ::sqf::type expected, ::sqf::type got) :
				ExpectedArrayTypeMissmatchWeak(loc, position, std::array<::sqf::type, 1> { expected }, got)
			{}
			template<size_t size>
			ExpectedArrayTypeMissmatchWeak(LogLocationInfo loc, size_t position, std::array<::sqf::type, size> expected, ::sqf::type got) :
				ExpectedArrayTypeMissmatchWeak(loc, position, std::vector(expected.begin(), expected.end), got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class IndexOutOfRange : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60009;
			size_t m_range;
			size_t m_index;
		public:
			IndexOutOfRange(LogLocationInfo loc, size_t range, size_t index) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_range(range),
				m_index(index)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class IndexOutOfRangeWeak : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60010;
			size_t m_range;
			size_t m_index;
		public:
			IndexOutOfRangeWeak(LogLocationInfo loc, size_t range, size_t index) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_range(range),
				m_index(index)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NegativeIndex : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60011;
		public:
			NegativeIndex(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NegativeIndexWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60012;
		public:
			NegativeIndexWeak(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class IndexEqualsRange : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60013;
			size_t m_range;
			size_t m_index;
		public:
			IndexEqualsRange(LogLocationInfo loc, size_t range, size_t index) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_range(range),
				m_index(index)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningNil : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60014;
		public:
			ReturningNil(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningEmptyArray : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60015;
		public:
			ReturningEmptyArray(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NegativeSize : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60016;
		public:
			NegativeSize(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NegativeSizeWeak: public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60017;
		public:
			NegativeSizeWeak(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ArrayRecursion: public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60018;
		public:
			ArrayRecursion(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class InfoMessage: public RuntimeBase {
			static const loglevel level = loglevel::info;
			static const size_t errorCode = 60019;
			std::string_view m_source;
			std::string m_message;
		public:
			InfoMessage(LogLocationInfo loc, std::string_view source, std::string message) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_source(source),
				m_message(message)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class SuspensionDisabled : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60020;
		public:
			SuspensionDisabled(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class SuspensionInUnscheduledEnvironment : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60021;
		public:
			SuspensionInUnscheduledEnvironment(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
	}
}

