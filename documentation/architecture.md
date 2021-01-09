# Architecture Documentation

## Core Principles

  * Use right tool for the right job.
  * Avoid dynamic allocation where possible.
  * Simple things can be simple: don't wrap everything in a class.
  * Polymorphism beyond "interface" classes is prohibited.

### Class Types

All classes should, in some way, inherit `Ona::Core::Object`. This is a common class that provides necessary features and restrictions for working with runtime types like classes in Ona.

As mentioned, `Ona::Core::Object` restricts some things - with one of them being copy operations. When allocated on the stack, a class cannot be copied from the location it was initialized. This is a safeguard from destructor and copy constructor call abuse that is prevalent when passing complex types around by value.

Unless it is less costly and the object lifetime is short, classes should be allocated on the heap and passed around by reference.

```cpp
FixedArray<uint8_t, 24> tempBuffer = {};

for (size_t i = 0; i < tempBuffer.Length(); i += 1) {
	tempBuffer.At(i) = i;
}
```

Stack-allocated classes are to be treated as an optimization technique to avoid unnecessary double-calls to the underlying allocation strategy.

### Inheritance

As mentioned in the project principles, interface classes are the only acceptable use of polymorphism.

```cpp
template<typename ValueType> class Collection {
	public:
	virtual void Clear() = 0;

	virtual size_t Count() const = 0;

	virtual void ForValues(
		Callable<void(ValueType &)> const & action
	) = 0;

	virtual void ForValues(
		Callable<void(ValueType const &)> const & action
	) const = 0;
};
```

This is so as to discourage the use of polymorphism as a crutch for fixing design problems in software. Interfaces are only created when there is an evident need for them, with "design by interface" being a largely discouraged mentality in Ona as it bloats the codebase, increases compile times, and increases the time spent maintaining code.

So when to use interfaces? In short, interfaces are to be used when there's an evident need for runtime polymorphism to implement a good solution, such as when dealing runtime-dynamic logic or wrapping a selection of platform-specific code.

### Simple Things Can Be Simple

Just as interfaces should not be used everywhere, classes themselves should also not be used everywhere. For most simple tasks, structs with member functions and / or free-standing functions will serve the purpose of most tasks. Classes are incorporated within Ona as a suite of functions that carry frequently-changing state within them.

```cpp
class FileServer : public Object {
	public:
	virtual bool CheckFile(
		String const & filePath
	) = 0;

	virtual void CloseFile(
		File & file
	) = 0;

	virtual bool OpenFile(
		String const & filePath,
		File & file,
		File::OpenFlags openFlags
	) = 0;

	virtual void Print(
		File & file,
		String const & string
	) = 0;

	virtual size_t Read(
		File & file,
		Slice<uint8_t> output
	) = 0;

	virtual int64_t SeekHead(
		File & file,
		int64_t offset
	) = 0;

	virtual int64_t SeekTail(
		File & file,
		int64_t offset
	) = 0;

	virtual int64_t Skip(
		File & file,
		int64_t offset
	) = 0;

	virtual size_t Write(
		File & file,
		Slice<uint8_t const> const & input
	) = 0;
};

```

For the sake of demonstration, above is an example of the `Ona::Engine::FileServer` interface class. The "server" design pattern is used a lot in Ona as a means of encapsulating platforms-specific code inside of a class. In this case, it is hiding the implementation of the file system, so any unit of code may plug itself in as a replacement for it should the need arise.

```cpp
struct File {
	enum OpenFlags {
		OpenUnknown = 0,
		OpenRead = 0x1,
		OpenWrite = 0x2
	};

	FileServer * server;

	void * userdata;
};
```

`Ona::Engine::File`, on the other hand, doesn't need to change its state beyond its initialization and then once more when it is closed.

```cpp
struct ConfigValue {
	Config * config;

	int64_t handle;

	void Free();
};
```

Some struct instances, like `Ona::Engine::ConfigValue`, may choose to implement member-functions like "Free", which itself is just a wrapper around `Ona::Engine::Config::FreeValue` for the sake of convenience.

This allows it to be contextually elevated to the status of "class" when the need arises via  the `Ona::Core::Owned` class, which accepts a type and takes full scope-based ownership of it - calling the free function of the type once it is done with.

### Dynamic Memory Allocation

Much like classes and interfaces, dynamic memory usage should be moderated to where it is necessary. If the upper bounds of an operation can be reasoned about, allocate for it on the stack using either a `Ona::Core::FixedArray` in automatic memory or a stack buffer.

```cpp
void MyFunction(
	Allocator * allocator,
	FixedArray<uint8_t> const & information
) {
	PackedStack<float> transformed = {allocator};

	for (size_t i = 0; i < information.Length(); i += 1) {
		// ...
	}
}
```

When dynamic memory can't be avoided, it is encouraged that the function or class using it expose itself to the Ona polymorphic allocator API. This enables other programmers using the code to control the allocation strategy used on a per-case and heavily contextualized basis.

## Graphics API

Graphics APIs are already difficult to design without needing to support multiple threads making calls to it. Ona achieves this by seperating the API into two components: server and queue.

```cpp
GraphicsQueue * graphics = ona->graphicsQueueAcquire();

ona->renderSprite(graphics, ...);
```

Graphics calls cannot be directly made to the server. Instead, a `Ona::Engine::GraphicsQueue` instance must first be created within the local thread by calling the locking `Ona::Engine::GraphicsServer::CreateQueue` procedure. This will instantiate a new queue resource and make the graphics server aware of it for later use.

From there, any number of graphics commands may be queued by the thread using the graphics queue.

```py
async_pool = AsyncPool()

for loaded_system in loaded_systems:
	async_pool.enqueue(loaded_system.update())

# Stalls the main thread until all async tasks
# conclude operation.
async_pool.execute()

# Exclusive control is returned to the main thread
# as all others have completed their processing for
# the frame.
#
# Now, all queued graphics commands from the
# threads are dispatched.
graphics_server.update()
```

Once all threads conclude their update for that frame, control is returned to the main thread and it iterates over each graphics queue, dispatching them in order of thread priority. This approach allows Ona to keep the rendering loop completely lock-free.
