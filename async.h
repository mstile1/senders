#pragma once

// @todo:
// - how to split receiver between success and error callbacks
// - how to return future result - future built with lazy future?
//   - any other options of types to send back? 'auto' operation state? then not composable...
// - what should sync wait really do?
// - add execution that is actually async
// @todo:

// Operation State
// - represents an object that binds a sender and receiver, typically as members
// - 'connect(sender, receiver)' -> operation state
// - 'start()': invokes the sender meant to fulfill the receiver
// - *may* be an inner type owned by the sender

// Sender
// - represents async computation or operation that can complete by:
//   - producing a value:     set_value(args...)
//   - failing with an error: set_error(err)
//   - canceled/terminated:   set_stopped()

// Receiver
// - represents a sink that receives the result or error of an async operation
// @implements:
//  set_value(x)
//  set_error(err)
//  set_stopped()

#include <optional>
#include <utility>

namespace ODK::Exec
{

// Connect
template <typename S, typename R>
[[nodiscard]] auto Connect(S&& Sender, R&& Receiver)
{
	return std::forward<S>(Sender).Connect(std::forward<R>(Receiver));
}

// Start: connect and auto start
template <typename S, typename R>
[[nodiscard]] auto Start(S&& Sender, R&& Receiver)
{
	auto op = Connect(std::forward<S>(Sender), std::forward<R>(Receiver));
	op.Start();
}

// SyncWait
template <typename T>
struct SyncWaitReceiver
{
	template <typename U>
	void SetValue(U&& Value) {
		Result_ = std::forward<U>(Value);
	}

	template <typename E>
	void SetError(E&&) {
		// unhandled
	}

	void SetStopped() {
		// unhandled
	}

	std::optional<T>& Result_;
};

template <typename S>
[[nodiscard]] auto SyncWait(S&& Sender)
{
	using value_type = decltype(std::forward<S>(Sender)());

	auto ret = std::optional<value_type>{};
	auto op = ODK::Exec::Connect(std::forward<S>(Sender), SyncWaitReceiver<value_type>{ret});
	op.Start();
	return ret;
}

// Just
template <typename T>
class JustSender
{
public:
	explicit JustSender(T&& Value) :
		Value_{std::forward<T>(Value)}
	{
	}

	T operator()()
	{
		return std::move(Value_);
	}

	template <typename R>
	class OperationState
	{
	public:
		explicit OperationState(JustSender&& Sender, R&& Receiver) :
			Sender_{std::move(Sender)},
			Receiver_{std::forward<R>(Receiver)}
		{
		}

		void Start()
		{
			Receiver_.SetValue(Sender_());
		}

	private:
		JustSender Sender_;
		R Receiver_;
	};

	template <typename R>
	[[nodiscard]] OperationState<R> Connect(R&& Receiver)
	{
		return OperationState{std::move(*this), std::forward<R>(Receiver)};
	}

private:
	T Value_;
};

template <typename T>
[[nodiscard]] JustSender<T> Just(T&& Value)
{
	return JustSender{std::forward<T>(Value)};
}

// JustError
template <typename E>
class JustErrorSender
{
public:
	explicit JustErrorSender(E&& Err) :
		Err_{std::forward<E>(Err)}
	{
	}

	E operator()()
	{
		return std::move(Err_);
	}

	template <typename R>
	class OperationState
	{
	public:
		explicit OperationState(JustErrorSender&& Sender, R&& Receiver) :
			Sender_{std::move(Sender)},
			Receiver_{std::forward<R>(Receiver)}
		{
		}

		void Start()
		{
			Receiver_.SetError(Sender_());
		}

	private:
		JustErrorSender Sender_;
		R Receiver_;
	};

	template <typename R>
	[[nodiscard]] OperationState<R> Connect(R&& Receiver)
	{
		return OperationState{std::move(*this), std::forward<R>(Receiver)};
	}

private:
	E Err_;
};

template <typename E>
[[nodiscard]] JustErrorSender<E> JustError(E&& Err)
{
	return JustErrorSender{std::forward<E>(Err)};
}

// Then
template <typename S, typename F>
class ThenSender
{
public:
	explicit ThenSender(S&& Sender, F&& Cont) :
		FromSender_{std::forward<S>(Sender)},
		Cont_{std::forward<F>(Cont)}
	{
	}

	decltype(auto) operator()()
	{
		return std::invoke(Cont_, FromSender_());
	}

	template <typename R>
	class OperationState
	{
	public:
		explicit OperationState(ThenSender&& Sender, R&& Receiver) :
			Sender_{std::move(Sender)},
			Receiver_{std::forward<R>(Receiver)}
		{
		}

		void Start()
		{
			Receiver_.SetValue(Sender_());
		}

	private:
		ThenSender Sender_;
		R Receiver_;
	};

	template <typename R>
	[[nodiscard]] OperationState<R> Connect(R&& Receiver)
	{
		return OperationState{std::move(*this), std::forward<R>(Receiver)};
	}

private:
	S FromSender_;
	F Cont_;
};

template <typename S, typename F>
[[nodiscard]] ThenSender<S, F> Then(S&& Sender, F&& Cont)
{
	return ThenSender{std::forward<S>(Sender), std::forward<F>(Cont)};
}

// Then Pipe
template <typename F>
struct ThenProxy
{
	F Cont_;
};

template <typename F>
ThenProxy(F&& Cont) -> ThenProxy<decltype(Cont)>;

template <typename F>
[[nodiscard]] auto Then(F&& Cont)
{
	return ThenProxy{std::forward<F>(Cont)};
}

template <typename S, typename F>
[[nodiscard]] auto operator|(S&& Sender, ThenProxy<F> Cont)
{
	return ThenSender{std::forward<S>(Sender), std::move(Cont.Cont_)};
}

} // ODK::Exec
