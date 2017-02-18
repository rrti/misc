// best compiled with -DWINDOWED -DTHREADED
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <sys/stat.h>

#ifdef WINDOWED
#include <SDL/SDL.h>
#else
#include <SDL/SDL_timer.h>
#endif

#ifdef THREADED
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#endif

typedef unsigned int uint;
typedef const uint cuint;
typedef const float cfloat;
typedef const double cdouble;

using std::cout;
using std::endl;

enum ColorMode {CM_DISCRETE, CM_CONTINUOUS};
enum ComputeMode {COMPUTE_IMAGE, COMPUTE_AREA};

const static float FRAND_MAX = float(RAND_MAX);



/**
 * Represents a type independent triplet
 * and is meant for storing RGB values.
 */
template<typename ColorType> struct RGBColor {
	ColorType r;
	ColorType g;
	ColorType b;

	// default
	RGBColor(): r(ColorType()), g(ColorType()), b(ColorType()) {
	}

	// color
	RGBColor(const ColorType& x, const ColorType& y, const ColorType& z):
		r(x), g(y), b(z) {
	}

	// gray
	RGBColor(const ColorType& x): r(x), g(x), b(x) {
	}
};



#ifdef WINDOWED
struct SDLWindow {
	SDL_Surface* s;

	SDLWindow(): s(0) {
	}

	void InitSDL(cuint w, cuint h, const char* caption) {
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
		SDL_WM_SetCaption(caption, NULL);

		s = SDL_SetVideoMode(w, h, 0, SDL_HWSURFACE | SDL_DOUBLEBUF);
	}

	void FlipBuffer() {
		if (s) {
			SDL_Flip(s);
		}
	}

	inline void DrawRect(cuint rx, cuint ry, cuint rw, cuint rh) {
		for (uint i = rx; i < rx + rw; i++) { DrawPixel(i,       ry,      0xFFFFFFFF); }
		for (uint i = rx; i < rx + rw; i++) { DrawPixel(i,       ry + rh, 0xFFFFFFFF); }

		for (uint i = ry; i < ry + rh; i++) { DrawPixel(rx,      i,       0xFFFFFFFF); }
		for (uint i = ry; i < ry + rh; i++) { DrawPixel(rx + rw, i,       0xFFFFFFFF); }
	}

	inline void DrawPixel(cuint x, cuint y, const RGBColor<uint>& c) {
		if (s) {
			DrawPixel(x, y, ((c.r << 16) | (c.g << 8) | (c.b << 0)));
		}
	}

	void DrawPixel(cuint x, cuint y, const Uint32 c) {
		if (x >= uint(s->w) || y >= uint(s->h)) {
			return;
		}

		cuint bpp = s->format->BytesPerPixel;
		Uint8* p = (Uint8*) s->pixels + y * s->pitch + x * bpp;

		switch (bpp) {
			case 1: {
				(*(Uint8*) p) = c;
			} break;
			case 2: {
				(*(Uint16*) p) = c;
			} break;
			case 3: {
				p = (Uint8*) p;

				if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
					p[0] = (c >> 16) & 0xFF;
					p[1] = (c >>  8) & 0xFF;
					p[2] = (c >>  0) & 0xFF;
				} else {
					p[0] = (c >>  0) & 0xFF;
					p[1] = (c >>  8) & 0xFF;
					p[2] = (c >> 16) & 0xFF;
				}
			} break;
			case 4: {
				(*(Uint32*) p) = c;
			} break;
		}
	}
};
#endif



struct RNG {
	RNG() {
		static bool inited = false;

		if (!inited) {
			inited = true;
			srandom(time(0x0));
		}
	}

	// returns a uniformly distributed number in the range [0.0f, 1.0f)
	float FUniformRandom() const {
		#ifdef THREADED
		boost::mutex::scoped_lock l(rngMutex);
		#endif

		// never allow (r / d) to be 1
		cuint r = random();
		cfloat d = (r == RAND_MAX)? FRAND_MAX + 1.0f: FRAND_MAX;

		return (r / d);
	}

	uint IUniformRandom(uint range) const {
		return uint(FUniformRandom() * range);
	}

	#ifdef THREADED
	// needs to be mutable because FUR() is const
	mutable boost::mutex rngMutex;
	#endif
};



struct ColorWeights {
	ColorWeights(): rw(0.299), gw(0.587), bw(0.114) {
		rw = 0.835002; // 0.858141
		gw = 0.114957; // 0.088806
		bw = 0.050041; // 0.053054
	}

