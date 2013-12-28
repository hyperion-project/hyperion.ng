
// STl includes
#include <iostream>
#include <cmath>

bool requiredTiming(const int tHigh_ns, const int tLow_ns, const int error_ns, const int nrBits)
{
	std::cout << "=== " << nrBits << " bits case ===== " << std::endl;
	double bitLength_ns = (tHigh_ns + tLow_ns)/double(nrBits);
	double baudrate_Hz  = 1.0 / bitLength_ns * 1e9;
	std::cout << "Required bit length: " << bitLength_ns << "ns => baudrate = " << baudrate_Hz << std::endl;

	double highBitsExact = tHigh_ns/bitLength_ns;
	int highBits = std::round(highBitsExact);
	double lowBitsExact  = tLow_ns/bitLength_ns;
	int lowBits  = std::round(lowBitsExact);
	std::cout << "Bit division: high=" << highBits << "(" << highBitsExact << "); low=" << lowBits << "(" << lowBitsExact << ")" << std::endl;

	double highBitsError = std::fabs(highBitsExact - highBits);
	double lowBitsError  = std::fabs(highBitsExact - highBits);
	double highError_ns = highBitsError * bitLength_ns;
	double lowError_ns  = lowBitsError * bitLength_ns;

	if (highError_ns > error_ns || lowError_ns > error_ns)
	{
		std::cerr << "Timing error outside specs: " << highError_ns << "; " << lowError_ns << " > " << error_ns << std::endl;
	}
	else
	{
		std::cout << "Timing within margins: " << highError_ns << "; " << lowError_ns << " < " << error_ns << std::endl;
	}



	return true;
}

int main()
{
	// 10bits
	requiredTiming(400, 850, 150, 10);	// Zero
	requiredTiming(800, 450, 150, 10);	// One

	// 6bits
	requiredTiming(400, 850, 150, 6);	// Zero
	requiredTiming(800, 450, 150, 6);	// One

	// 5bits
	requiredTiming(400, 850, 150, 5);	// Zero
	requiredTiming(800, 450, 150, 5);	// One

	requiredTiming(650, 600, 150, 5);	// One

	// 4bits
	requiredTiming(400, 850, 150, 4);	// Zero
	requiredTiming(800, 450, 150, 4);	// One

	// 3bits
	requiredTiming(400, 850, 150, 3);	// Zero
	requiredTiming(800, 450, 150, 3);	// One
	return 0;
}
