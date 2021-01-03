
namespace Ona::Core {
	class Object {
		public:
		Object() = default;

		Object(Object const &) = delete;

		virtual ~Object() {};

		virtual bool Equals(Object const * that) const {
			return (this == that);
		}

		virtual uint64_t ToHash() const {
			return reinterpret_cast<int64_t>(this);
		}

		virtual String ToString() const {
			return String{"{}"};
		}
	};
}