	void AddWeights(double r, double g, double b, bool n = true) {
		rw += r;
		gw += g;
		bw += b;

		if (n) {
			NormalizeWeights();
		}
	}

	void NormalizeWeights() {
		cdouble s = (1.0 / (rw + gw + bw));

		rw *= s;
		gw *= s;
		bw *= s;
	}

	void PrintWeights() {
		cout << "[ColorWeights::" << __FUNCTION__ << "]" << endl;
		cout << "\trw: " << rw;
		cout << "\tgw: " << gw;
		cout << "\tbw: " << bw;
		cout << endl;
	}

	double rw;
	double gw;
	double bw;
};



struct MandelbrotPoint {
	MandelbrotPoint(double _x = 0.0, double _y = 0.0):
		x(_x), xn(0.0), xnSq(0.0),
		y(_y), yn(0.0), ynSq(0.0),
		inSet(false), n(0) {
	}

	inline void Init(double _x, double _y) {
		x = _x; xn = 0.0; xnSq = 0.0;
		y = _y; yn = 0.0; ynSq = 0.0;
		inSet = false;
		n = 0;
	}

	// execute a single iteration
	// of the complex recurrence
	inline void Iterate() {
		yn = (2.0 * xn * yn) + y;
		xn = (xnSq - ynSq) + x;

		xnSq = xn * xn;
		ynSq = yn * yn;

		n++;
	}

	// the basic escape-time test
	inline bool IsInSet(cdouble sqEscRad, cuint nMax) {
		cdouble ySq = y * y;
		cdouble p = ((x - 0.25) * (x - 0.25)) + ySq;
		cdouble q = sqrt(p);

		// test if point in large center cardioid
		if (x <= ((q - 2.0 * p) + 0.25)) {
			return (inSet = true);
		}

		// test if point in circle left of center cardioid
		if ((((x + 1.0) * (x + 1.0)) + ySq) <= 0.0625) {
			return (inSet = true);
		}

		while ((xnSq + ynSq) <= sqEscRad && n < nMax) {
			Iterate();
		}

		return (inSet = (n == nMax));
	}

	inline double GetLuminance(const ColorMode cm, cdouble rlogExp) {
		if (cm == CM_CONTINUOUS) {
			Iterate(); Iterate();
			Iterate(); Iterate();

			// assumes an escape radius of 2 and exponent of 2,
			// but we're not rendering any other fractal anyway
			//
			// log(x^n) is (n * log(x)), sqrt(x) is (x^0.5)
			// return (n + -1.0 - (log(log(sqrt(xnSq + ynSq))) / logExp));
			return (n + -1.0 - (log(0.5 * log(xnSq + ynSq)) * rlogExp));
		}

		return 0.0;
	}

	inline RGBColor<uint> GetColor(const ColorMode cm, cdouble lum, const ColorWeights* cw) const {
		RGBColor<uint> c;

		if (cm == CM_CONTINUOUS) {
			c.r = std::max(0U, std::min(uint(lum * cw->rw * 255), 255U));
			c.g = std::max(0U, std::min(uint(lum * cw->gw * 255), 255U));
			c.b = std::max(0U, std::min(uint(lum * cw->bw * 255), 255U));
		} else {
			c.r = std::min(uint(n * cw->rw * 5), 255U);
			c.g = std::min(uint(n * cw->gw * 7), 255U);
			c.b = std::min(uint(n * cw->bw * 3), 255U);
		}

		return c;
	}

	double x, xn, xnSq;
	double y, yn, ynSq;

	// true if this point belongs to the set
	bool inSet;

	// iteration count for this point
	uint n;
};



struct Pixel {
	Pixel(): l(0.0) {
	}

	// the complex point this pixel corresponds to
	MandelbrotPoint p;

	// color
	RGBColor<uint> c;

	// luminance
	double l;
};



struct ZoomRectangle {
	ZoomRectangle(cuint sw, cuint sh) {
		Init(sw, sh);
	}

	void Init(uint sw, uint sh) {
		// no point in making dx and dy smaller than the level
		// of detail that image resolution is able to capture
		// in discrete pixels (for rendering at least)
		xmin = -2.5, xmax = 1.5, xrange = xmax - xmin, dx = xrange / sw;
		ymin = -1.5, ymax = 1.5, yrange = ymax - ymin, dy = yrange / sh;
		ar = sw / double(sh);
	}

