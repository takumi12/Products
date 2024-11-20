#pragma once
#define FPS_LIMIT_FACTOR					65.0
#define FPS_DEFAULT_FACTOR					25.0
#define FPS_ANIMATION_FACTOR				(gGameManager.get_fps_factor())
#define time_base_check						(gGameManager.get_base_check())
#define rand_fps_check(reference)			(gGameManager.rand_calc_check(reference))
#define RunTimeTakeOut()					(gGameManager.runtimeTakeout())


class steady_clock_
{
public:
	double _runvalue;
	double _runvalueback;

	// Constructor
	steady_clock_() : _runvalue(0.0), _runvalueback(0.0) {}
	steady_clock_(double value) : _runvalue(value), _runvalueback(value) {}

	steady_clock_& operator=(double value) {
		_runvalueback = value;
		_runvalue = value;
		return *this;
	}

	explicit operator double() const {
		return _runvalue;
	}

	explicit operator float() const {
		return static_cast<float>(_runvalue);
	}

	explicit operator int() const {
		return static_cast<int>(_runvalue);
	}

	// Adición y Asignación
	template<typename T>
	steady_clock_& operator+=(T value) {
		_runvalueback = _runvalue;
		_runvalue += static_cast<double>(value);
		return *this;
	}

	// Sustracción y Asignación
	template<typename T>
	steady_clock_& operator-=(T value) {
		_runvalueback = _runvalue;
		_runvalue -= static_cast<double>(value);
		return *this;
	}

	// Multiplicación y Asignación
	template<typename T>
	steady_clock_& operator*=(T value) {
		_runvalueback = _runvalue;
		_runvalue *= static_cast<double>(value);
		return *this;
	}

	// División y Asignación
	template<typename T>
	steady_clock_& operator/=(T value) {
		_runvalueback = _runvalue;
		_runvalue /= static_cast<double>(value);
		return *this;
	}

	// Incremento
	steady_clock_& operator++() { // Pre-incremento
		_runvalueback = _runvalue;
		++_runvalue;
		return *this;
	}

	steady_clock_ operator++(int) {
		steady_clock_ temp = *this;
		_runvalueback = _runvalue;
		++_runvalue;
		return temp;
	}

	// Incremento
	steady_clock_& operator--() { // Pre-incremento
		_runvalueback = _runvalue;
		--_runvalue;
		return *this;
	}

	steady_clock_ operator--(int) {
		steady_clock_ temp = *this;
		_runvalueback = _runvalue;
		--_runvalue;
		return temp;
	}

	// Módulo
	double operator%(int element) const
	{
		return std::fmod(_runvalue, element);
	}

	// Igualdad
	template<typename T>
	bool operator==(T value) const {
		return numeral(value);
	}

	// Igualdad
	template<typename T>
	bool operator!=(T value) const {
		return !numeral(value);
	}

	// Menor o igual que
	template<typename T>
	bool operator<=(T value) const {
		return _runvalue <= value;
	}

	// Mayor o igual que
	template<typename T>
	bool operator>=(T value) const {
		return _runvalue >= value;
	}

	// Menor que
	template<typename T>
	bool operator<(T value) const {
		return _runvalue < value;
	}

	// Mayor que
	template<typename T>
	bool operator>(T value) const {
		return _runvalue > value;
	}

	bool numeral(int element) const;
	bool duration(int element);
	bool residual_duration(int element, int time);
};


class GameManager
{
public:
	GameManager();
	virtual~GameManager();

	bool get_base_check();
	double get_fps_factor();

	void calc_base_check();
	void calc_frame_avg();
	bool rand_calc_check(int fr);

	double runtimeTakeout();
private:
	int frame_check;
	bool time_normal_check;

	double fps_avg;
	double fps_animation_factor;
	std::chrono::steady_clock::time_point last_time;
	std::chrono::steady_clock::time_point start_time;
	std::chrono::steady_clock::time_point runtimeThread;
	std::chrono::steady_clock::time_point last_check_time;
};

extern GameManager gGameManager;
extern double timepow(double pow1);
extern double timefac(double fac1);