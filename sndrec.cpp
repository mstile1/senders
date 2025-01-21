#include "async.h"

#include <functional>
#include <print>
#include <string>

namespace Exec = ODK::Exec;

struct SimpleReceiver
{
	template <typename T>
	void SetValue(T&& Value) {
		std::println("the value is: '{}'", Value);
	}

	template <typename E>
	void SetError(E&& Err) {
		std::println("the error is: '{}'", Err);
	}

	void SetStopped() {
		std::println("the operation was stopped");
	}
};

//void exampleAPI(std::function<void(bool)>)

//---------------------------------------------------------------------------------------

void example1()
{
	std::println("\nexample1: just | connect | start");

	auto snd = Exec::Just(42);

	auto op = Exec::Connect(std::move(snd), SimpleReceiver{});
	op.Start();
}

void example2()
{
	std::println("\nexample2: then | start");

	auto input = Exec::Just(42);

	auto snd = Exec::Then(std::move(input), [](int const Value) {
		std::println("incrementing: '{}'", Value);
		return Value + 1;
	});

	auto op = Exec::Connect(std::move(snd), SimpleReceiver{});
	op.Start();
}

void example3()
{
	std::println("\nexample3: then composed");

	auto const Increment = [](int const Value) {
		std::println("incrementing: '{}'", Value);
		return Value + 1;
	};

	auto op = Exec::Connect(Exec::Just(42) | Exec::Then(Increment),
		SimpleReceiver{});
	op.Start();
}

void example3b()
{
	std::println("\nexample3b: then composed chain");

	auto snd = Exec::Just(std::string{"666666"})
		| Exec::Then([](std::string const s) { return s + std::string{"7777777"}; })
		| Exec::Then([](std::string const s) { return s.length(); })
		| Exec::Then([](auto const n) { return n * 2; })
		| Exec::Then([](auto const n) { return n * 2 - 10; });

	Exec::Start(std::move(snd), SimpleReceiver{});
}

void example4()
{
	std::println("\nexample4: sync_wait");

	if (auto const result = Exec::SyncWait(Exec::Just(42))) {
		SimpleReceiver{}.SetValue(*result);
	} else {
		SimpleReceiver{}.SetError("result missing");
	}
}

void example5()
{
	std::println("\nexample5: sync_wait error");

	auto input = Exec::JustError(std::string{"error"});

	if (auto const result = Exec::SyncWait(std::move(input))) {
		SimpleReceiver{}.SetValue(*result);
	} else {
		SimpleReceiver{}.SetError("result missing");
	}
}

//---------------------------------------------------------------------------------------

int main()
{
	example1();
	example2();
	example3();
	example3b();
	example4();
	example5();

	return 0;
}
