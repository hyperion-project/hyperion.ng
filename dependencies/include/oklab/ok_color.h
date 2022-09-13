#ifndef OKCOLOR_H
#define OKCOLOR_H
// Copyright(c) 2021 Björn Ottosson + Modifications by Hyperion Project
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files(the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions :
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cfloat>
#include <cmath>

namespace ok_color
{

struct Lab { double L; double a; double b; };
struct RGB { double r; double g; double b; };

struct HSV { double h; double s; double v; };
struct HSL { double h; double s; double l; };
struct LC { double L; double C; };

// Alternative representation of (L_cusp, C_cusp)
// Encoded so S = C_cusp/L_cusp and T = C_cusp/(1-L_cusp)
// The maximum value for C in the triangle is then found as fmin(S*L, T*(1-L)), for a given L
struct ST { double S; double T; };

constexpr double pi = 3.1415926535897932384626433832795028841971693993751058209749445923078164062;

double clamp(double x, double min, double max)
{
	if (x < min) {
		return min;
	}
	if (x > max) {
		return max;
	}

	return x;
}

double sgn(double x)
{
	return static_cast<double>(0.0 < x) - static_cast<double>(x < 0.0);
}

double srgb_transfer_function(double a)
{
	return .0031308 >= a ? 12.92 * a : 1.055 * pow(a, .4166666666666667) - .055;
}

double srgb_transfer_function_inv(double a)
{
	return .04045< a ? pow((a + .055) / 1.055, 2.4) : a / 12.92;
}

Lab linear_srgb_to_oklab(RGB c)
{
	double l = 0.4122214708 * c.r + 0.5363325363 * c.g + 0.0514459929 * c.b;
	double m = 0.2119034982 * c.r + 0.6806995451 * c.g + 0.1073969566 * c.b;
	double s = 0.0883024619 * c.r + 0.2817188376 * c.g + 0.6299787005 * c.b;

	double l_ = cbrt(l);
	double m_ = cbrt(m);
	double s_ = cbrt(s);

	return {
			0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
			1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
			0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_,
	};
}

RGB oklab_to_linear_srgb(Lab c)
{
	double l_ = c.L + 0.3963377774 * c.a + 0.2158037573 * c.b;
	double m_ = c.L - 0.1055613458 * c.a - 0.0638541728 * c.b;
	double s_ = c.L - 0.0894841775 * c.a - 1.2914855480 * c.b;

	double l = l_ * l_ * l_;
	double m = m_ * m_ * m_;
	double s = s_ * s_ * s_;

	return {
			+4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
			-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
			-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s,
	};
}

// Finds the maximum saturation possible for a given hue that fits in sRGB
// Saturation here is defined as S = C/L
// a and b must be normalized so a^2 + b^2 == 1
double compute_max_saturation(double a, double b)
{
	// Max saturation will be when one of r, g or b goes below zero.

	// Select different coefficients depending on which component goes below zero first
	double k0;
	double k1;
	double k2;
	double k3;
	double k4;
	double wl;
	double wm;
	double ws;

	if (-1.88170328 * a - 0.80936493 * b > 1)
	{
		// Red component
		k0 = +1.19086277; k1 = +1.76576728; k2 = +0.59662641; k3 = +0.75515197; k4 = +0.56771245;
		wl = +4.0767416621; wm = -3.3077115913; ws = +0.2309699292;
	}
	else
		if (1.81444104 * a - 1.19445276 * b > 1)
		{
			// Green component
			k0 = +0.73956515; k1 = -0.45954404; k2 = +0.08285427; k3 = +0.12541070; k4 = +0.14503204;
			wl = -1.2684380046; wm = +2.6097574011; ws = -0.3413193965;
		}
		else
		{
			// Blue component
			k0 = +1.35733652; k1 = -0.00915799; k2 = -1.15130210; k3 = -0.50559606; k4 = +0.00692167;
			wl = -0.0041960863; wm = -0.7034186147; ws = +1.7076147010;
		}

	// Approximate max saturation using a polynomial:
	double S = k0 + k1 * a + k2 * b + k3 * a * a + k4 * a * b;

	// Do one step Halley's method to get closer
	// this gives an error less than 10e6, except for some blue hues where the dS/dh is close to infinite
	// this should be sufficient for most applications, otherwise do two/three steps

	double k_l = +0.3963377774 * a + 0.2158037573 * b;
	double k_m = -0.1055613458 * a - 0.0638541728 * b;
	double k_s = -0.0894841775 * a - 1.2914855480 * b;

	{
		double l_ = 1.0 + S * k_l;
		double m_ = 1.0 + S * k_m;
		double s_ = 1.0 + S * k_s;

		double l = l_ * l_ * l_;
		double m = m_ * m_ * m_;
		double s = s_ * s_ * s_;

		double l_dS = 3.0 * k_l * l_ * l_;
		double m_dS = 3.0 * k_m * m_ * m_;
		double s_dS = 3.0 * k_s * s_ * s_;

		double l_dS2 = 6.0 * k_l * k_l * l_;
		double m_dS2 = 6.0 * k_m * k_m * m_;
		double s_dS2 = 6.0 * k_s * k_s * s_;

		double f = wl * l + wm * m + ws * s;
		double f1 = wl * l_dS + wm * m_dS + ws * s_dS;
		double f2 = wl * l_dS2 + wm * m_dS2 + ws * s_dS2;

		S = S - f * f1 / (f1 * f1 - 0.5 * f * f2);
	}

	return S;
}

// finds L_cusp and C_cusp for a given hue
// a and b must be normalized so a^2 + b^2 == 1
LC find_cusp(double a, double b)
{
	// First, find the maximum saturation (saturation S = C/L)
	double S_cusp = compute_max_saturation(a, b);

	// Convert to linear sRGB to find the first point where at least one of r,g or b >= 1:
	RGB rgb_at_max = oklab_to_linear_srgb({ 1, S_cusp * a, S_cusp * b });
	double L_cusp = cbrt(1.0 / fmax(fmax(rgb_at_max.r, rgb_at_max.g), rgb_at_max.b));
	double C_cusp = L_cusp * S_cusp;

	return { L_cusp , C_cusp };
}

// Finds intersection of the line defined by
// L = L0 * (1 - t) + t * L1;
// C = t * C1;
// a and b must be normalized so a^2 + b^2 == 1
double find_gamut_intersection(double a, double b, double L1, double C1, double L0, LC cusp)
{
	// Find the intersection for upper and lower half seprately
	double t;
	if (((L1 - L0) * cusp.C - (cusp.L - L0) * C1) <= 0.0)
	{
		// Lower half

		t = cusp.C * L0 / (C1 * cusp.L + cusp.C * (L0 - L1));
	}
	else
	{
		// Upper half

		// First intersect with triangle
		t = cusp.C * (L0 - 1.0) / (C1 * (cusp.L - 1.0) + cusp.C * (L0 - L1));

		// Then one step Halley's method
		{
			double dL = L1 - L0;
			double dC = C1;

			double k_l = +0.3963377774 * a + 0.2158037573 * b;
			double k_m = -0.1055613458 * a - 0.0638541728 * b;
			double k_s = -0.0894841775 * a - 1.2914855480 * b;

			double l_dt = dL + dC * k_l;
			double m_dt = dL + dC * k_m;
			double s_dt = dL + dC * k_s;


			// If higher accuracy is required, 2 or 3 iterations of the following block can be used:
			{
				double L = L0 * (1.0 - t) + t * L1;
				double C = t * C1;

				double l_ = L + C * k_l;
				double m_ = L + C * k_m;
				double s_ = L + C * k_s;

				double l = l_ * l_ * l_;
				double m = m_ * m_ * m_;
				double s = s_ * s_ * s_;

				double ldt = 3 * l_dt * l_ * l_;
				double mdt = 3 * m_dt * m_ * m_;
				double sdt = 3 * s_dt * s_ * s_;

				double ldt2 = 6 * l_dt * l_dt * l_;
				double mdt2 = 6 * m_dt * m_dt * m_;
				double sdt2 = 6 * s_dt * s_dt * s_;

				double r0 = 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s - 1;
				double r1 = 4.0767416621 * ldt - 3.3077115913 * mdt + 0.2309699292 * sdt;
				double r2 = 4.0767416621 * ldt2 - 3.3077115913 * mdt2 + 0.2309699292 * sdt2;

				double u_r = r1 / (r1 * r1 - 0.5 * r0 * r2);
				double t_r = -r0 * u_r;

				double g0 = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s - 1;
				double g1 = -1.2684380046 * ldt + 2.6097574011 * mdt - 0.3413193965 * sdt;
				double g2 = -1.2684380046 * ldt2 + 2.6097574011 * mdt2 - 0.3413193965 * sdt2;

				double u_g = g1 / (g1 * g1 - 0.5 * g0 * g2);
				double t_g = -g0 * u_g;

				double b0 = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s - 1;
				double b1 = -0.0041960863 * ldt - 0.7034186147 * mdt + 1.7076147010 * sdt;
				double b2 = -0.0041960863 * ldt2 - 0.7034186147 * mdt2 + 1.7076147010 * sdt2;

				double u_b = b1 / (b1 * b1 - 0.5 * b0 * b2);
				double t_b = -b0 * u_b;

				t_r = u_r >= 0.0 ? t_r : DBL_MAX;
				t_g = u_g >= 0.0 ? t_g : DBL_MAX;
				t_b = u_b >= 0.0 ? t_b : DBL_MAX;

				t += fmin(t_r, fmin(t_g, t_b));
			}
		}
	}

	return t;
}

double find_gamut_intersection(double a, double b, double L1, double C1, double L0)
{
	// Find the cusp of the gamut triangle
	LC cusp = find_cusp(a, b);

	return find_gamut_intersection(a, b, L1, C1, L0, cusp);
}

RGB gamut_clip_preserve_chroma(RGB rgb)
{
	if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
	{
		return rgb;
	}

	Lab lab = linear_srgb_to_oklab(rgb);

	double L = lab.L;
	double eps = 0.00001;
	double C = fmax(eps, sqrt(lab.a * lab.a + lab.b * lab.b));
	double a_ = lab.a / C;
	double b_ = lab.b / C;

	double L0 = clamp(L, 0, 1);

	double t = find_gamut_intersection(a_, b_, L, C, L0);
	double L_clipped = L0 * (1 - t) + t * L;
	double C_clipped = t * C;

	return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
}

RGB gamut_clip_project_to_0_5(RGB rgb)
{
	if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0) {
		return rgb;
	}

	Lab lab = linear_srgb_to_oklab(rgb);

	double L = lab.L;
	double eps = 0.00001;
	double C = fmax(eps, sqrt(lab.a * lab.a + lab.b * lab.b));
	double a_ = lab.a / C;
	double b_ = lab.b / C;

	double L0 = 0.5;

	double t = find_gamut_intersection(a_, b_, L, C, L0);
	double L_clipped = L0 * (1 - t) + t * L;
	double C_clipped = t * C;

	return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
}

RGB gamut_clip_project_to_L_cusp(RGB rgb)
{
	if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0) {
		return rgb;
	}

	Lab lab = linear_srgb_to_oklab(rgb);

	double L = lab.L;
	double eps = 0.00001;
	double C = fmax(eps, sqrt(lab.a * lab.a + lab.b * lab.b));
	double a_ = lab.a / C;
	double b_ = lab.b / C;

	// The cusp is computed here and in find_gamut_intersection, an optimized solution would only compute it once.
	LC cusp = find_cusp(a_, b_);

	double L0 = cusp.L;

	double t = find_gamut_intersection(a_, b_, L, C, L0);

	double L_clipped = L0 * (1 - t) + t * L;
	double C_clipped = t * C;

	return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
}

