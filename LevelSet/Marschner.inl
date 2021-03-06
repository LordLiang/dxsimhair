//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2010-2011, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of Image Engine Design nor the names of any
//       other contributors to this software may be used to endorse or
//       promote products derived from this software without specific prior
//       written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "IEMath.h"
#include <cmath>
#include <climits>

namespace IECore
{
	namespace
	{
		template <class T>
		T clamp(T val, T low, T up)
		{
			if (val > up) return up;
			if (val < low) return low;
			return val;
		}
	}


	template < typename T >
	MarschnerBCSDF<T>::MarschnerBCSDF(
		V refraction, const T &absorption, V eccentricity,
		V shiftR, V shiftTT, V shiftTRT,
		V widthR, V widthTT, V widthTRT,
		V glint,
		V causticWidth, V causticFade, V causticLimit) :
		m_refraction(refraction),
		m_absorption(absorption),
		m_eccentricity(eccentricity),
		m_shiftR(shiftR), m_shiftTT(shiftTT), m_shiftTRT(shiftTRT),
		m_widthR(widthR), m_widthTT(widthTT), m_widthTRT(widthTRT),
		m_glint(glint),
		m_causticWidth(causticWidth), m_causticFade(causticFade), m_causticLimit(causticLimit)
	{
	}

	template < typename T >
	MarschnerBCSDF<T>::~MarschnerBCSDF()
	{
	}

	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::marschnerM(V shift, V width, V normWidth, V x) const
	{
		//                   (deg to radians)
		V norm = 1.0 / (((normWidth / 180.0)* M_PI) * sqrt(2.0 * M_PI));
		V3 coefficients = gaussianPDF(shift, width);
		return (gaussian(coefficients[0], coefficients[1], coefficients[2], x) / coefficients[0]) * norm;
	}

	/// \TODO Migrate to boost or some other library functions.
	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::gaussian(V a, V b, V c, V x) const
	{
		V o = (x - b);
		return a * exp(-(o * o) / (2.0 * c * c));
	}

	/// \TODO Migrate to boost or some other library functions.
	template < typename T >
	typename MarschnerBCSDF<T>::V3 MarschnerBCSDF<T>::gaussianPDF(V mu, V sigma) const
	{
		V3 values;
		values[0] = 1.0 / (sigma * sqrt(2.0 * M_PI));
		values[1] = mu;
		values[2] = sigma;
		return values;
	}

	//template < typename T >
	//typename MarschnerBCSDF<T>::V2 MarschnerBCSDF<T>::computeLocalVector(Imath::M44f hairSystem, const MarschnerBCSDF<T>::V3 &pos) const
	//{
	//	V3 posLocal;
	//	hairSystem.multVecMatrix(pos, posLocal);
	//	return computeLocalVector(posLocal);
	//}

	//template < typename T >
	//typename MarschnerBCSDF<T>::V2 MarschnerBCSDF<T>::computeLocalVector(const MarschnerBCSDF<T>::V3 &pos) const
	//{
	//	// convert euclidian vector to spherical coordinates 
	//	V2 res = m_sphericalConverter.transform(pos);
	//	// Mapping ranges on spherical coordinate X from [0,pi] to [pi/2,-pi/2] according to convention addopted by [1] on section 2.2.
	//	res[0] = M_PI / 2.0 - res[0];
	//	return res;
	//}

	// converts a given refraction index ( eta ) to work on a 2d plane that is a cross section of the hair.
	// the theta parameter is the angle from the incident light to the cross section plane.
	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::bravaisIndex(V theta, V eta)
	{
		V sinTheta = sin(theta);
		return sqrt(eta*eta - sinTheta*sinTheta) / cos(theta);
	}

