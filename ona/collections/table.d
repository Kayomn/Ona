module ona.collections.table;

private import
	ona.core,
	std.traits;

/**
 * Hash-ordered table of values organized and accessible via key types.
 */
public class Table(KeyType, ValueType) {
	private struct Bucket {
		Entry entry;

		Bucket * next;
	}

	private struct Entry {
		KeyType key;

		ValueType value;
	}

	private enum defaultHashSize = 256;

	private NotNull!Allocator allocator;

	private size_t count;

	/**
	 * When assigned a non-zero value, `Table.loadMaximum` allows the `Table` to intelligently
	 * rehash itself as it grows, optimizing for a maximum load limit of a value between `float.min`
	 * and `1.0f`.
	 */
	public float loadMaximum = 0.0f;

	private Bucket* freedBuckets;

	private Bucket*[] buckets;

	/**
	 * Constructs an `Table` with `allocator` as the `Allocator`.
	 */
	@nogc
	public this(NotNull!Allocator allocator) pure {
		this.allocator = allocator;
	}

	public ~this() {
		void destroyChainAllocator(Bucket* bucket) {
			while (bucket) {
				Bucket* nextBucket = bucket.next;

				static if (hasElaborateDestructor!KeyType) {
					destroy(bucket.entry.key);
				}

				static if (hasElaborateDestructor!ValueType) {
					destroy(bucket.entry.value);
				}

				this.allocator.deallocate(bucket);

				bucket = nextBucket;
			}
		}

		destroyChainAllocator(this.freedBuckets);

		if (this.count) {
			foreach (i; 0 .. this.buckets.length) destroyChainAllocator(this.buckets[i]);
		}

		this.allocator.deallocate(this.buckets.ptr);
	}

	/**
	 * Attempts to retrieve the `Allocator` being used by the `Table`.
	 */
	@nogc
	public inout (NotNull!Allocator) allocatorOf() pure inout {
		return this.allocator;
	}

	/**
	 * Clears contents of the `Table`, destroying values if they are destructible.
	 */
	public void clear() pure {
		if (this.count) {
			foreach (i; 0 .. this.buckets.length) {
				Bucket* rootBucket = this.buckets[i];

				if (rootBucket) {
					static if (hasElaborateDestructor!KeyType) {
						destroy(rootBucket.entry.key);
					}

					static if (hasElaborateDestructor!ValueType) {
						destroy(rootBucket.entry.value);
					}

					Bucket* bucket = rootBucket;

					while (bucket.next) {
						static if (hasElaborateDestructor!KeyType) {
							destroy(bucket.entry.key);
						}

						static if (hasElaborateDestructor!ValueType) {
							destroy(bucket.entry.value);
						}

						bucket = bucket.next;
					}

					bucket.next = this.freedBuckets;
					this.freedBuckets = bucket;
					this.buckets[i] = null;
				}
			}

			this.count = 0;
		}
	}

	/**
	 * Retrieves the number of values in the `Table`.
	 */
	@nogc
	public size_t countOf() const pure {
		return this.count;
	}

	@nogc
	private Bucket* createBucket(KeyType key, ValueType value) {
		if (this.freedBuckets) {
			Bucket* bucket = this.freedBuckets;
			this.freedBuckets = this.freedBuckets.next;
			(*bucket) = Bucket(Entry(key, value), null);

			return bucket;
		} else {
			Bucket* bucket = (cast(Bucket*)(this.allocator.allocate(Bucket.sizeof).ptr));

			if (bucket) {
				(*bucket) = Bucket(Entry(key, value), null);

				return bucket;
			}
		}

		return null;
	}

	private static ulong hashKey(in KeyType key) {
		static assert(
			__traits(hasMember, KeyType, "toHash"),
			(KeyType.stringof ~ " is not a hashable type")
		);

		return key.toHash();
	}

