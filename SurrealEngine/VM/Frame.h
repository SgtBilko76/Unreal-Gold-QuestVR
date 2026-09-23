#pragma once

#include "ExpressionValue.h"
#include "Iterator.h"

class DebuggerWindow;
class Bytecode;
class UObject;
class UFunction;
class Expression;
struct ExpressionEvalResult;

enum class FrameRunState
{
	Running,
	DebugBreak,
	StepInto,
	StepOver,
	StepOut
};

enum class LatentRunState
{
	Continue,
	Stop,
	Sleep,
	FinishAnim,
	FinishInterpolation,
	MoveTo,
	MoveToward,
	StrafeTo,
	StrafeFacing,
	TurnTo,
	TurnToward,
	WaitForLanding
};

struct Breakpoint
{
	NameString Class;
	NameString Function;
	NameString State;
	Expression* Expr = nullptr;
	UProperty* Property = nullptr; // For watchpoints. Not implemented yet.
	bool Enabled = true;
};

class LocalVariables
{
public:
	LocalVariables(UStruct* func);
	~LocalVariables();

	UStruct* Func = nullptr;
	void* Data = nullptr;
};

class Frame
{
public:
	static ExpressionValue Call(UFunction* func, UObject* instance, Array<ExpressionValue> args);
	static std::string GetCallstack();
	static std::string GetDisassembly(Expression* statement);

	static bool AddBreakpoint(const NameString& cls, const NameString& func, const NameString& state = {}, int statementIndex = 0);

	static std::function<void()> RunDebugger;
	static Array<Breakpoint> Breakpoints;
	static Array<Frame*> Callstack;
	static FrameRunState RunState;
	static Frame* StepFrame;
	static Expression* StepExpression;
	static std::string ExceptionText;

	static void Break();
	static void Resume();
	static void StepInto();
	static void StepOver();
	static void StepOut();
	static void ThrowException(const std::string& text);

	static std::unique_ptr<Iterator> CreatedIterator;

	Frame(UObject* instance, UStruct* func);

	void SetState(UStruct* func);

	void GotoLabel(const NameString& label);
	void Tick();

	std::string GetName();

	LatentRunState LatentState = LatentRunState::Continue;

	std::unique_ptr<LocalVariables> Variables;
	UObject* Object = nullptr;
	UStruct* Func = nullptr;
	size_t StatementIndex = 0;
	Array<std::unique_ptr<Iterator>> Iterators;

private:
	ExpressionEvalResult Run();
	void ProcessSwitch(const ExpressionValue& condition);

	static ExpressionValue CallNative(UFunction* func, UObject* instance, Array<ExpressionValue> args);
	static ExpressionValue CallScript(UFunction* func, UObject* instance, Array<ExpressionValue> args);
	static void TraceCall(UFunction* func, UObject* instance, const Array<ExpressionValue>& args);

	struct ActiveCallStackFrame
	{
		ActiveCallStackFrame(Frame* frame)
		{
			// Runaway script recursion otherwise ends in a native stack overflow (SIGSEGV with
			// 500+ nested Frame::Run frames, real Quest 3 hardware) that names no script
			// function at all; fail with the script callstack instead so it can be diagnosed.
			if (Frame::Callstack.size() > 300)
			{
				std::string stack = Frame::GetCallstack();
				if (stack.size() > 3000)
					stack = stack.substr(0, 3000) + "\n...";
				Frame::ThrowException("Script recursion limit exceeded (300 nested calls). Callstack:\n" + stack);
			}
			Frame::Callstack.push_back(frame);
		}
		~ActiveCallStackFrame() { Frame::Callstack.pop_back(); }
	};
};
