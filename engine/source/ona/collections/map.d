module ona.collections.map;

private import ona.functional, std.traits;

public interface Map(KeyType, ValueType) {

}

public final class HashTable(KeyType, ValueType) : Map!(KeyType, ValueType) {
	private enum defaultHashSize = 256;

	private struct Item {
		KeyType key;

		ValueType value;
	}

	private struct Bucket {
		Item item;

		Bucket * next;
	}

	private uint itemCount = 0;

	private Bucket*[] buckets = [];

	private Bucket* freeBuckets = null;

	private float loadMaximum = 0;

	private Bucket* createBucket(in KeyType key, ValueType value) {
		if (this.freeBuckets) {
			Bucket* bucket = this.freeBuckets;
			this.freeBuckets = this.freeBuckets.next;
			(*bucket) = Bucket(Item(key, value), null);

			return bucket;
		}

		return new Bucket(Item(key, value), null);
	}

	public void clear() {
		if (this.itemCount) {
			for (uint i = 0; i < this.buckets.length; i += 1) {
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

	public uint count() const {
		return this.itemCount;
	}

	public void forEach(scope void delegate(in KeyType, in ValueType) action) const {
		uint count = this.itemCount;

		for (uint i = 0; count != 0; i += 1) {
			const (Bucket)* bucket = this.buckets[i];

			while (bucket) {
				action(bucket.item.key, bucket.item.value);

				count -= 1;
				bucket = bucket.next;
			}
		}
	}

	public void forValues(scope void delegate(in ValueType) action) const {
		uint count = this.itemCount;

		for (uint i = 0; count != 0; i += 1) {
			const (Bucket)* bucket = this.buckets[i];

			while (bucket) {
				action(bucket.item.value);

				count -= 1;
				bucket = bucket.next;
			}
		}
	}

	@safe
	public ref ValueType assign(in KeyType key, ValueType value) {
		if (!(this.buckets.length)) this.rehash(defaultHashSize);

		immutable (uint) hash = (cast(uint)(hashOf(key) % this.buckets.length));
		Bucket* bucket = this.buckets[hash];

		scope (exit) {
			if (this.loadMaximum && (this.loadFactor() > this.loadMaximum)) {
				this.rehash((cast(uint)this.buckets.length) * 2);
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

			bucket.next = this.createBucket(key, value);

			return bucket.next.item.value;
		}

		bucket = this.createBucket(key, value);
		this.buckets[hash] = bucket;
		this.itemCount += 1;

		return bucket.item.value;
	}

	public bool erase(in KeyType key) {
		// TODO: Implement.
		return false;
	}

	public float loadFactor() const {
		return (this.itemCount / this.buckets.length);
	}

	public inout (Optional!ValueType) lookup(in KeyType key) inout {
		inout (ValueType*) value = this.lookupImplementation(key);

		if (value) {
			return inout (Optional!ValueType)(*value);
		}

		return Optional!ValueType();
	}

	@safe
	private inout (ValueType)* lookupImplementation(in KeyType key) inout {
		if (this.buckets.length && this.itemCount) {
			inout (Bucket)* bucket = this.buckets[hashOf(key) % this.buckets.length];

			if (bucket) {
				while (!(bucket.item.key == key)) bucket = bucket.next;

				if (bucket) return (&bucket.item.value);
			}
		}

		return null;
	}

	public void rehash(in uint tableSize) {
		Bucket*[] oldBuckets = this.buckets;
		this.buckets = new Bucket*[tableSize];

		if (oldBuckets.length && this.itemCount) {
			foreach (i; 0 .. this.buckets.length)  {
				Bucket* bucket = oldBuckets[i];

				while (bucket) {
					this.assign(bucket.item.key, bucket.item.value);

					bucket = bucket.next;
				}
			}
		}
	}

	@safe
	public ref ValueType require(in KeyType key, ValueType fallback) {
		ValueType* lookupValue = this.lookupImplementation(key);

		if (lookupValue) return (*lookupValue);

		return this.assign(key, fallback);
	}

	public ref ValueType require(in KeyType key, scope ValueType delegate() fallbackProducer) {
		ValueType* lookupValue = this.lookupImplementation(key);

		if (lookupValue) return (*lookupValue);

		return this.assign(key, fallbackProducer());
	}
}
