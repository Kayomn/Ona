#include "engine/exports.hpp"

namespace Ona::Engine {
	Async::Async(
		Allocator * allocator,
		float hardwarePriority
	) :
		tasks{allocator},

		threads{
			allocator,
			static_cast<uint32_t>(Clamp(CountHardwareConcurrency() * hardwarePriority, 0.f, 1.f))
		}
	{
		if (
			this->threads.Length() &&
			CreateCondition(this->taskCondition).IsOk() &&
			CreateMutex(this->taskMutex).IsOk()
		) {
			static ThreadProperties const threadProperties = {
				.isCancellable = true,
			};

			String threadName = {"ona.thread[{}]"};

			this->taskCount.Store(0);

			for (uint32_t i = 0; i < this->threads.Length(); i += 1) {
				AcquireThread(String::Format(threadName, {i}), threadProperties, [this]() {
					for (;;) {
						this->taskMutex.Lock();

						while (this->tasks.Count() == 0) {
							// While there's no work, just listen for some and wait.
							this->taskCondition.Wait(this->taskMutex);
						}

						// Ok, work acquired - let the next thread in on the action and let this one
						// deal with the task it has acquired.
						Task task = this->tasks.Dequeue();

						this->taskMutex.Unlock();
						task.Invoke();
						this->taskCount.FetchSub(1);
					}
				}, this->threads.At(i));
			}
		}
	}

	Async::~Async() {
		for (uint32_t i = 0; i < this->threads.Length(); i += 1) this->threads.At(i).Cancel();

		this->taskMutex.Free();
		this->taskCondition.Free();
	}

	void Async::Execute(Task const & task) {
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