	// derive the zooming frame given the top-left corner
	// in screen-space of the drawn rectangle along with
	// its width and height (the frame is always forced
	// to the same aspect ratio as the screen)
	void SetBounds(uint tlx, uint tly, uint rw, uint rh) {
		// the window cannot be resized anyway
		static cdouble sw = xrange / dx;
		static cdouble sh = yrange / dy;

		cdouble nxrange = (rw / sw) * xrange;
	//	cdouble nyrange = (rh / sh) * yrange;
		cdouble nyrange = nxrange / ar;
		cdouble nxmin = xmin + (tlx / sw) * xrange;
		cdouble nymax = ymax - (tly / sh) * yrange;

		cdouble nxmax = nxmin + nxrange;
		cdouble nymin = nymax - nyrange;
		cdouble ndx = (nxrange / sw);
		cdouble ndy = (nyrange / sh);

		xmin = nxmin; xmax = nxmax; xrange = nxrange; dx = ndx;
		ymin = nymin; ymax = nymax; yrange = nyrange; dy = ndy;
	}

	double xmin, xmax, xrange, dx;
	double ymin, ymax, yrange, dy;
	double ar;
};



struct MandelbrotSet {
private:
	uint swidth;
	uint sheight;

	ColorMode cm;
	ColorWeights cw;
	const RGBColor<uint> setColor;
	RNG rng;

	// functions as a stack of rectangles;
	// current zoom level is zrLst.front()
	std::list<ZoomRectangle> zrLst;

	#ifdef WINDOWED
	SDLWindow win;
	#endif

	#ifdef THREADED
	std::vector<boost::thread*> threads;
	#endif

	std::vector< std::vector<Pixel> > image;

public:
	MandelbrotSet(cuint sw, cuint sh, ColorMode c, cuint t, bool w): setColor(255, 255, 255) {
		swidth = sw;
		sheight = sh;
		cm = c;

		zrLst.push_front(ZoomRectangle(sw, sh));

		#ifdef WINDOWED
		if (w) {
			win.InitSDL(sw, sh, "Mandelbrot Set Explorer");
		}
		#endif

		#ifdef THREADED
		threads.resize(t, 0x0);
		#endif

		image.resize(sw, std::vector<Pixel>(sh, Pixel()));
	}



	#ifdef THREADED
	// fire the threads
	bool SpawnThreads(ComputeMode m, uint jobSize, uint maxIters, std::vector<uint>& ptsIn, std::vector<uint>& ptsOut) {
		cuint numThreads = threads.size();
		cuint thrJobSize = jobSize / numThreads;

		// for now, numThreads must evenly divide jobSize
		if (jobSize != (thrJobSize * numThreads)) {
			return false;
		}

		for (uint threadID = 0; threadID < numThreads; threadID++) {
			if (m == COMPUTE_IMAGE) {
				threads[threadID] = new boost::thread(boost::bind(
					&MandelbrotSet::ComputeImage, this,
					maxIters, threadID, thrJobSize,
					&ptsIn[threadID], &ptsOut[threadID]
				));
				continue;
			}

			if (m == COMPUTE_AREA) {
				threads[threadID] = new boost::thread(boost::bind(
					&MandelbrotSet::ComputeArea, this,
					maxIters, threadID, thrJobSize,
					&ptsIn[threadID], &ptsOut[threadID]
				));
				continue;
			}
		}

		return true;
	}

	// gather the per-thread results
	void JoinThreads(const std::vector<uint>& ptsIn, const std::vector<uint>& ptsOut, uint* tPointsIn, uint* tPointsOut) {
		for (uint threadID = 0; threadID < threads.size(); threadID++) {
			threads[threadID]->join();
			delete threads[threadID];
			threads[threadID] = 0x0;

			(*tPointsIn) += ptsIn[threadID];
			(*tPointsOut) += ptsOut[threadID];
		}
	}
	#endif



