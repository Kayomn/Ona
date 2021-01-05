
namespace Ona::Collections {
	template<typename ValueType> class Queue : public Collection<ValueType> {
		public:
		virtual ValueType & Dequeue() = 0;

		virtual ValueType * Enqueue(ValueType const & value) = 0;
	};
}