RGB gamut_clip_adaptive_L0_0_5(RGB rgb, double alpha = 0.05)
{
	if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0) {
		return rgb;
	}

	Lab lab = linear_srgb_to_oklab(rgb);

	double L = lab.L;
	double eps = 0.00001;
	double C = fmax(eps, sqrt(lab.a * lab.a + lab.b * lab.b));
	double a_ = lab.a / C;
	double b_ = lab.b / C;

	double Ld = L - 0.5;
	double e1 = 0.5 + fabs(Ld) + alpha * C;
	double L0 = 0.5 * (1.0 + sgn(Ld) * (e1 - sqrt(e1 * e1 - 2.0 * fabs(Ld))));

	double t = find_gamut_intersection(a_, b_, L, C, L0);
	double L_clipped = L0 * (1.0 - t) + t * L;
	double C_clipped = t * C;

	return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
}

RGB gamut_clip_adaptive_L0_L_cusp(RGB rgb, double alpha = 0.05)
{
	if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0) {
		return rgb;
	}

	Lab lab = linear_srgb_to_oklab(rgb);

	double L = lab.L;
	double eps = 0.00001;
	double C = fmax(eps, sqrt(lab.a * lab.a + lab.b * lab.b));
	double a_ = lab.a / C;
	double b_ = lab.b / C;

	// The cusp is computed here and in find_gamut_intersection, an optimized solution would only compute it once.
	LC cusp = find_cusp(a_, b_);

	double Ld = L - cusp.L;
	double k = 2.0 * (Ld > 0 ? 1.0 - cusp.L : cusp.L);

	double e1 = 0.5 * k + fabs(Ld) + alpha * C / k;
	double L0 = cusp.L + 0.5 * (sgn(Ld) * (e1 - sqrt(e1 * e1 - 2.0 * k * fabs(Ld))));

	double t = find_gamut_intersection(a_, b_, L, C, L0);
	double L_clipped = L0 * (1.0 - t) + t * L;
	double C_clipped = t * C;

	return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
}