	// the basic escape-time algorithm
	void ComputeImage(cuint maxIters, cuint tID, cuint tCols, uint* ptsIn, uint* ptsOut) {
		// nDistrib[i] is the total number of points
		// that were identified as escaping after <i>
		// iterations
		// to use this, add "nDistrib[mp.n] += 1" to
		// the loop and then sum over the per-thread
		// distributions
		// std::vector<uint> nDistrib(maxIters + 4, 0);
		//
		cdouble setExp   = 2.0;
		cdouble rlogExp  = 1.0 / log(setExp);
		cdouble sqEscRad = 2.0 * 2.0;

		RGBColor<uint> co;
		MandelbrotPoint mp;

		cuint wMin = tID * tCols, wMax = wMin + tCols;
		cuint hMin =           0, hMax = sheight;

		const ZoomRectangle& zr = zrLst.front();
		double x = zr.xmin + (wMin * zr.dx);
		double y = zr.ymax;
		double lu = 0.0;

		// w and h are the screen-coors of the projected pixel,
		// x and y are its (real, imaginary) coordinates in the
		// complex plane
		// (w=0, h=0) maps to (-xmax, ymax)
		// (SDL coordinate origin is in top-left window corner!)
		// compute in column-major order
		for (uint w = wMin; w < wMax; w++) {
			y = zr.ymax;

			for (uint h = hMin; h < hMax; h++) {
				mp.Init(x, y);

				if (mp.IsInSet(sqEscRad, maxIters)) {
					(*ptsIn) += 1;

					co = setColor;
					lu = maxIters;

					// lu = 1.0 / sqrt(mp.xnSq + mp.ynSq + 0.01);
					// co = mp.GetColor(cm, lum, &cw);
				} else {
					(*ptsOut) += 1;

					lu = mp.GetLuminance(cm, rlogExp);
					co = mp.GetColor(cm, lu, &cw);
				}

				image[w][h].p = mp;
				image[w][h].c = co;
				image[w][h].l = lu;
				y -= zr.dy;
			}

			x += zr.dx;
		}
	}

	bool ComputeImage(cuint maxIters, bool verbose) {
		uint tPointsIn = 0;
		uint tPointsOut = 0;
		uint ticks = SDL_GetTicks();

		#ifdef THREADED
		cuint numThreads = threads.size();

		std::vector<uint> ptsIn; ptsIn.resize(numThreads, 0);
		std::vector<uint> ptsOut; ptsOut.resize(numThreads, 0);

		if (SpawnThreads(COMPUTE_IMAGE, swidth, maxIters, ptsIn, ptsOut)) {
			JoinThreads(ptsIn, ptsOut, &tPointsIn, &tPointsOut);
		} else {
			return false;
		}

		#else
		ComputeImage(maxIters, 0, swidth, &tPointsIn, &tPointsOut);
		#endif

		const ZoomRectangle& zr = zrLst.front();

		// if pointsOut is 0, result is infinity (not an FPEX)
		cdouble inOutRatio = tPointsIn / double(tPointsOut);
		cdouble inTotRatio = tPointsIn / double(tPointsIn + tPointsOut);

		if (verbose) {
			cout << "[MandelbrotSet::" << __FUNCTION__ << "]" << endl;
			cout << "\tpoints drawn total:     " << (tPointsIn + tPointsOut) << endl;
			cout << "\tpoints inside set:      " << tPointsIn << endl;
			cout << "\tpoints outside set:     " << tPointsOut << endl;
			cout << "\tinside / outside ratio: " << inOutRatio << endl;
			cout << "\tinside / total ratio:   " << inTotRatio << endl;
			cout << "\tdrawn set area:         " << (zr.xrange * zr.yrange * inTotRatio) << endl;
			cout << "\timage computation time: " << (SDL_GetTicks() - ticks) << "ms" << endl;
			cout << "\titeration limit:        " << maxIters << endl;
		}

		return true;
	}



	void UpdateImageColors() {
		cuint size = swidth * sheight;

		for (uint i = 0; i < size; i++) {
			cuint w = i % swidth;
			cuint h = i / swidth;

			Pixel& pxl = image[w][h];

			const MandelbrotPoint& mp = pxl.p;
			const bool inSet = mp.inSet;
			const RGBColor<uint> c = (inSet)? setColor: mp.GetColor(cm, pxl.l, &cw);

			pxl.c = c;
		}
	}

	void RefreshImage() {
		#ifdef WINDOWED
		cuint size = swidth * sheight;

		for (uint i = 0; i < size; i++) {
			cuint w = i % swidth;
			cuint h = i / swidth;

			win.DrawPixel(w, h, image[w][h].c);
		}
		#endif
	}



