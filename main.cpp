#include <iostream>
#include <fstream>
#include <string>
#include <memory>

//const std::string Alphabet{ "1,qM2;wN3.eB4:rV5_tC6+yX7-uZ8*iL9/oK0<pJ>aH!sG@dF#fD$gS%hA&jP/kO{lI(zU[xY=cT?vR'bE^nW~mQ])}|\\\"`" };

// contains duplicates:
const std::string Alphabet{ "1qM,2wN;3eB.4rV:5tC_6yX/7uZ*8iL-9oK+0pJ!1aH@2sG#3dF$4fD%5gS&6hA/7jP{8kO(9lI[0zU)xY]|cT='vR}<bE?>nW^\\mQ~\"`" };

int main(int argc, char** argv)
{
	if ( argc < 2 )
	{
		std::cout << "Ei argumenttia.\n";
		return 1;
	}

	constexpr uint32_t alphaSize{85};
	//const uint32_t alphaSize = static_cast<uint32_t>(Alphabet.size());
	// while ( log < UINT32_MAX )
	// 	log *= Alphabet.size();

	std::ifstream infile(argv[1], std::ios::binary);
	infile.seekg(0, std::ios::end);
	size_t inputSize = infile.tellg();
	infile.seekg(0, std::ios::beg);
	const size_t bufsize{ ((inputSize + 3u) / 4u) * 4u };
	std::unique_ptr<char[]> buffer(new char[bufsize]);
	infile.read(buffer.get(), inputSize);
	infile.close();

	const size_t padding{ bufsize - inputSize };
	for (size_t i = inputSize; i < bufsize; ++i)
	{
		buffer[i] = 0;
	}

	for (size_t i = 0; i < bufsize; i += 4)
	{
		uint32_t word = (buffer[i] << 24) | (buffer[i+1] << 16) | (buffer[i+2] << 8) | (buffer[i+3]);
		//char output[5]{ 33, 33, 33, 33, static_cast<char>(33 + word % alphaSize) };
		char output[5];

		output[4] = Alphabet[word % alphaSize];
		for ( int j = 3; j >= 0; --j )
		{
			word /= alphaSize;
			output[j] = Alphabet[word % alphaSize];
		}

		size_t limit{ i < (bufsize - 4) ? 5 : 5 - padding };
		for ( size_t j = 0; j < limit; ++j )
			std::cout << output[j];
	}
	
	std::cout << "\n";
	return 0;
}