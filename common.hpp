#ifndef ONA_COMMON_H
#define ONA_COMMON_H

#include <cassert>
#include <stddef.h>
#include <stdint.h>
#include <new>
#include <initializer_list>

#define $packed __attribute__ ((packed))

#define $length(arr) (sizeof(arr) / sizeof(*arr))

#include "common/core.hpp"
#include "common/text.hpp"
#include "common/math.hpp"
#include "common/system.hpp"
#include "common/array.hpp"

namespace Ona {
	class Channel;

	void CloseChannel(Channel * & channel);

	Channel * OpenChannel(uint32_t typeSize);

	uint32_t ChannelReceive(Channel * channel, Slice<uint8_t> outputBuffer);

	uint32_t ChannelSend(Channel * channel, Slice<uint8_t const> const & inputBuffer);

	void InitScheduler();

	void ScheduleTask(Callable<void()> const & task);

	void WaitAllTasks();
}

#include "common/collections.hpp"

#endif
