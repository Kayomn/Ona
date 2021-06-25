module ona.collections.map;

private import core.memory, ona.functional, std.traits;

/**
 * Hashing map structure for indexing `ValueType` values under unique `KeyType` keys, creating
 * "item" associations between them.
 *
 * `toHash` and `opEquals` member functions may be defined on `KeyType` for fine-tuned indexing
 * behavior.
 */
public final class HashTable(KeyType, ValueType) {
	private enum defaultHashSize = 256;

	private struct Item {
		KeyType key;

		ValueType value;
	}

	private struct Bucket {
		Item item;

		Bucket* next;
	}

	private uint itemCount = 0;

	private uint bucketCapacity = 0;

	private Bucket** buckets = null;

	private Bucket* freeBuckets = null;

	private float loadMaximum = 0;

	/**
	 * Clears all items from the contents, calling the destructors of any contained `struct` types.
	 */
	public void clear() {
		if (this.itemCount) {
			foreach (i; 0 .. this.bucketCapacity) {
				Bucket* rootBucket = this.buckets[i];

				if (rootBucket) {
					static if (is(KeyType == struct) && hasElaborateDestructor!KeyType) {
						rootBucket.item.key.__xdtor();
					}

					static if (is(ValueType == struct) && hasElaborateDestructor!ValueType) {
						rootBucket.item.value.__xdtor();
					}

					Bucket * bucket = rootBucket;

					while (bucket.next) {
						static if (is(KeyType == struct) && hasElaborateDestructor!KeyType) {
							bucket.next.item.key.__xdtor();
						}

						static if (is(ValueType == struct) && hasElaborateDestructor!ValueType) {
							bucket.next.item.value.__xdtor();
						}

						bucket = bucket.next;
					}

					bucket.next = this.freeBuckets;
					this.freeBuckets = rootBucket;
					this.buckets[i] = null;
				}
			}

			this.itemCount = 0;
		}
	}

	/**
	 * Returns the number of items currently held.
	 */
	public uint count() const {
		return this.itemCount;
	}

	/**
	 * Assigns `value` to `key`, returning a reference to the inserted value.
	 */
	public ref ValueType assign(in KeyType key, ValueType value) {
		Bucket* createBucket(in KeyType key, ValueType value) {
			if (this.freeBuckets) {
				Bucket* bucket = this.freeBuckets;
				this.freeBuckets = this.freeBuckets.next;
				(*bucket) = Bucket(Item(key, value), null);

				return bucket;
			}

			auto bucket = cast(Bucket*)pureMalloc(Bucket.sizeof);

			assert(bucket, typeof(this).stringof ~ " assignment bucket creation out of memory");

			(*bucket) = Bucket(Item(key, value), null);

			return bucket;
		}

		if (!(this.bucketCapacity)) this.rehash(defaultHashSize);

		immutable (uint) hash = (cast(uint)(hashOf(key) % this.bucketCapacity));
		Bucket* bucket = this.buckets[hash];

		scope (exit) {
			if (this.loadMaximum && (this.loadFactor() > this.loadMaximum)) {
				this.rehash((cast(uint)this.bucketCapacity) * 2);
			}
		}

		if (bucket) {
			if (bucket.item.key == key) {
				bucket.item.value = value;

				return bucket.item.value;
			}

			while (bucket.next) {
				bucket = bucket.next;

				if (bucket.item.key == key) {
					bucket.item.value = value;

					return bucket.item.value;
				}
			}

			bucket.next = createBucket(key, value);

			return bucket.next.item.value;
		}

		bucket = createBucket(key, value);
		this.buckets[hash] = bucket;
		this.itemCount += 1;

		return bucket.item.value;
	}

	/**
	 * Returns the table saturation of the items contained as a value between `0` and `1`, where `0`
	 * is empty and `1` is complete saturation.
	 */
	public float loadFactor() const {
		return (this.itemCount / this.bucketCapacity);
	}

	/**
	 * Looks for a value indexed by `key`, returning it, if found, or nothing wrapped in a
	 * [Optional].
	 */
	public inout (Optional!ValueType) lookup(in KeyType key) inout {
		inout (ValueType*) value = this.lookupImplementation(key);

		if (value) {
			return inout (Optional!ValueType)(*value);
		}

		return Optional!ValueType();
	}

	private inout (ValueType)* lookupImplementation(in KeyType key) inout {
		if (this.buckets && this.itemCount) {
			inout (Bucket)* bucket = this.buckets[hashOf(key) % this.bucketCapacity];

			if (bucket) {
				while (!(bucket.item.key == key)) bucket = bucket.next;

				if (bucket) return (&bucket.item.value);
			}
		}

		return null;
	}

	/**
	 * Re-generates the table for indexing items to be equal in size to `tableSize`, with lower
	 * numbers using less memory but experiencing slower lookup speeds, and higher numbers using
	 * more memory and faster lookup speeds.
	 */
	public void rehash(in uint tableSize) {
		Bucket** oldBuckets = this.buckets;
		immutable (uint) oldCapacity = this.bucketCapacity;
		this.buckets = (cast(Bucket**)pureMalloc((Bucket*).sizeof * tableSize));

		assert(this.buckets, typeof(this).stringof ~ " Rehashing out of memory");

		this.bucketCapacity = tableSize;

		if (oldBuckets && this.itemCount) {
			foreach (i; 0 .. oldCapacity)  {
				Bucket* bucket = oldBuckets[i];

				while (bucket) {
					this.assign(bucket.item.key, bucket.item.value);

					bucket = bucket.next;
				}
			}
		}

		pureFree(oldBuckets);
	}
}