double toe(double x)
{
	constexpr double k_1 = 0.206;
	constexpr double k_2 = 0.03;
	constexpr double k_3 = (1.0 + k_1) / (1.0 + k_2);
	return 0.5 * (k_3 * x - k_1 + sqrt((k_3 * x - k_1) * (k_3 * x - k_1) + 4 * k_2 * k_3 * x));
}

double toe_inv(double x)
{
	constexpr double k_1 = 0.206;
	constexpr double k_2 = 0.03;
	constexpr double k_3 = (1.0 + k_1) / (1.0 + k_2);
	return (x * x + k_1 * x) / (k_3 * (x + k_2));
}

ST to_ST(LC cusp)
{
	double L = cusp.L;
	double C = cusp.C;
	return { C / L, C / (1 - L) };
}

// Returns a smooth approximation of the location of the cusp
// This polynomial was created by an optimization process
// It has been designed so that S_mid < S_max and T_mid < T_max
ST get_ST_mid(double a_, double b_)
{
	double S = 0.11516993 + 1.0 / (
								+ 7.44778970 + 4.15901240 * b_
								+ a_ * (-2.19557347 + 1.75198401 * b_
										+ a_ * (-2.13704948 - 10.02301043 * b_
												+ a_ * (-4.24894561 + 5.38770819 * b_ + 4.69891013 * a_
														)))
								);

	double T = 0.11239642 + 1.0 / (
								+ 1.61320320 - 0.68124379 * b_
								+ a_ * (+0.40370612 + 0.90148123 * b_
										+ a_ * (-0.27087943 + 0.61223990 * b_
												+ a_ * (+0.00299215 - 0.45399568 * b_ - 0.14661872 * a_
														)))
								);

	return { S, T };
}