	// the basic hit-miss MC algorithm
	void ComputeArea(cuint maxIters, cuint tID, cuint tPts, uint* ptsIn, uint* ptsOut) {
		cdouble escRad   = 2.0;
		cdouble sqEscRad = escRad * escRad;

		MandelbrotPoint mp;
		const ZoomRectangle& zr = zrLst.front();

		cuint minSamplePt = tID * tPts;
		cuint maxSamplePt = minSamplePt + tPts;
		for (uint i = minSamplePt; i < maxSamplePt; i++) {
			cdouble rx = zr.xmin + (rng.FUniformRandom() * zr.xrange);
			cdouble ry = zr.ymin + (rng.FUniformRandom() * zr.yrange);

			mp.Init(rx, ry);

			if (mp.IsInSet(sqEscRad, maxIters)) {
				(*ptsIn)++;
			} else {
				(*ptsOut)++;
			}
		}
	}

	bool ComputeArea(cuint numSamplePoints, cuint maxIters, double* area, bool verbose) {
		uint tPointsIn = 0;
		uint tPointsOut = 0;

		#ifdef THREADED
		cuint numThreads = threads.size();

		std::vector<uint> ptsIn; ptsIn.resize(numThreads, 0);
		std::vector<uint> ptsOut; ptsOut.resize(numThreads, 0);

		if (SpawnThreads(COMPUTE_AREA, numSamplePoints, maxIters, ptsIn, ptsOut)) {
			JoinThreads(ptsIn, ptsOut, &tPointsIn, &tPointsOut);
		} else {
			return false;
		}

		#else
		ComputeArea(maxIters, 0, numSamplePoints, &tPointsIn, &tPointsOut);
		#endif

		const ZoomRectangle& zr = zrLst.front();

		// estimate the area
		cdouble inOutRatio = (tPointsIn / double(tPointsOut));
		cdouble inTotRatio = (tPointsIn / double(numSamplePoints));
		cdouble sampleArea = (zr.xrange * zr.yrange) * inTotRatio;

		if (verbose) {
			cout << "[MandelbrotSet::" << __FUNCTION__ << "]" << endl;
			cout << "\tpoints sampled total:   " << numSamplePoints << endl;
			cout << "\tpoints inside set:      " << tPointsIn << endl;
			cout << "\tpoints outside set:     " << tPointsOut << endl;
			cout << "\tinside / outside ratio: " << inOutRatio << endl;
			cout << "\tinside / total ratio:   " << inTotRatio << endl;
			cout << "\tsampled set area:       " << sampleArea << endl;
		}

		*area = sampleArea;
		return true;
	}



	void WriteImage(const std::string& fname) const {
		cout << "[MandelbrotSet::" << __FUNCTION__ << "]" << endl;

		std::ofstream f(fname.c_str(), std::ios::out);
		std::stringstream ss;

		ss << "P3" << endl;
		ss << "## Mandelbrot Set" << endl;
		ss << swidth << " " << sheight << endl;
		ss << "255" << endl;

		// output in row-major order (all
		// columns in row 0, then all in
		// row 1, etc)
		cuint size = swidth * sheight;

		for (uint i = 0; i < size; i++) {
			cuint w = i % swidth;
			cuint h = i / swidth;
			const RGBColor<uint>& c = image[w][h].c;

			ss << c.r << " " << c.g << " " << c.b << endl;
		}

		f << ss.str();
		f.close();

		cout << "\tPPM image written to " << fname << endl;
	}



	#ifdef WINDOWED
	void ZoomIn(cuint xpress, cuint ypress, cuint xrelease, cuint yrelease, cuint iters) {
		// get the top-left corner in default SDL screen-space
		cuint tlx = std::min(xpress, xrelease);
		cuint tly = std::min(ypress, yrelease);
		// get the width and height in default SDL screen-space
		cuint rw = abs(xpress - xrelease);
		cuint rh = abs(ypress - yrelease);

		ZoomRectangle zr = zrLst.front();
		zr.SetBounds(tlx, tly, rw, rh);
		zrLst.push_front(zr);

		ComputeImage(iters, true);
		RefreshImage();
	}

	void ZoomOut(cuint iters) {
		if (zrLst.size() > 1) {
			zrLst.pop_front();

			ComputeImage(iters, true);
			RefreshImage();
		}
	}

	void ResetZoom(cuint iters) {
		// reset the zoom level
		ZoomRectangle zr = zrLst.back();
		zrLst.clear();
		zrLst.push_front(zr);

		ComputeImage(iters, true);
		RefreshImage();
	}
	#endif



