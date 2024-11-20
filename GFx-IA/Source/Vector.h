template<typename T>
class std_vector {
public:
	std_vector() : _size(0), _capacity(1), _data(new T[_capacity]) {}

	~std_vector() {
		delete[] _data;
	}

	void push_back(const T& value) {
		if (_size == _capacity) {
			_resize(_capacity * 2);
		}
		_data[_size++] = value;
	}

	size_t size() const {
		return _size;
	}

	T& operator[](size_t index) {
		if (index >= _size) {
			throw std::out_of_range("Index out of range");
		}
		return _data[index];
	}

	const T& operator[](size_t index) const {
		if (index >= _size) {
			throw std::out_of_range("Index out of range");
		}
		return _data[index];
	}
	
	void clear() {
		delete[] _data;
		_capacity = 1;
		_data = new T[_capacity];
		_size = 0;
	}
private:
	void _resize(size_t new_capacity) {
		T* new_data = new T[new_capacity];
		for (size_t i = 0; i < _size; ++i) {
			new_data[i] = _data[i];
		}
		delete[] _data;
		_data = new_data;
		_capacity = new_capacity;
	}

	size_t _size;
	size_t _capacity;
	T* _data;
};
