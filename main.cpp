#include <iostream>
#include <fstream>

int main(int argc, char** argv)
{
	std::ios::sync_with_stdio(false); // toivotaan, että tämä nopeuttaa aiheuttamatta haittaa
	constexpr uint32_t alphabetSize{85};
	std::ifstream infile;

	if ( argc == 2 )
	{
		infile.open(argv[1], std::ios::binary);
		if ( !infile.good() )
		{
			std::cerr << "Unable to open input file.\n";
			return 1;
		}
	}
	// else
	// {
	// 	std::cerr << "No input file specified. Reading from std::cin.\n";
	// }

	std::istream& instream{ argc == 2 ? infile : std::cin };
	size_t wrap { 75 }, charsOnThisLine{ 0 };

	while ( instream.good() )
	{
		char buffer[4]{0,0,0,0};
		instream.read(buffer, 4);
		size_t bytesRead = instream.gcount();

		if ( bytesRead > 0 )
		{
			uint32_t word = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];

			if ( word == 0 )
			{
				std::cout << 'z';
				++charsOnThisLine;
				continue;
			}

			if ( word == 0x20202020 )
			{
				std::cout << 'y';
				++charsOnThisLine;
				continue;
			}

			char output[5];
			output[4] = 33 + word % alphabetSize;
			for ( int j = 3; j >= 0; --j )
			{
				word /= alphabetSize;
				output[j] = 33 + word % alphabetSize;
			}

			size_t writeThisMany = bytesRead + 1;
			for ( size_t j = 0; j < writeThisMany; ++j )
			{
				if ( (wrap != 0) && (charsOnThisLine == wrap) )
				{
					std::cout << '\n';
					charsOnThisLine = 0;
				}
				std::cout << output[j];
				++charsOnThisLine;
			}
		}
	}

	return 0;
}