	void Show() {
		#ifdef WINDOWED
		cout << "[MandelbrotSet::" << __FUNCTION__ << "] press 'q' to quit" << endl;

		RefreshImage();

		bool quit   =    0, zoom     = 0;
		uint xpress =    0, xrelease = 0;
		uint ypress =    0, yrelease = 0;
		uint iters  = 1000;

		while (!quit) {
			SDL_Event e;
			SDL_WaitEvent(&e);

			switch (e.type) {
				case SDL_KEYDOWN: {
					switch (e.key.keysym.sym) {
						case SDLK_q: {
							quit = true;
						} break;

						case SDLK_w: {
							WriteImage("screenshot.ppm");
						} break;

						case SDLK_i: {
							iters <<= 1;

							ComputeImage(iters, true);
							RefreshImage();
						} break;
						case SDLK_j: {
							iters >>= 1;

							ComputeImage(iters, true);
							RefreshImage();
						} break;

						case SDLK_s: {
							ResetZoom(iters);
						} break;
						case SDLK_o: {
							ZoomOut(iters);
						} break;

						case SDLK_r: {
							cw.AddWeights(0.05, 0.0, 0.0);
							cw.PrintWeights();

							UpdateImageColors();
							RefreshImage();
						} break;
						case SDLK_g: {
							cw.AddWeights(0.0, 0.05, 0.0);
							cw.PrintWeights();

							UpdateImageColors();
							RefreshImage();
						} break;
						case SDLK_b: {
							cw.AddWeights(0.0, 0.0, 0.05);
							cw.PrintWeights();

							UpdateImageColors();
							RefreshImage();
						} break;

						default: {
						} break;
					}
				} break;

				case SDL_MOUSEBUTTONDOWN: {
					zoom   =  1;
					xpress =  e.motion.x;
					ypress =  e.motion.y;
				} break;
				case SDL_MOUSEBUTTONUP: {
					zoom     =  0;
					xrelease =  e.motion.x;
					yrelease =  e.motion.y;

					ZoomIn(xpress, ypress, xrelease, yrelease, iters);
				} break;
				case SDL_MOUSEMOTION: {
					if (zoom) {
						// get the top-left corner in default SDL screen-space
						cuint tlx = std::min(xpress, uint(e.motion.x));
						cuint tly = std::min(ypress, uint(e.motion.y));
						// get the width and height in default SDL screen-space
						cuint rw = abs(xpress - e.motion.x);
						cuint rh = abs(ypress - e.motion.y);

						// this is too slow
						// RefreshImage();

						// draw the selection rectangle
						win.DrawRect(tlx, tly, rw, rh);
					}
				} break;
			}

			win.FlipBuffer();
		}

		SDL_Quit();
		#endif
	}
};



struct GraphPoint {
	GraphPoint(uint _t, uint _s, uint _i, double _a, double _d):
		t(_t), s(_s), i(_i), a(_a), d(_d) {
	}

	GraphPoint& operator += (const GraphPoint& gp) {
		t += gp.t;
		s += gp.s;
		i += gp.i;
		a += gp.a;
		d += gp.d;
		return *this;
	}

	uint t;			// computation time for A(s, i) in msecs
	uint s;			// number of sample points tested
	uint i;			// number of iterations per point

	double a;		// best-estimated area A(s, i)
	double d;		// delta between <a> and exact area
};

void WriteBatchData(const char* dir, const std::vector<GraphPoint>& v, cuint n) {
	mkdir(dir, S_IRWXU | S_IRWXG);

	// char f0name[64] = {'\0'}; snprintf(f0name, 63, "%s/batch%u-s-i-d.dat", dir, n);
	// char f1name[64] = {'\0'}; snprintf(f1name, 63, "%s/batch%u-s-i-t.dat", dir, n);
	//
	// std::ofstream f0(f0name, std::ios::out);
	// std::ofstream f1(f1name, std::ios::out);
	char f0name[64] = {'\0'}; snprintf(f0name, 63, "%s/batchSeq-s-i-d.dat", dir);
	char f1name[64] = {'\0'}; snprintf(f1name, 63, "%s/batchSeq-s-i-t.dat", dir);

	std::ofstream f0(f0name, std::ios::app);
	std::ofstream f1(f1name, std::ios::app);
	std::stringstream ss0;
	std::stringstream ss1;

	std::vector<GraphPoint>::const_iterator it;

	for (it = v.begin(); it != v.end(); it++) {
		const GraphPoint& p = *it;
		ss0 << p.s << "\t" << p.i << "\t" << p.d << endl;
		ss1 << p.s << "\t" << p.i << "\t" << p.t << endl;
	}

	f0 << ss0.str(); f0.close();
	f1 << ss1.str(); f1.close();
}