	// Computes reflectance fresnel with different index (eta) of refractions for perpendicular and parallel polarized light.
	// Assumes the source media is vaccuum ( n = 1 ).
	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::fresnel(V incidenceAngle, V etaPerp, V etaParal, bool invert)
	{
		V n1, n2;

		V rPerp = 1.0;
		V rParal = 1.0;

		V angle = fabs(incidenceAngle);
		if (angle > M_PI / 2.0)
		{
			angle = M_PI - angle;
		}

		V cosA = cos(angle);

		n1 = (invert) ? etaPerp : 1.0;
		n2 = (invert) ? 1.0 : etaPerp;

		// Perpendicular light reflectance
		V a = (n1 / n2)*sin(angle);
		a *= a;
		if (a <= 1.0)
		{
			V b = n2 * sqrt(1.0 - a);
			V c = n1 * cosA;
			rPerp = (c - b) / (c + b);
			rPerp *= rPerp;
			rPerp = std::min(V(1.0), rPerp);
		}

		n1 = (invert) ? etaParal : 1.0;
		n2 = (invert) ? 1.0 : etaParal;

		// Parallel light reflectance
		V d = (n1 / n2)*sin(angle);
		d *= d;
		if (d <= 1.0)
		{
			V e = n1*sqrt(1 - d);
			V f = n2*cosA;
			rParal = (e - f) / (e + f);
			rParal *= rParal;
			rParal = std::min(V(1.0), rParal);
		}

		return 0.5f * (rPerp + rParal);
	}

	// computes a new refraction index based on the hair eccentricity and the azimuth distance.
	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::eccentricityRefraction(V averageAzimuth) const
	{
		V n1 = 2.0 * (m_refraction - 1.0) * m_eccentricity * m_eccentricity - m_refraction + 2.0;
		V n2 = 2.0 * (m_refraction - 1.0) / (m_eccentricity * m_eccentricity) - m_refraction + 2.0;
		return ((n1 + n2) + cos(2.0 * averageAzimuth) * (n1 - n2)) / 2.0;
	}


	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::exitAngle(int p, V eta, V h)
	{
		// use polynomial that approximates original equation.
		V gamma = asin(h);
		V c = asin(1.0 / eta);
		return (6.0*p*c / M_PI - 2.0)*gamma - 8.0*(p*c / (M_PI*M_PI*M_PI))*gamma*gamma*gamma + p*M_PI;
	}

	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::dExitAngle(int p, V eta, V h)
	{
		// computes the derivative of the polynomial relative to h.
		V gamma = asin(h);
		V c = asin(1.0 / eta);
		V dGamma = (6.0*p*c / M_PI - 2.0) - 3.0*8.0*(p*c / (M_PI*M_PI*M_PI))*gamma*gamma;
		V denom = sqrt(1.0 - h*h);
		return dGamma / std::max(V(1e-5), denom);
	}

	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::ddExitAngle(int p, V eta, V h)
	{
		// computes the second derivative of the polynomial relative to h.
		V gamma = asin(h);
		V c = asin(1.0 / eta);
		V dGamma = -2.0*3.0*8.0*(p*c / (M_PI*M_PI*M_PI))*gamma;
		V denom = pow(1.0 - h*h, 3.0 / 2.0);
		return (dGamma*h) / std::max(V(1e-5), denom);
	}

	template < typename T >
	T MarschnerBCSDF<T>::marschnerA(int p,
		V gammaI,
		V refraction,
		V etaPerp,
		V etaParal,
		const V2 &light
	) const
	{

		if (p == 0)
		{
			return T(fresnel(gammaI, etaPerp, etaParal, false));
		}

		V h = sin(gammaI);			// from [1] right before equation 3.
		V gammaT = asin(clamp(h / etaPerp, V(-1.0), V(1.0)));	// from [1] right before equation 3.

		V etaOverRcosY = clamp(V((etaPerp / refraction) * cos(light[1])), V(-1.0), V(1.0));
		V thetaT = acos(etaOverRcosY); // definition for equation 20 in [2].	
		V cosTheta = cos(thetaT);

		V l = 2.0 * cos(gammaT) / cosTheta;

		// computes exp(-m_absorption*l) 
		T segmentAbsorption;
		segmentAbsorption = std::exp(-m_absorption*l*p);

		V invFr = fresnel(gammaT, etaPerp, etaParal, true);	// equation 24 in [2]
		V fr = (1.0 - fresnel(gammaI, etaPerp, etaParal, false)) * (1.0 - invFr);

		if (p > 1.0)
		{
			fr *= invFr;
		}

		return fr * segmentAbsorption;
	}

	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::targetAngle(int p, V relativeAzimuth)
	{
		V t = fabs(relativeAzimuth);

		// set right range to match polynomial representation of the real curve.
		if (p != 1)
		{
			// converts angles to range [-PI,PI]
			if (t > M_PI)
			{
				t -= 2.0*M_PI;
			}
			// offset center
			t += p*M_PI;
		}
		return t;
	}

