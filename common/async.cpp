#include "common.hpp"

#include "thirdparty/marl/include/marl/scheduler.h"
#include "thirdparty/marl/include/marl/waitgroup.h"
#include "thirdparty/marl/include/marl/event.h"

#include <unistd.h>

namespace Ona {
	static marl::Scheduler scheduler = {marl::Scheduler::Config{}.setWorkerThreadCount(2)};

	static marl::WaitGroup waitGroup = {0};

	class Channel final : public Object {
		public:
		marl::Event sendEvent;

		marl::Event receiveEvent;

		uint32_t typeSize;

		AtomicU32 storedBytes;

		Channel(uint32_t typeSize) :
			typeSize{typeSize},
			storedBytes{0},
			sendEvent{marl::Event::Mode::Auto},
			receiveEvent{marl::Event::Mode::Auto} {

		}
	};

	void CloseChannel(Channel * & channel) {
		Deallocate(Allocator::Default, channel);

		channel = nullptr;
	}

	uint32_t ChannelReceive(Channel * channel, Slice<uint8_t> outputBuffer) {
		size_t const typeSize = channel->typeSize;

		if (channel->storedBytes.Load() == 0) {
			// Channel is currently empty. Wait until data is sent.
			channel->receiveEvent.wait();
		}

		uint32_t const readBytes = static_cast<uint32_t>(CopyMemory(outputBuffer, Slice<uint8_t>{
			.length = typeSize,
			.pointer = reinterpret_cast<uint8_t *>(channel + typeSize),
		}));

		channel->storedBytes.Store(0);
		channel->sendEvent.signal();

		return readBytes;
	}

	uint32_t ChannelSend(Channel * channel, Slice<uint8_t const> const & inputBuffer) {
		size_t const typeSize = channel->typeSize;

		if (channel->storedBytes.Load()) {
			// Channel is currently full. Wait until data is received.
			channel->sendEvent.wait();
		}

		uint32_t const writtenBytes = static_cast<uint32_t>(CopyMemory(Slice<uint8_t>{
			.length = typeSize,
			.pointer = reinterpret_cast<uint8_t *>(channel + typeSize),
		}, inputBuffer));

		channel->storedBytes.Store(writtenBytes);
		channel->receiveEvent.signal();

		return writtenBytes;
	}

	Channel * OpenChannel(uint32_t typeSize) {
		uint8_t * channelBuffer = Allocate(Allocator::Default, sizeof(Channel) + typeSize);

		return (channelBuffer ? new (channelBuffer) Channel{typeSize} : nullptr);
	}

	void InitScheduler() {
		scheduler.bind();
	}

	void ScheduleTask(Callable<void()> const & task) {
		waitGroup.add(1);

		marl::schedule([task]{
			task.Invoke();
			waitGroup.done();
		});
	}

	void WaitAllTasks() {
		waitGroup.wait();
	}
}
