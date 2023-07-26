#pragma once
#include <vector>
#include <math.h>
#define U_FIELD 0
#define V_FIELD 1
#define S_FIELD 2

struct Fluid {
	Fluid(float density, int numX, int numY, float h) {
		this->density = density;
		this->numX = numX + 2;
		this->numY = numY + 2;
		this->numCells = this->numX * this->numY;
		this->h = h;
		this->u.resize(this->numCells);
		this->v.resize(this->numCells);
		this->newU.resize(this->numCells);
		this->newV.resize(this->numCells);
		this->p.resize(this->numCells);
		this->s.resize(this->numCells);
		this->m.resize(this->numCells, 1.0);
		this->newM.resize(this->numCells);
		//auto num = numX * numY;
	}

	void integrate(float dt, float gravity) {
		auto n = this->numY;
		for (auto i = 1; i < this->numX; i++) {
			for (auto j = 1; j < this->numY - 1; j++) {
				if (this->s[i * n + j] != 0.0 && this->s[i * n + j - 1] != 0.0)
					this->v[i * n + j] += gravity * dt;
			}
		}
	}

	void solveIncompressibility(int numIters, float dt) {
		auto n = this->numY;
		auto cp = this->density * this->h / dt;

		for (auto iter = 0; iter < numIters; iter++) {

			for (auto i = 1; i < this->numX - 1; i++) {
				for (auto j = 1; j < this->numY - 1; j++) {

					if (this->s[i * n + j] == 0.0)
						continue;

					//auto s = this->s[i * n + j];
					auto sx0 = this->s[(i - 1) * n + j];
					auto sx1 = this->s[(i + 1) * n + j];
					auto sy0 = this->s[i * n + j - 1];
					auto sy1 = this->s[i * n + j + 1];
					auto s = sx0 + sx1 + sy0 + sy1;
					if (s == 0.0)
						continue;

					auto div = this->u[(i + 1) * n + j] - this->u[i * n + j] +
						this->v[i * n + j + 1] - this->v[i * n + j];

					auto p = -div / s;
					//p *= scene.overRelaxation;
					p *= 1.9;
					this->p[i * n + j] += cp * p;

					this->u[i * n + j] -= sx0 * p;
					this->u[(i + 1) * n + j] += sx1 * p;
					this->v[i * n + j] -= sy0 * p;
					this->v[i * n + j + 1] += sy1 * p;
				}
			}
		}
	}

	void extrapolate() {
		auto n = this->numY;
		for (auto i = 0; i < this->numX; i++) {
			this->u[i * n + 0] = this->u[i * n + 1];
			this->u[i * n + this->numY - 1] = this->u[i * n + this->numY - 2];
		}
		for (auto j = 0; j < this->numY; j++) {
			this->v[0 * n + j] = this->v[1 * n + j];
			this->v[(this->numX - 1) * n + j] = this->v[(this->numX - 2) * n + j];
		}
	}

	float sampleField(float x, float y, int field) {
		auto n = this->numY;
		auto h = this->h;
		auto h1 = 1.0f / h;
		auto h2 = 0.5f * h;

		x = std::max(std::min(x, this->numX * h), h);
		y = std::max(std::min(y, this->numY * h), h);

		auto dx = 0.0f;
		auto dy = 0.0f;

		float* f = nullptr;

		switch (field) {
			case U_FIELD: f = &this->u[0]; dy = h2; break;
			case V_FIELD: f = &this->v[0]; dx = h2; break;
			case S_FIELD: f = &this->m[0]; dx = h2; dy = h2; break;
			default: break;
		}

		auto x0 = std::min((int)floorf((x - dx) * h1), this->numX - 1);
		auto tx = ((x - dx) - x0 * h) * h1;
		auto x1 = std::min(x0 + 1, this->numX - 1);

		auto y0 = std::min((int)floorf((y - dy) * h1), this->numY - 1);
		auto ty = ((y - dy) - y0 * h) * h1;
		auto y1 = std::min(y0 + 1, this->numY - 1);

		auto sx = 1.0f - tx;
		auto sy = 1.0f - ty;

		auto val = sx * sy * f[x0 * n + y0] +
			tx * sy * f[x1 * n + y0] +
			tx * ty * f[x1 * n + y1] +
			sx * ty * f[x0 * n + y1];

		return val;
	}

	float avgU(int i, int j) {
		auto n = this->numY;
		auto u = (this->u[i * n + j - 1] + this->u[i * n + j] +
			this->u[(i + 1) * n + j - 1] + this->u[(i + 1) * n + j]) * 0.25;
		return u;

	}

	float avgV(int i, int j) {
		auto n = this->numY;
		auto v = (this->v[(i - 1) * n + j] + this->v[i * n + j] +
			this->v[(i - 1) * n + j + 1] + this->v[i * n + j + 1]) * 0.25;
		return v;
	}

	void advectVel(float dt) {

		this->newU = this->u;
		this->newV = this->v;

		auto n = this->numY;
		auto h = this->h;
		auto h2 = 0.5 * h;

		for (auto i = 1; i < this->numX; i++) {
			for (auto j = 1; j < this->numY; j++) {

				//cnt++;

				// u component
				if (this->s[i * n + j] != 0.0 && this->s[(i - 1) * n + j] != 0.0 && j < this->numY - 1) {
					auto x = i * h;
					auto y = j * h + h2;
					auto u = this->u[i * n + j];
					auto v = this->avgV(i, j);
					//						auto v = this->sampleField(x,y, V_FIELD);
					x = x - dt * u;
					y = y - dt * v;
					u = this->sampleField(x, y, U_FIELD);
					this->newU[i * n + j] = u;
				}
				// v component
				if (this->s[i * n + j] != 0.0 && this->s[i * n + j - 1] != 0.0 && i < this->numX - 1) {
					auto x = i * h + h2;
					auto y = j * h;
					auto u = this->avgU(i, j);
					//						auto u = this->sampleField(x,y, U_FIELD);
					auto v = this->v[i * n + j];
					x = x - dt * u;
					y = y - dt * v;
					v = this->sampleField(x, y, V_FIELD);
					this->newV[i * n + j] = v;
				}
			}
		}

		this->u = this->newU;
		this->v = this->newV;
	}

	void advectSmoke(float dt) {

		this->newM = this->m;

		auto n = this->numY;
		auto h = this->h;
		auto h2 = 0.5 * h;

		for (auto i = 1; i < this->numX - 1; i++) {
			for (auto j = 1; j < this->numY - 1; j++) {

				if (this->s[i * n + j] != 0.0) {
					auto u = (this->u[i * n + j] + this->u[(i + 1) * n + j]) * 0.5;
					auto v = (this->v[i * n + j] + this->v[i * n + j + 1]) * 0.5;
					auto x = i * h + h2 - dt * u;
					auto y = j * h + h2 - dt * v;

					this->newM[i * n + j] = this->sampleField(x, y, S_FIELD);
				}
			}
		}
		this->m = this->newM;
	}

	// ----------------- end of simulator ------------------------------


	void simulate(float dt, float gravity, int numIters) {

		this->integrate(dt, gravity);

		for (auto& val: p) val = 0.0f;
		this->solveIncompressibility(numIters, dt);

		this->extrapolate();
		this->advectVel(dt);
		this->advectSmoke(dt);
	}

	float density;
	int numX;
	int numY;
	int numCells;
	float h = h;
	std::vector<float> u;
	std::vector<float> v;
	std::vector<float> newU;
	std::vector<float> newV;
	std::vector<float> p;
	std::vector<float> s;
	std::vector<float> m;
	std::vector<float> newM;
};
