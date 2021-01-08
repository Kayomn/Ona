#include "ona/engine/module.hpp"

namespace Ona::Engine {
	Async::Async(
		Allocator * allocator,
		float hardwarePriority
	) :
		tasks{allocator},
		threads{allocator, static_cast<uint32_t>(CountThreads() * hardwarePriority)}
	{
		this->taskCondition = AllocateCondition();
		this->taskMutex = AllocateMutex();

		if (this->threads.Length() && this->taskCondition && this->taskMutex) {
			ThreadProperties props = {
				.isCancellable = true,
			};

			String name = {"ona.thread[{}]"};

			this->taskCount.Store(0);

			for (uint32_t i = 0; i < this->threads.Length(); i += 1) {
				this->threads.At(i) = AcquireThread(String::Format(name, {i}), props, [this]() {
					for (;;) {
						this->taskMutex->Lock();

						while (this->tasks.Count() == 0) {
							// While there's no work, just listen for some and wait.
							this->taskCondition->Wait(this->taskMutex);
						}

						// Ok, work acquired - let the next thread in on the action and let this one
						// deal with the task it has acquired.
						Task task = this->tasks.Dequeue();

						this->taskMutex->Unlock();
						task.Invoke();
						this->taskCount.FetchSub(1);
					}
				});
			}
		}
	}

	Async::~Async() {
		for (uint32_t i = 0; i < this->threads.Length(); i += 1) this->threads.At(i).Cancel();

		DestroyMutex(this->taskMutex);
		DestroyCondition(this->taskCondition);
	}

	void Async::Execute(Task const & task) {
		this->taskMutex->Lock();
		this->tasks.Enqueue(task);
		this->taskMutex->Unlock();
		this->taskCount.FetchAdd(1);
		// Signal to the listener that there's work to be done.
		this->taskCondition->Signal();
	}

	void Async::Wait() {
		while (this->taskCount.Load());
	}
}