	/**
	 * Attempts to insert a `value` into the `Table` at `key`, overwriting any value that may have
	 * existed at the index of `key` in the process.
	 *
	 * Overwritten values are destroyed, provided they are destructible.
	 */
	public ValueType* insert(KeyType key, ValueType value) {
		if (!(this.buckets)) this.rehash(defaultHashSize);

		immutable (size_t) hash = (hashKey(key) % this.buckets.length);
		Bucket* bucket = this.buckets[hash];

		if (bucket) {
			if (bucket.entry.key == key) {
				// Key fits in the root of the bucket table and overwrites an existing value.
				bucket.entry.value = value;

				return (&bucket.entry.value);
			} else {
				while (bucket.next) {
					bucket = bucket.next;

					if (bucket.entry.key == key) {
						// Key is nested in the bucket table and overwrites an existing value.
						bucket.entry.value = value;

						return (&bucket.entry.value);
					}
				}

				// Key is nested in the bucket table with no overwritten values.
				bucket.next = this.createBucket(key, value);

				return (bucket.next ? (&bucket.next.entry.value) : null);
			}

			bucket = bucket.next;
		} else {
			// Key fits in the root of the bucket table with no overwritten values.
			bucket = this.createBucket(key, value);
			this.buckets[hash] = bucket;
			this.count += 1;

			return (&bucket.entry.value);
		}

		if (this.loadMaximum && (this.loadFactorOf() > this.loadMaximum)) {
			this.rehash(this.buckets.length * 2);
		}

		return null;
	}

	public bool remove(KeyType key) pure {
		return false;
	}

	/**
	 * Attempts to re-hash the `Table` bucket table to have a root that is `tableSize` values large.
	 *
	 * As a general rule, a larger `tableSize` enables quicker lookup for values in exchange for
	 * storage space efficiency.
	 *
	 * If a non-zero `Table.loadMaximum` value is assigned, `Table.rehash` will be automatically
	 * called when the `Table` excedes the load limit.
	 *
	 * Failure to rehash the `Table` will result in `false` being returned, otherwise `true`.
	 */
	public bool rehash(const (size_t) tableSize) {
		Bucket*[] oldBuckets = this.buckets;

		this.buckets = (
			this.allocator ?
			(cast(Bucket*[])this.allocator.allocate(tableSize * size_t.sizeof)) :
			(cast(Bucket*[])allocate(tableSize * size_t.sizeof))
		);

		if (this.buckets) {
			zeroMemory(cast(ubyte[])this.buckets);

			if (oldBuckets && this.count) {
				foreach (i; 0 .. oldBuckets.length) {
					Bucket* bucket = oldBuckets[i];

					while (bucket) {
						this.insert(bucket.entry.key, bucket.entry.value);

						bucket = bucket.next;
					}
				}

				deallocate(oldBuckets.ptr);
			}

			return true;
		}

		return false;
	}

	/**
	 * Retrieves the load factor of the `Table` as a value between `0` and `Table.loadMaximum`.
	 */
	@nogc
	public float loadFactorOf() pure const {
		return (this.count / this.buckets.length);
	}

	/**
	 * Attempts to lookup `key` in the `Table`, returning the address of the associated value.
	 *
	 * If `key` does not point to a value in the `Table` then `null` is returned instead.
	 */
	public inout (ValueType*) lookup(KeyType key) inout {
		if (this.buckets && this.count) {
			inout (Bucket)* bucket = this.buckets[hashKey(key) % this.buckets.length];

			if (bucket) {
				while (bucket.entry.key != key) bucket = bucket.next;

				if (bucket) return (&bucket.entry.value);
			}
		}

		return null;
	}

	/**
	 * Attempts to lookup `key` in the `Table`. If the `key` does not point to a value in the
	 * `Table` then the return value of `producer` is inserted at `key` and its address is returned
	 * instead.
	 *
	 * If the `key` does not point to a value in the `Table` and the value created by `producer`
	 * fails to be inserted then `null` is returned instead.
	 */
	public ValueType* lookupOrInsert(KeyType key, ValueType delegate() producer) {
		ValueType* lookupValue = this.lookup(key);

		if (lookupValue) return lookupValue;

		this.insert(key, producer());

		return this.lookup(key);
	}

	/**
	 * Creates an iterable range for the `Table` which may be used in a `foreach` statement.
	 */
	public auto itemsOf() pure {
		struct Values {
			Table context;

			int opApply(int delegate(ref KeyType key, ref ValueType value) action) {
				if (this.context.count) foreach (i; 0 .. this.context.buckets.length) {
					Bucket* bucket = this.context.buckets[i];

					while (bucket) {
						if (action(bucket.entry.key, bucket.entry.value)) return 1;

						bucket = bucket.next;
					}
				}

				return 0;
			}
		}

		return Values(this);
	}
}
