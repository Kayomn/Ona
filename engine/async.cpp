#include "engine.hpp"

namespace Ona {
	Async::Async(
		Allocator * allocator,
		float hardwarePriority
	) :
		tasks{allocator},
		taskCount{0},
		isRunning{1},

		threads{
			allocator,
			static_cast<uint32_t>(Clamp(CountHardwareConcurrency() * hardwarePriority, 0.f, 1.f))
		}
	{
		if (
			this->threads.Length() &&
			IsOk(CreateCondition(this->taskCondition)) &&
			IsOk(CreateMutex(this->taskMutex))
		) {
			String threadName = {"ona.thread."};

			for (uint32_t i = 0; i < this->threads.Length(); i += 1) {
				AcquireThread(String::Concat({threadName, DecStringUnsigned(i)}), [this]() {
					for (;;) {
						this->taskMutex.Lock();

						while (this->tasks.Count() == 0) {
							if (this->isRunning.Load() == 0) {
								// Time to die.
								this->taskMutex.Unlock();

								return;
							}

							// While there's no work, just listen for some and wait.
							this->taskCondition.Wait(this->taskMutex);
						}

						// Ok, work acquired - let the next thread in on the action and let this one
						// deal with the task it has acquired.
						Callable<void()> task = this->tasks.Dequeue();

						this->taskMutex.Unlock();
						this->taskCount.FetchSub(1);
						task.Invoke();
					}
				}, this->threads.At(i));
			}
		}
	}

	Async::~Async() {
		this->isRunning.Store(0);
		this->taskCondition.Signal();

		for (Thread & thread : this->threads.Values()) thread.Join();

		this->taskMutex.Free();
		this->taskCondition.Free();
	}

	void Async::Execute(Callable<void()> const & task) {
		this->taskMutex.Lock();
		this->tasks.Enqueue(task);
		this->taskMutex.Unlock();
		this->taskCount.FetchAdd(1);
		// Signal to the listener that there's work to be done.
		this->taskCondition.Signal();
	}

	void Async::Wait() {
		while (this->taskCount.Load());
	}
}
