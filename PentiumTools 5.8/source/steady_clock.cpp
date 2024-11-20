#include "stdafx.h"
#include "SkillManager.h"
#include "ZzzOpenglUtil.h"
#include "steady_clock.h"


GameManager gGameManager;
std::random_device rd;  // a seed source for the random number engine
std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
std::uniform_real_distribution<> distrib(0.0, 1.0);

GameManager::GameManager()
{
	fps_avg = 25.0;
	frame_check = 0;

	time_normal_check = true;
	fps_animation_factor = 1.0;

	last_time = std::chrono::steady_clock::now();
	runtimeThread = std::chrono::steady_clock::now();
	last_check_time = std::chrono::steady_clock::now();
}

GameManager::~GameManager()
{
}

bool GameManager::get_base_check()
{
	return time_normal_check;
}

double GameManager::get_fps_factor()
{
	return fps_animation_factor;
}

bool GameManager::rand_calc_check(int fr)
{
	const auto rand_value = distrib(gen);
	const auto chance = (fr == 1) ? fps_animation_factor : (1.0 / fr) * fps_animation_factor;
	return rand_value <= chance;
}

double GameManager::runtimeTakeout()
{
	const double fps_avg_max = CLOCKS_PER_SEC / FPS_LIMIT_FACTOR;

	const std::chrono::steady_clock::time_point last_render_tick = runtimeThread;
	runtimeThread = std::chrono::steady_clock::now();

	double difTime = std::chrono::duration<double>(runtimeThread - last_render_tick).count();

	difTime *= CLOCKS_PER_SEC;
	if (difTime < fps_avg_max)
	{
		uint64_t rest_ms = (fps_avg_max - difTime);
		std::this_thread::sleep_for(std::chrono::milliseconds(rest_ms));
		difTime += fps_avg_max;
	}

	return (difTime);
}

void GameManager::calc_base_check()
{
	auto current_time = std::chrono::steady_clock::now();

	double elapsed_time = std::chrono::duration<double>(current_time - last_check_time).count();

	if (elapsed_time >= (0.04))
	{
		time_normal_check = true;
		last_check_time = current_time;
	}
	else
	{
		time_normal_check = false;
	}
}

void GameManager::calc_frame_avg()
{
	static int timeinit = 0;

	if (!timeinit)
	{
		FPS = FPS_DEFAULT_FACTOR;
		timeinit = true;
		start_time = std::chrono::steady_clock::now();
	}

	frame_check++;
	WorldTime = (float)timeGetTime();

	auto current_time = std::chrono::steady_clock::now();
	double difTime = std::chrono::duration<double>(current_time - last_time).count();
	double diflast = std::chrono::duration<double>(current_time - start_time).count();


	if (difTime <= 0)
	{
		DeltaT = 0.0001; // it just cant be 0
		fps_avg = 10.0;
	}
	else
	{
		DeltaT = difTime;
		fps_avg = 1.0 / difTime;
	}


	if(diflast >= 1.0)
	{
		FPS = frame_check;
		start_time = current_time;
		frame_check = 0;
	}

	last_time = current_time;
	fps_animation_factor = minf(static_cast<double>(FPS_DEFAULT_FACTOR / fps_avg), 1.0); // no less than 10 fps

	if (SceneFlag == MAIN_SCENE)
	{
		gSkillManager.CalcSkillDelay(static_cast<int>((difTime * CLOCKS_PER_SEC)));
	}

	calc_base_check();
}

double timepow(double pow1)
{
	return pow(pow1, FPS_ANIMATION_FACTOR);
}

double timefac(double fac1)
{
	return (fac1 * FPS_ANIMATION_FACTOR);
}

bool steady_clock_::numeral(int element) const
{
	if (_runvalueback >= _runvalue)
		return (_runvalueback >= element && _runvalue <= element);
	else
		return (_runvalue >= element && _runvalueback <= element);
}

bool steady_clock_::duration(int element)
{
	if (_runvalueback >= _runvalue)
		return _runvalue <= (std::floor((_runvalueback / element)) * element);
	else
		return _runvalue >= (std::ceil((_runvalueback / element)) * element);
}

bool steady_clock_::residual_duration(int element, int time)
{
	int multiplo;
	double residual_memory1, residual_memory2;

	if (_runvalueback >= _runvalue)
	{
		multiplo = (std::floor((_runvalueback / element)) * element);

		residual_memory1 = (_runvalue - multiplo);
		residual_memory2 = (_runvalueback - multiplo);

		return (residual_memory2 >= time && residual_memory1 <= time);
	}
	else
	{
		multiplo = (std::floor((_runvalue / element)) * element);

		residual_memory1 = (_runvalue - multiplo);
		residual_memory2 = (_runvalueback - multiplo);

		return (residual_memory2 <= time && residual_memory1 >= time);
	}
}