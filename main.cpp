#include <iostream>
#include <fstream>
#include <string>
#include <memory>

//const std::string Alphabet{ "1,qM2;wN3.eB4:rV5_tC6+yX7-uZ8*iL9/oK0<pJ>aH!sG@dF#fD$gS%hA&jP/kO{lI(zU[xY=cT?vR'bE^nW~mQ])}|\\\"`" };

// contains duplicates:
const std::string Alphabet{ "1qM,2wN;3eB.4rV:5tC_6yX/7uZ*8iL-9oK+0pJ!1aH@2sG#3dF$4fD%5gS&6hA/7jP{8kO(9lI[0zU)xY]|cT='vR}<bE?>nW^\\mQ~\"`" };

int main(int argc, char** argv)
{
	constexpr uint32_t alphaSize{85};
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
	else
	{
		//std::cerr << "No input file specified. Reading from std::cin.\n";
	}

	std::istream& instream{ argc == 2 ? infile : std::cin };
	
	while ( instream.good() )
	{
		char buffer[4]{0,0,0,0};
		instream.read(buffer, 4);
		size_t bytesRead = instream.gcount();

		if ( bytesRead > 0 )
		{
			uint32_t word = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
			char output[5];

			output[4] = Alphabet[word % alphaSize];
			for ( int j = 3; j >= 0; --j )
			{
				word /= alphaSize;
				output[j] = Alphabet[word % alphaSize];
			}

			const size_t limit{ bytesRead + 1 };
			for ( size_t j = 0; j < limit; ++j )
				std::cout << output[j];
		}
	}

	//std::cout << "\n";
	return 0;
}