struct Cs { double C_0; double C_mid; double C_max; };
Cs get_Cs(double L, double a_, double b_)
{
	LC cusp = find_cusp(a_, b_);

	double C_max = find_gamut_intersection(a_, b_, L, 1, L, cusp);
	ST ST_max = to_ST(cusp);

	// Scale factor to compensate for the curved part of gamut shape:
	double k = C_max / fmin((L * ST_max.S), (1 - L) * ST_max.T);

	double C_mid;
	{
		ST ST_mid = get_ST_mid(a_, b_);

		// Use a soft minimum function, instead of a sharp triangle shape to get a smooth value for chroma.
		double C_a = L * ST_mid.S;
		double C_b = (1.0 - L) * ST_mid.T;
		C_mid = 0.9 * k * sqrt(sqrt(1.0 / (1.0 / (C_a * C_a * C_a * C_a) + 1.0 / (C_b * C_b * C_b * C_b))));
	}

	double C_0;
	{
		// for C_0, the shape is independent of hue, so ST are constant. Values picked to roughly be the average values of ST.
		double C_a = L * 0.4;
		double C_b = (1.0 - L) * 0.8;

		// Use a soft minimum function, instead of a sharp triangle shape to get a smooth value for chroma.
		C_0 = sqrt(1.0 / (1.0 / (C_a * C_a) + 1.0 / (C_b * C_b)));
	}

	return { C_0, C_mid, C_max };
}