void WriteBatchAvgData(const char* dir, const std::vector<GraphPoint>& v, cuint numBatches) {
	char f0name[64] = {'\0'}; snprintf(f0name, 63, "%s/batchSeqAvg-s-i-d.dat", dir);
	char f1name[64] = {'\0'}; snprintf(f1name, 63, "%s/batchSeqAvg-s-i-t.dat", dir);

	std::ofstream f0(f0name, std::ios::out);
	std::ofstream f1(f1name, std::ios::out);
	std::stringstream ss0;
	std::stringstream ss1;

	cdouble d = double(numBatches);
	std::vector<GraphPoint>::const_iterator it;

	for (it = v.begin(); it != v.end(); it++) {
		const GraphPoint& p = *it;
		ss0 << (p.s / d) << "\t" << (p.i / d) << "\t" << (p.d / d) << endl;
		ss1 << (p.s / d) << "\t" << (p.i / d) << "\t" << (p.t / d) << endl;
	}

	f0 << ss0.str(); f0.close();
	f1 << ss1.str(); f1.close();
}



// run a single batch of Monte-Carlo simulations
// (ie., compute A(s, i) for a given interval of
// <s> and <i>)
std::vector<GraphPoint> RunBatch(MandelbrotSet& m, cuint maxPts, cuint maxIts, cdouble exArea, GraphPoint* bestEstim) {
	double currEstimArea = 0.0, bestEstimArea = 999.0;
	double currAreaDelta = 0.0, bestAreaDelta = 999.0;

	// set (somewhat) tractable step-sizes for <s> and <i>
	// (if <s> is fex. 10000, then M's area estimates will
	// be calculated for <s> equal 1000, 2000, ..., 10000)
	cuint sStep = maxPts / 10;
	cuint iStep = maxIts / 10;

	uint t0, t1, bt = 0;

	// stores the best A(s, i) estimate for _this_ batch
	GraphPoint batchBestEstim(0, 0, 0, bestEstimArea, bestAreaDelta);
	// stores all A(s, i) estimates for this batch
	std::vector<GraphPoint> v;

	for (uint s = sStep; s <= maxPts; s += sStep) {
		for (uint i = iStep; i <= maxIts; i += iStep) {
			t0 = SDL_GetTicks(); // clock();

			if (m.ComputeArea(s, i, &currEstimArea, false)) {
				t1 = SDL_GetTicks(); // clock();
				bt += (t1 - t0);

				currAreaDelta = (currEstimArea - exArea);
				currAreaDelta = std::max(currAreaDelta, -currAreaDelta);

				if (currAreaDelta < bestAreaDelta) {
					bestAreaDelta = currAreaDelta;
					bestEstimArea = currEstimArea;

					batchBestEstim.t = t1 - t0;
					batchBestEstim.s = s;
					batchBestEstim.i = i;
					batchBestEstim.a = bestEstimArea;
					batchBestEstim.d = bestAreaDelta;
				}

				GraphPoint p(t1 - t0, s, i, currEstimArea, currAreaDelta);
				v.push_back(p);
			}
		}
	}

	cout << "[" << __FUNCTION__ << "]" << endl;
	cout << "\tbatch computation time: " << bt << "ms (" << (bt / 1000) << "s)" << endl;
	cout << "\tbest area estimate:     " << batchBestEstim.a << endl;
	cout << "\tbest area difference:   " << (exArea - batchBestEstim.a) << endl;

	// save the best estimate across _all_ batches
	if (batchBestEstim.d < bestEstim->d) {
		*bestEstim = batchBestEstim;
	}

	return v;
}

