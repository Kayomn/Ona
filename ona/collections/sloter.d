module ona.collections.sloter;

import
	core.stdc.string,
	ona.core,
	std.traits;

public struct Key(IndexType) if (isIntegral!IndexType) {
	private IndexType index;

	private ulong generation;

	@nogc
	public IndexType indexOf() const pure {
		return this.index;
	}

	@nogc
	public Type opCast(Type)() const shared pure if (is(Type == bool)) {
		return (this.generation != 0);
	}
}

public final class Sloter(ValueType, IndexType) if (isIntegral!IndexType) {
	private ulong generation;

	private Allocator allocator;

	private Key!IndexType* slots;

	private ValueType* values;

	private IndexType count;

	private IndexType capacity;

	private IndexType freeHead;

	@nogc
	public this(Allocator allocator) pure {
		this.allocator = allocator;
	}

	@nogc
	public ~this() {
		static if (hasElaborateDestructor!ValueType) {
			foreach (ref value; this.valuesOf()) destroy(value);
		}

		this.allocator.deallocate(this.values);
	}

	@nogc
	public inout (Allocator) allocatorOf() inout pure {
		return this.allocator;
	}

	public Key!IndexType append(ValueType value) {
		if (this.freeHead) {
			immutable freeSlotIndex = this.freeHead;

			scope (exit) this.count += 1;

			this.freeHead = this.slots[this.freeHead].index;
			this.slots[freeSlotIndex].index = this.count;
			this.values[this.count] = value;

			return Key!IndexType(freeSlotIndex, this.slots[freeSlotIndex].generation);
		} else if (this.reserve(this.count ? this.count : 2)) {
			scope (exit) this.count += 1;

			this.slots[this.count] = Key!IndexType(this.count, 1);
			this.values[this.count] = value;

			return Key!IndexType(this.count, this.slots[this.count].generation);
		}

		// Invalid key due to allocation failure.
		return Key!IndexType();
	}

	@nogc
	public ref inout (ValueType) at(in IndexType index) pure inout {
		return this.values[index];
	}

	@nogc
	public IndexType capacityOf() pure const {
		return this.count;
	}

	@nogc
	public IndexType countOf() pure const {
		return this.count;
	}

	@nogc
	public inout (ValueType)* lookup(in Key!IndexType key) pure inout {
		return (this.validate(key) ? (this.values + this.slots[key.index].index) : null);
	}

	public bool remove(in Key!IndexType key) pure {
		if (this.validate(key)) {
			immutable targetIndex = this.slots[key.index].index;
			immutable difference = ((this.count - 1) - targetIndex);
			immutable oldFreeHead = this.freeHead;
			this.freeHead = key.index;

			if (difference) {
				memmove((this.values + targetIndex), (this.values + targetIndex + 1), difference);
			}

			this.slots[key.index].index = oldFreeHead;
			this.slots[key.index].generation += 1;
			this.count -= 1;

			return true;
		}

		return false;
	}

	@nogc
	public bool reserve(in IndexType capacity) {
		immutable newCapacity = (this.capacity + capacity);

		this.slots = (cast(Key!IndexType*)this.allocator.reallocate(
			this.slots,
			(newCapacity * Key!IndexType.sizeof)
		).ptr);

		this.values = (cast(ValueType*)this.allocator.reallocate(
			this.values,
			(newCapacity * ValueType.sizeof)
		).ptr);

		if ((this.slots != null) && (this.values != null)) {
			this.capacity = newCapacity;

			return true;
		}

		this.capacity = 0;

		return false;
	}

	@nogc
	private bool validate(in Key!IndexType key) pure const {
		if (key.index < this.count) {
			immutable slotKey = this.slots[key.index];

			if (slotKey.generation == key.generation) return true;
		}

		return false;
	}

	@nogc
	public inout (ValueType)[] valuesOf() inout pure {
		return this.values[0 .. this.count];
	}
}