RGB okhsl_to_srgb(HSL hsl)
{
	double h = hsl.h;
	double s = hsl.s;
	double l = hsl.l;

	if (l == 1.0)
	{
		return { 1, 1, 1 };
	}

	if (l == 0.0)
	{
		return { 0, 0, 0 };
	}

	double a_ = cos(2 * pi * h);
	double b_ = sin(2 * pi * h);
	double L = toe_inv(l);

	Cs cs = get_Cs(L, a_, b_);
	double C_0 = cs.C_0;
	double C_mid = cs.C_mid;
	double C_max = cs.C_max;

	double mid = 0.8;
	double mid_inv = 1.25;

	double C;
	double t;
	double k_0;
	double k_1;
	double k_2;

	if (s < mid)
	{
		t = mid_inv * s;

		k_1 = mid * C_0;
		k_2 = (1.0 - k_1 / C_mid);

		C = t * k_1 / (1.0 - k_2 * t);
	}
	else
	{
		t = (s - mid)/ (1 - mid);

		k_0 = C_mid;
		k_1 = (1.0 - mid) * C_mid * C_mid * mid_inv * mid_inv / C_0;
		k_2 = (1.0 - (k_1) / (C_max - C_mid));

		C = k_0 + t * k_1 / (1.0 - k_2 * t);
	}

	RGB rgb = oklab_to_linear_srgb({ L, C * a_, C * b_ });
	return {
			srgb_transfer_function(rgb.r),
			srgb_transfer_function(rgb.g),
			srgb_transfer_function(rgb.b),
			};
}

HSL srgb_to_okhsl(RGB rgb)
{
	Lab lab = linear_srgb_to_oklab({
		srgb_transfer_function_inv(rgb.r),
		srgb_transfer_function_inv(rgb.g),
		srgb_transfer_function_inv(rgb.b)
	});

	double C = sqrt(lab.a * lab.a + lab.b * lab.b);
	double a_ = lab.a / C;
	double b_ = lab.b / C;

	double L = lab.L;
	double h = 0.5 + 0.5 * atan2(-lab.b, -lab.a) / pi;

	Cs cs = get_Cs(L, a_, b_);
	double C_0 = cs.C_0;
	double C_mid = cs.C_mid;
	double C_max = cs.C_max;

	// Inverse of the interpolation in okhsl_to_srgb:

	double mid = 0.8;
	double mid_inv = 1.25;

	double s;
	if (C < C_mid)
	{
		double k_1 = mid * C_0;
		double k_2 = (1.0 - k_1 / C_mid);

		double t = C / (k_1 + k_2 * C);
		s = t * mid;
	}
	else
	{
		double k_0 = C_mid;
		double k_1 = (1.0 - mid) * C_mid * C_mid * mid_inv * mid_inv / C_0;
		double k_2 = (1.0 - (k_1) / (C_max - C_mid));

		double t = (C - k_0) / (k_1 + k_2 * (C - k_0));
		s = mid + (1.0 - mid) * t;
	}

	double l = toe(L);
	return { h, s, l };
}