	template < typename T >
	T MarschnerBCSDF<T>::marschnerNP(int p,
		V refraction,
		V etaPerp,
		V etaParal,
		const V2 &light,
		V targetAngle
	) const
	{


		// Use the polynomial approximation: o(p,y) = (6pc / PI - 2)y - 8(pc/PI^3)y^3 + pPI where C = asin( 1/eta )
		// Computes the roots of: o(p,y) - targetAngle = 0
		// Expressing as aX^3+bX^2+cX+d=0 results in:
		// a = -8(pC/PI^3)
		// b = 0
		// c = (6pC / PI - 2)
		// d = (pPI - targetAngle)

		V C = asin(clamp(V(1.0) / etaPerp, V(-1.0), V(1.0)));

		V a = -8.0 * (p*C / (M_PI*M_PI*M_PI));
		V b = 0.0;
		V c = ((6.0 * p * C) / M_PI) - 2.0;
		V d = (p * M_PI) - targetAngle;

		V roots[3] = { 0.0, 0.0, 0.0 };
		int rootCount = cubicRoots(a, b, c, d, roots);

		T result(0.0);
		for (int i = 0; i < rootCount; i++)
		{
			V gammaI = roots[i];
			if (fabs(gammaI) <= M_PI / 2.0)
			{
				V h = sin(gammaI);			// from [1] right before equation 3.
				T finalAbsorption = marschnerA(p, gammaI, refraction, etaPerp, etaParal, light);
				V denom = std::max(1e-5, (2.0 * fabs(dExitAngle(p, etaPerp, h))));
				result += finalAbsorption;
			}
		}
		return result;
	}

	template < typename T >
	T MarschnerBCSDF<T>::marschnerNTRT(V refraction,
		V etaPerp,
		V etaParal,
		const V2 &light,
		V targetAngle
	) const
	{
		V dH, t, hc, Oc1, Oc2;
		if (etaPerp < 2.0)
		{
			V c = asin(1.0 / etaPerp);
			V gammac = sqrt(((6.0 * 2.0 * c) / (M_PI - 2.0)) / (3.0 * 8.0 * ((2.0 * c) / (M_PI*M_PI*M_PI))));
			hc = fabs(sin(gammac));
			V ddexitAngle = ddExitAngle(2, etaPerp, hc);
			dH = std::min(m_causticLimit, V(2.0 * sqrt((2.0*m_causticWidth) / fabs(ddexitAngle))));
			t = 1.0;
		}
		else
		{
			hc = 0.0;
			dH = m_causticLimit;
			t = 1.0 - smoothstep(V(2.0), V(2.0 + m_causticFade), etaPerp);
		}

		Oc1 = exitAngle(2, etaPerp, hc);
		Oc2 = exitAngle(2, etaPerp, -hc);

		V3 coefficients = gaussianPDF(0.0, m_causticWidth);
		V causticCenter = gaussian(coefficients[0], coefficients[1], coefficients[2], 0.0);
		V causticLeft = gaussian(coefficients[0], coefficients[1], coefficients[2], targetAngle - Oc1);
		V causticRight = gaussian(coefficients[0], coefficients[1], coefficients[2], targetAngle - Oc2);

		T A = marschnerA(2, asin(hc), refraction, etaPerp, etaParal, light);
		T L = marschnerNP(2, refraction, etaPerp, etaParal, light, targetAngle);

		L *= 1.0 - t*causticLeft / causticCenter;
		L *= 1.0 - t*causticRight / causticCenter;

		L += t * m_glint * A * dH * (causticLeft + causticRight);

		return L;
	}

	template < typename T >
	T MarschnerBCSDF<T>::operator() (const V2 &eye, const V2 &light) const
	{
		T r, tt, trt;
		return (*this)(eye, light, r, tt, trt);
	}