// run a sequence of batches so their individual
// results can be averaged; this is needed since
// A(s=constant, i=constant) can vary for two or
// more invokations of the MC simulation
void RunBatches(MandelbrotSet& m, cuint numBatches, cdouble exArea, cuint maxPts, cuint maxIts, GraphPoint* bestEstim) {
	std::vector<GraphPoint> sumVec;
	uint t0, t1, tt = 0;

	for (uint i = 0; i < numBatches; i++) {
		cout << "[" << __FUNCTION__ << "] batch number: " << i << endl;

		t0 = SDL_GetTicks();

		// get the A(s=variable, i=variable)
		// estimates for the current batch
		std::vector<GraphPoint> v = RunBatch(m, maxPts, maxIts, exArea, bestEstim);

		t1 = SDL_GetTicks();
		tt += (t1 - t0);

		// accumulate the batch results
		if (sumVec.empty()) {
			for (uint i = 0; i < v.size(); i++) {
				sumVec.push_back(v[i]);
			}
		} else {
			for (uint i = 0; i < v.size(); i++) {
				sumVec[i] += v[i];
			}
		}

		WriteBatchData("data", v, i);
	}

	WriteBatchAvgData("data", sumVec, numBatches);

	cout << "[" << __FUNCTION__ << "] cumulative time for all batches: " << tt << "ms (" << (tt / 1000) << "s)" << endl;
}



int main(int argc, char** argv) {
	uint xResolution     =   800;
	uint yResolution     =   600;
	uint maxIterations   =  1000;
	uint numSamplePoints = 10000;
	uint numThreads      =     1;
	uint numBatches      =     1;

	bool computeImage    =  true;
	bool outputImage     = false;
	bool computeArea     =  true;
	bool contColors      =  true;
	bool showWindow      =  true;
	bool batchMode       = false;

	for (int i = 0; i < argc; i++) {
		const std::string s(argv[i]);
		const bool moreArgs = (i < argc - 1);

		if (s == "--xres" && moreArgs) { xResolution     =   atoi(argv[i + 1]); continue; }
		if (s == "--yres" && moreArgs) { yResolution     =   atoi(argv[i + 1]); continue; }
		if (s == "--iter" && moreArgs) { maxIterations   =   atoi(argv[i + 1]); continue; }
		if (s == "--samp" && moreArgs) { numSamplePoints =   atoi(argv[i + 1]); continue; }
		if (s == "--tcnt" && moreArgs) { numThreads      =   atoi(argv[i + 1]); continue; }
		if (s == "--bcnt" && moreArgs) { numBatches      =   atoi(argv[i + 1]); continue; }
		if (s == "--cimg" && moreArgs) { computeImage    = !!atoi(argv[i + 1]); continue; }
		if (s == "--outp" && moreArgs) { outputImage     = !!atoi(argv[i + 1]); continue; }
		if (s == "--area" && moreArgs) { computeArea     = !!atoi(argv[i + 1]); continue; }
		if (s == "--ccol" && moreArgs) { contColors      = !!atoi(argv[i + 1]); continue; }
		if (s == "--wind" && moreArgs) { showWindow      = !!atoi(argv[i + 1]); continue; }
		if (s == "--bmod" && moreArgs) { batchMode       = !!atoi(argv[i + 1]); continue; }
	}

	MandelbrotSet m(xResolution, yResolution, (contColors? CM_CONTINUOUS: CM_DISCRETE), numThreads, showWindow);

	if (computeImage) {
		m.ComputeImage(maxIterations, true);
	}

	if (computeImage && outputImage) {
		m.WriteImage("output.ppm");
	}

	if (computeArea) {
		cdouble exactArea = sqrt((6.0 * M_PI) - 1.0) - M_E;
		GraphPoint bestEstim(0, 0, 0, 999.0, 999.0);

		if (batchMode) {
			/// usage example: "./mandelbrot_fractal_gen --cimg 0 --bmod 1 --bcnt 2 --tcnt 8 --wind 0 --samp 500000 --iter 10000"
			/// the arguments specify batch-mode=1, batches=2, threads=8, 500K sample points per MC sim, 10K iterations per point
			RunBatches(m, numBatches, exactArea, numSamplePoints, maxIterations, &bestEstim);
		} else {
			m.ComputeArea(numSamplePoints, maxIterations, &bestEstim.a, true);
		}

		cout << "[" << __FUNCTION__ << "]"    << endl;
		cout << "\texact area:              " << exactArea << endl;
		cout << "\tbest estimated area:     " << bestEstim.a << endl;
		cout << "\testimation difference:   " << (exactArea - bestEstim.a) << endl;
		cout << "\tnumber of sample points: " << bestEstim.s << endl;
		cout << "\tnumber of iterations:    " << bestEstim.i << endl;
		cout << "\ttime for best estimate:  " << bestEstim.t << "ms" << endl;
	}

	if (computeImage && showWindow) {
		m.Show();
	}

	return 0;
}