RGB okhsv_to_srgb(HSV hsv)
{
	double h = hsv.h;
	double s = hsv.s;
	double v = hsv.v;

	double a_ = cos(2.0 * pi * h);
	double b_ = sin(2.0 * pi * h);

	LC cusp = find_cusp(a_, b_);
	ST ST_max = to_ST(cusp);
	double S_max = ST_max.S;
	double T_max = ST_max.T;
	double S_0 = 0.5;
	double k = 1 - S_0 / S_max;

	// first we compute L and V as if the gamut is a perfect triangle:

	// L, C when v==1:
	double L_v = 1     - s * S_0 / (S_0 + T_max - T_max * k * s);
	double C_v = s * T_max * S_0 / (S_0 + T_max - T_max * k * s);

	double L = v * L_v;
	double C = v * C_v;

	// then we compensate for both toe and the curved top part of the triangle:
	double L_vt = toe_inv(L_v);
	double C_vt = C_v * L_vt / L_v;

	double L_new = toe_inv(L);
	C = C * L_new / L;
	L = L_new;

	RGB rgb_scale = oklab_to_linear_srgb({ L_vt, a_ * C_vt, b_ * C_vt });
	double scale_L = cbrt(1.0 / fmax(fmax(rgb_scale.r, rgb_scale.g), fmax(rgb_scale.b, 0.0)));

	L = L * scale_L;
	C = C * scale_L;

	RGB rgb = oklab_to_linear_srgb({ L, C * a_, C * b_ });
	return {
			srgb_transfer_function(rgb.r),
			srgb_transfer_function(rgb.g),
			srgb_transfer_function(rgb.b),
			};
}

HSV srgb_to_okhsv(RGB rgb)
{
	Lab lab = linear_srgb_to_oklab({
		srgb_transfer_function_inv(rgb.r),
		srgb_transfer_function_inv(rgb.g),
		srgb_transfer_function_inv(rgb.b)
	});

	double C = sqrt(lab.a * lab.a + lab.b * lab.b);
	double a_ = lab.a / C;
	double b_ = lab.b / C;

	double L = lab.L;
	double h = 0.5 + 0.5 * atan2(-lab.b, -lab.a) / pi;

	LC cusp = find_cusp(a_, b_);
	ST ST_max = to_ST(cusp);
	double S_max = ST_max.S;
	double T_max = ST_max.T;
	double S_0 = 0.5;
	double k = 1 - S_0 / S_max;

	// first we find L_v, C_v, L_vt and C_vt

	double t = T_max / (C + L * T_max);
	double L_v = t * L;
	double C_v = t * C;

	double L_vt = toe_inv(L_v);
	double C_vt = C_v * L_vt / L_v;

	// we can then use these to invert the step that compensates for the toe and the curved top part of the triangle:
	RGB rgb_scale = oklab_to_linear_srgb({ L_vt, a_ * C_vt, b_ * C_vt });
	double scale_L = cbrt(1.0 / fmax(fmax(rgb_scale.r, rgb_scale.g), fmax(rgb_scale.b, 0.0)));

	L = L / scale_L;
	//C = C / scale_L;

	//C = C * toe(L) / L;
	L = toe(L);

	// we can now compute v and s:

	double v = L / L_v;
	double s = (S_0 + T_max) * C_v / ((T_max * S_0) + T_max * k * C_v);

	return { h, s, v };
}

} // namespace ok_color

#endif // OKCOLOR_H