	template < typename T >
	T MarschnerBCSDF<T>::operator() (const V2 &eye, const V2 &light, T &R, T &TT, T &TRT) const
	{
		// converts 3d light position into angular position using Z as hair direction (root to tip) and
		// X as major axis direction ( in case the hair is elliptical ). The x component is the inclination
		// to the normal plane (theta) that ranges from -90 to 90 and the y component is the azymuth angle
		// (phi) that ranges from 0 to 360 ( in radians ). get refraction indices as described in [1] for R and TT.

		V relativeTheta = fabs(eye[1] - light[1]) / 2.0;
		V averageTheta = (eye[1] + light[1]) / 2.0;
		V cosRelativeTheta = cos(relativeTheta);
		V invSqrCosRelativeTheta = 1.0 / (cosRelativeTheta * cosRelativeTheta);

		V etaPerp = bravaisIndex(relativeTheta, m_refraction);
		V etaParal = (m_refraction * m_refraction) / etaPerp;

		V refractionTRT = eccentricityRefraction((eye[0] + light[0]) / 2.0);
		V etaPerpTRT = bravaisIndex(relativeTheta, refractionTRT);
		V etaParalTRT = (refractionTRT*refractionTRT) / etaPerpTRT;

		V relativeAzimuth = fmod(fabs(eye[0] - light[0]), 2.0 * M_PI);

		V cosLight = cos(light[1]);
		V finalScale = invSqrCosRelativeTheta * cosLight;

		V rWidth = 5.0;
		V MR = marschnerM(m_shiftR, m_widthR, rWidth, averageTheta);
		V MTT = marschnerM(m_shiftTT, m_widthTT, rWidth / 2.0, averageTheta);
		V MTRT = marschnerM(m_shiftTRT, m_widthTRT, rWidth*2.0, averageTheta);

		T NR = marschnerNP(0, m_refraction, etaPerp, etaParal, light, targetAngle(0, relativeAzimuth));
		T NTT = marschnerNP(1, m_refraction, etaPerp, etaParal, light, targetAngle(1, relativeAzimuth));
		T NTRT = marschnerNTRT(refractionTRT, etaPerpTRT, etaParalTRT, light, targetAngle(2, relativeAzimuth));

		R = MR*NR*finalScale;
		TT = MTT*NTT*finalScale;
		TRT = MTRT*NTRT*finalScale;

		return R + TT + TRT;
	}

	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::MR(const V2 &eye, const V2 &light) const
	{
		V rWidth = 5.0;
		V averageTheta = (eye[1] + light[1]) / 2.0;
		V relativeTheta = fabs(eye[1] - light[1]) / 2.0;
		V cosRelativeTheta = cos(relativeTheta);
		V invSqrCosRelativeTheta = 1.0 / (cosRelativeTheta * cosRelativeTheta);
		V cosLight = cos(light[1]);
		V finalScale = invSqrCosRelativeTheta * cosLight;

		return marschnerM(m_shiftR, m_widthR, rWidth, averageTheta) * finalScale;
	}

	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::MTT(const V2 &eye, const V2 &light) const
	{
		V rWidth = 5.0;
		V averageTheta = (eye[1] + light[1]) / 2.0;
		V relativeTheta = fabs(eye[1] - light[1]) / 2.0;
		V cosRelativeTheta = cos(relativeTheta);
		V invSqrCosRelativeTheta = 1.0 / (cosRelativeTheta * cosRelativeTheta);
		V cosLight = cos(light[1]);
		V finalScale = invSqrCosRelativeTheta * cosLight;

		return marschnerM(m_shiftTT, m_widthTT, rWidth / 2.0, averageTheta) * finalScale;
	}

	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::MTRT(const V2 &eye, const V2 &light) const
	{
		V rWidth = 5.0;
		V averageTheta = (eye[1] + light[1]) / 2.0;
		V relativeTheta = fabs(eye[1] - light[1]) / 2.0;
		V cosRelativeTheta = cos(relativeTheta);
		V invSqrCosRelativeTheta = 1.0 / (cosRelativeTheta * cosRelativeTheta);
		V cosLight = cos(light[1]);
		V finalScale = invSqrCosRelativeTheta * cosLight;

		return marschnerM(m_shiftTRT, m_widthTRT, rWidth*2.0, averageTheta) * finalScale;
	}

	template < typename T >
	T MarschnerBCSDF<T>::NR(const V2 &eye, const V2 &light) const
	{
		V relativeTheta = fabs(eye[1] - light[1]) / 2.0;
		V relativeAzimuth = fmod(fabs(eye[0] - light[0]), 2.0 * M_PI);

		V etaPerp = bravaisIndex(relativeTheta, m_refraction);
		V etaParal = (m_refraction * m_refraction) / etaPerp;

		return marschnerNP(0, m_refraction, etaPerp, etaParal, light, targetAngle(0, relativeAzimuth));
	}

