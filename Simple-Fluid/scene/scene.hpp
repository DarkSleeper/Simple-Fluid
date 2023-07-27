#pragma once
#include <memory.h>
#include "../fluid/fluid.hpp"
#define SIM_WIDTH 1280
#define SIM_HEIGHT 720

struct Scene
{
	float gravity{-9.81};
	float dt{1.0 / 120.0};
	int numIters{100};
	int frameNr{0};
	float overRelaxation{1.9};
	float obstacleX{0.0};
	float obstacleY{0.0};
	float obstacleRadius{0.15};
	bool paused{false};
	int sceneNr{0};
	bool showObstacle{false};
	bool showStreamlines{false};
	bool showVelocities{false};
	bool showPressure{false};
	bool showSmoke{true};
	std::unique_ptr<Fluid> fluid;
};

inline void simulate(Scene& scene)
{
	if (!scene.paused) {
		scene.fluid->simulate(scene.dt, scene.gravity, scene.numIters);
		scene.frameNr++;
	}
}

inline void setObstacle(Scene& scene, float x, float y, bool reset) {

	auto vx = 0.0;
	auto vy = 0.0;

	if (!reset) {
		vx = (x - scene.obstacleX) / scene.dt;
		vy = (y - scene.obstacleY) / scene.dt;
	}

	scene.obstacleX = x;
	scene.obstacleY = y;
	auto r = scene.obstacleRadius;
	auto& f = *scene.fluid.get();
	auto n = f.numY;
	auto cd = std::sqrt(2.f) * f.h;

	for (auto i = 1; i < f.numX - 2; i++) {
		for (auto j = 1; j < f.numY - 2; j++) {

			f.s[i * n + j] = 1.0;

			auto dx = (i + 0.5) * f.h - x;
			auto dy = (j + 0.5) * f.h - y;

			if (dx * dx + dy * dy < r * r) {
				f.s[i * n + j] = 0.0;
				if (scene.sceneNr == 2)
					f.m[i * n + j] = 0.5 + 0.5 * std::sin(0.1 * scene.frameNr);
				else
					f.m[i * n + j] = 1.0;
				f.u[i * n + j] = vx;
				f.u[(i + 1) * n + j] = vx;
				f.v[i * n + j] = vy;
				f.v[i * n + j + 1] = vy;
			}
		}
	}

	scene.showObstacle = true;
}

inline void setupScene(Scene& scene, int sceneNr = 0)
{
	scene.sceneNr = sceneNr;
	scene.obstacleRadius = 0.15;
	scene.overRelaxation = 1.9;

	scene.dt = 1.0 / 60.0;
	scene.numIters = 40;

	auto res = 100;

	//if (sceneNr == 0)
	//	res = 50;
	//else if (sceneNr == 3)
	//	res = 200;

	auto domainHeight = 1.0f;
	auto domainWidth = domainHeight / SIM_HEIGHT * SIM_WIDTH;
	auto h = domainHeight / res;

	auto numX = floor(domainWidth / h);
	auto numY = floor(domainHeight / h);

	auto density = 1000.0;

	scene.fluid = std::move(std::unique_ptr<Fluid>(new Fluid(density, numX, numY, h)));
	auto& f = *scene.fluid.get();

	auto n = f.numY;

	if (sceneNr == 0) {   		// tank

		for (auto i = 0; i < f.numX; i++) {
			for (auto j = 0; j < f.numY; j++) {
				auto s = 1.0;	// fluid
				if (i == 0 || i == f.numX - 1 || j == 0)
					s = 0.0;	// solid
				f.s[i * n + j] = s;
			}
		}
		scene.gravity = -9.81;
		scene.showPressure = true;
		scene.showSmoke = false;
		scene.showStreamlines = false;
		scene.showVelocities = false;
	}
	else if (sceneNr == 1 || sceneNr == 3) { // vortex shedding

		auto inVel = 2.0;
		for (auto i = 0; i < f.numX; i++) {
			for (auto j = 0; j < f.numY; j++) {
				auto s = 1.0;	// fluid
				if (i == 0 || j == 0 || j == f.numY - 1)
					s = 0.0;	// solid
				f.s[i * n + j] = s;

				if (i == 1) {
					f.u[i * n + j] = inVel;
				}
			}
		}

		auto pipeH = 0.1 * f.numY;
		auto minJ = floor(0.5 * f.numY - 0.5 * pipeH);
		auto maxJ = floor(0.5 * f.numY + 0.5 * pipeH);

		for (auto j = minJ; j < maxJ; j++)
			f.m[j] = 0.0;

		setObstacle(scene, 0.4, 0.5, true);

		scene.gravity = 0.0;
		scene.showPressure = false;
		scene.showSmoke = true;
		scene.showStreamlines = false;
		scene.showVelocities = false;

		if (sceneNr == 3) {
			scene.dt = 1.0 / 120.0;
			scene.numIters = 100;
			scene.showPressure = true;
		}

	}
	else if (sceneNr == 2) { // paint

		scene.gravity = 0.0;
		scene.overRelaxation = 1.0;
		scene.showPressure = false;
		scene.showSmoke = true;
		scene.showStreamlines = false;
		scene.showVelocities = false;
		scene.obstacleRadius = 0.1;
	}
}