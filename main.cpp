#include <iostream>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>

struct BinaryMode
{
	int oldin, oldout;

	BinaryMode()
	{
		oldin = _setmode(_fileno(stdout), O_BINARY);
		oldout = _setmode(_fileno(stdin), O_BINARY);
	}

	~BinaryMode()
	{
		_setmode(_fileno(stdout), oldin);
		_setmode(_fileno(stdin), oldout);
	}
};
#endif // _WIN32

struct Parameters
{
	char* inputFile{ nullptr };
	int wrap{ 75 };
	bool decode{ false };
};

static constexpr uint32_t alphabetSize{ 85 };
int Encode(std::istream& instream, size_t wrap);
int Decode(std::istream& instream);
Parameters ParseArgs(int argc, char** argv);

int main(int argc, char** argv) try
{
#ifdef _WIN32
	BinaryMode engage;
#endif
	
	std::ios::sync_with_stdio(false); // toivotaan, että tämä nopeuttaa aiheuttamatta haittaa
	std::ifstream infile;
	Parameters args = ParseArgs(argc, argv);

	if ( args.inputFile )
	{
		infile.open(args.inputFile, std::ios::binary);
		if ( !infile.good() )
		{
			std::cerr << "Unable to open input file.";
			return 1;
		}
	}

	std::istream& instream{ args.inputFile ? infile : std::cin };

	if ( args.decode )
		return Decode(instream);
	else
		return Encode(instream, args.wrap);
}
catch ( std::string& v )
{
	std::cout.flush();
	std::cerr << v << '\n';
	return 1;
}
catch(const std::exception& e)
{
	std::cout.flush();
	std::cerr << "Exception: " << e.what() << '\n';
	return 1;
}

Parameters ParseArgs(int argc, char** argv)
{
	Parameters retval;
	std::string input;

	for ( int i = 1; i < argc; ++i )
	{
		input = argv[i];

		if ( input == "-d" || input == "--decode" )
		{
			retval.decode = true;
		}
		else if ( input == "-w" || input == "--wrap" )
		{
			bool error{ argc <= i + 1 };
			if ( !error )
			{
				try
				{
					retval.wrap = std::max(0, std::stoi(argv[++i]));
				}
				catch(const std::exception&)
				{
					error = true;
				}
			}
			if ( error )
				throw std::string{ "Couldn't parse wrap parameter." };
		}
		else if ( input.size() > 0 && input[0] == '-' )
		{
			throw (std::string{ "Unknown switch \"" } + input + "\".");
		}
		else
		{
			retval.inputFile = argv[i];
		}
	}

	return retval;
}

int Encode(std::istream& instream, size_t wrap)
{
	size_t charsOnThisLine{ 0 };

	auto OutputCharacter = [wrap, &charsOnThisLine](char c)
	{
		if ( (wrap != 0) && (charsOnThisLine == wrap) )
		{
			std::cout << '\n';
			charsOnThisLine = 0;
		}
		std::cout << c;
		++charsOnThisLine;
	};

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
				OutputCharacter('z');
				continue;
			}

			if ( word == 0x20202020 )
			{
				OutputCharacter('y');
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
				OutputCharacter(output[j]);
		}
	}

	std::cout.flush();
	return 0;
}


char* DecodeXY(char* start, const char* const end)
{
	static constexpr char zeros[4]{0,0,0,0};
	static constexpr char spaces[4]{0x20,0x20,0x20,0x20};
	
	bool again{ 1 };
	while ( again && (start != end) )
	{
		again = 0;

		if ( *start == 'x' )
		{
			//std::cout.write(zeros, 4); // hah hah. ei toimi.
			++start;
			again = 1;
		}
		else if ( *start == 'y' )
		{
			std::cout.write(spaces, 4);
			++start;
			again = 1;
		}
	}

	return start;
}

char* Decode5(char* buf, size_t amount = 4)
{
	uint64_t word{ 0 };
	uint64_t powers[5]{ alphabetSize*alphabetSize*alphabetSize*alphabetSize,
	                    alphabetSize*alphabetSize*alphabetSize,
	                    alphabetSize*alphabetSize,
	                    alphabetSize,
	                    1 };
	
	for (size_t i = 0; i < 5; ++i)
	{
		char c{ *buf++ };
		if ( c == 'z' || c == 'y' )
			throw std::string{ "Decoding error: 'z' or 'y' in the middle of a group." };

		word += powers[i] * (c - 33);
	}
	
	if ( word > UINT32_MAX )
		throw std::string{ "Decoding error: Over-big group." };

	for ( size_t i = 0; i < amount; ++i )
	{
		std::cout.put(char(0xff & (word >> 24)));
		word <<= 8;
	}

	return buf;
}

char* DiscardInvalids(char* start, char* const end)
{
	while ( (start != end) && ((*start == 'x') || (*start == 'y') || ((33 <= *start) && (*start < 33 + alphabetSize))) )
		++start;

	char* cursor{ start };

	while ( start != end )
	{
		if ( ((33 <= *start) && (*start < 33 + alphabetSize)) || (*start == 'x') || (*start == 'y') )
			*cursor++ = *start;
		// else
		// 	std::cerr << "[skip]";
		
		++start;
	}

	return cursor;
}

int Decode(std::istream& instream)
{
	static constexpr size_t bufferSize{ 100 };
	char buffer[bufferSize];
	long long leftovers{ 0 };

	while ( instream.good() )
	{
		instream.read(buffer + leftovers, bufferSize - leftovers);
		size_t bytesRead = instream.gcount();
		if ( bytesRead == 0 )
			continue; // break saattaisi olla oikeampi
		
		char* const end{ DiscardInvalids(buffer, buffer + leftovers + bytesRead) };
		char* ptr{ DecodeXY(buffer, end) };

		while ( ptr + 4 < end)
		{
			ptr = Decode5(ptr);
			ptr = DecodeXY(ptr, end);
		}

		// copy leftovers to start of buffer
		leftovers = end - ptr;
		for (int i = 0; i < leftovers; ++i)
			buffer[i] = ptr[i];
	}

	// Deal with leftovers
	if ( leftovers > 0 )
	{
		for (size_t i = 0; i < 4; ++i) // pad with the last letter
			buffer[leftovers + i] = 33 + alphabetSize - 1;

		Decode5(buffer, leftovers - 1);
	}

	std::cout.flush();
	return 0;
}