	template < typename T >
	T MarschnerBCSDF<T>::NTT(const V2 &eye, const V2 &light) const
	{
		V relativeTheta = fabs(eye[1] - light[1]) / 2.0;
		V relativeAzimuth = fmod(fabs(eye[0] - light[0]), 2.0 * M_PI);

		V etaPerp = bravaisIndex(relativeTheta, m_refraction);
		V etaParal = (m_refraction * m_refraction) / etaPerp;

		return marschnerNP(1, m_refraction, etaPerp, etaParal, light, targetAngle(1, relativeAzimuth));
	}

	template < typename T >
	T MarschnerBCSDF<T>::NTRT(const V2 &eye, const V2 &light) const
	{
		V relativeTheta = fabs(eye[1] - light[1]) / 2.0;
		V relativeAzimuth = fmod(fabs(eye[0] - light[0]), 2.0 * M_PI);

		V refractionTRT = eccentricityRefraction((eye[0] + light[0]) / 2.0);
		V etaPerpTRT = bravaisIndex(relativeTheta, refractionTRT);
		V etaParalTRT = (refractionTRT*refractionTRT) / etaPerpTRT;

		return marschnerNTRT(refractionTRT, etaPerpTRT, etaParalTRT, light, targetAngle(2, relativeAzimuth));
	}


	/// \TODO Migrate these to boost or some other library solves
	// Computes real roots for a given cubic polynomial (x^3+Ax^2+Bx+C = 0).
	template < typename T >
	int MarschnerBCSDF<T>::cubicRoots(V A, V B, V C, V D, V roots[3])
	{
		if (fabs(A) < std::numeric_limits<T>::epsilon())
		{
			return quadraticRoots(B, C, D, roots);
		}
		else
		{
			return normalizedCubicRoots(B / A, C / A, D / A, roots);
		}
	}

	// Computes real roots for a given cubic polynomial (x^3+Ax^2+Bx+C = 0).
	template < typename T >
	int MarschnerBCSDF<T>::normalizedCubicRoots(V A, V B, V C, V roots[3])
	{
		if (fabs(C) < std::numeric_limits<T>::epsilon())
		{
			return quadraticRoots(1, A, B, roots);
		}

		V Q = (3.0*B - A*A) / 9.0;
		V R = (9.0*A*B - 27.0*C - 2.0*A*A*A) / 54.0;
		V D = Q*Q*Q + R*R;	// polynomial discriminant

		if (D >= 0) // complex or duplicate roots
		{
			V sqrtD = sqrt(D);
			V S = cubicRoot(R + sqrtD);
			V t = cubicRoot(R - sqrtD);
			roots[0] = -A / 3.0 + (S + t);                    // real root
			return 1;
		}
		else                                        // distinct real roots
		{
			V th = acos(R / sqrt(-Q*Q*Q));
			V sqrtQ = sqrt(-Q);
			roots[0] = 2.0*sqrtQ*cos(th / 3.0) - A / 3.0;
			roots[1] = 2.0*sqrtQ*cos((th + 2.0*M_PI) / 3.0) - A / 3.0;
			roots[2] = 2.0*sqrtQ*cos((th + 4.0*M_PI) / 3.0) - A / 3.0;
			return 3;
		}
	}

	template < typename T >
	typename MarschnerBCSDF<T>::V MarschnerBCSDF<T>::cubicRoot(V v)
	{
		if (v < 0) {
			return -pow(-v, 1.0 / 3.0);
		}
		return pow(v, 1.0 / 3.0);
	}


	template < typename T >
	int MarschnerBCSDF<T>::quadraticRoots(V a, V b, V c, V roots[2])
	{
		if (fabs(a) < std::numeric_limits<T>::epsilon())
		{
			int count = linearRoots(b, c, roots[0]);
			return count;
		}

		V D = b*b - 4.0*a*c;

		if (fabs(D) < std::numeric_limits<T>::epsilon())
		{
			roots[0] = -b / (2.0*a);
			return 1;
		}
		else if (D > 0)
		{
			V s = sqrt(D);
			roots[0] = (-b + s) / (2.0 * a);
			roots[1] = (-b - s) / (2.0 * a);
			return 2;
		}

		return 0;
	}

	template < typename T >
	int MarschnerBCSDF<T>::linearRoots(V a, V b, V &root)
	{
		int rootCount = -1;
		if (a != 0.0)
		{
			root = -b / a;
			rootCount = 1;
		}
		else if (b != 0.0)
		{
			rootCount = 0;
		}
		return rootCount;
	}


} // namespace