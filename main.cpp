#include <iostream>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>

struct BinaryMode
{
	int oldin{ -1 }, oldout{ -1 };

	BinaryMode()
	{
		oldout = _setmode(_fileno(stdout), _O_BINARY);
		oldin = _setmode(_fileno(stdin), _O_BINARY);

		if ( oldout == -1 )
			std::cerr << "Failed to set stdout to binary mode.\n";
		if ( oldin == -1 )
			std::cerr << "Failed to set stdin to binary mode.\n";
	}

	~BinaryMode()
	{
		if ( oldout != -1 ) _setmode(_fileno(stdout), oldout);
		if ( oldin != -1 ) _setmode(_fileno(stdin), oldin);
	}
};
#endif // _WIN32

struct Parameters
{
	char* inputFile{ nullptr };
	char* outputFile{ nullptr };
	int wrap{ 75 };
	bool decode{ false };
	bool help{ false };
};

static constexpr uint32_t alphabetSize{ 85 };

Parameters ParseArgs(int argc, char** argv);
void Encode(std::istream& instream, size_t wrap, std::ostream& outstream);
void Decode(std::istream& instream, std::ostream& outstream);
unsigned char* DiscardInvalids(unsigned char* start, unsigned char* const end);
unsigned char* Decode5(unsigned char* buf, std::ostream& outstream, size_t amount = 4);
unsigned char* DecodeZY(unsigned char* start, const unsigned char* const end, std::ostream& outstream);
void DisplayHelp(const char*);

int main(int argc, char** argv) try
{
	Parameters args = ParseArgs(argc, argv);
	if ( args.help )
	{
		DisplayHelp(argv[0]);
		return 0;
	}

	std::ios::sync_with_stdio(false); // toivotaan, että tämä nopeuttaa aiheuttamatta haittaa

#ifdef _WIN32
	BinaryMode engage;
#endif
	
	std::ifstream infile;
	std::ofstream outfile;

	if ( args.inputFile )
	{
		infile.open(args.inputFile, std::ios::binary);
		if ( !infile.good() )
		{
			std::cerr << "Unable to open input file.\n";
			return 1;
		}
	}

	if ( args.outputFile )
	{
		if ( args.decode )
			outfile.open(args.outputFile, std::ios::binary);
		else
			outfile.open(args.outputFile);

		if ( !outfile.good() )
		{
			std::cerr << "Unable to open output file.\n";
			return 1;
		}
	}

	std::istream& instream{ args.inputFile ? infile : std::cin };
	std::ostream& outstream{ args.outputFile ? outfile : std::cout };

	if ( args.decode )
		Decode(instream, outstream);
	else
		Encode(instream, args.wrap, outstream);
	
	return 0;
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
		else if ( input == "-h" || input == "--help" )
		{
			retval.help = true;
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
				throw std::string{ "Couldn't parse the --wrap parameter." };
		}
		else if ( input.size() > 0 && input[0] == '-' )
		{
			throw (std::string{ "Unknown switch \"" } + input + "\".");
		}
		else if ( retval.inputFile )
		{
			retval.outputFile = argv[i];
		}
		else
		{
			retval.inputFile = argv[i];
		}
	}

	return retval;
}

void Encode(std::istream& instream, size_t wrap, std::ostream& outstream)
{
	size_t charsOnThisLine{ 0 };

	auto WrappedPrint = [wrap, &charsOnThisLine, &outstream](unsigned char c)
	{
		if ( (wrap != 0) && (charsOnThisLine == wrap) )
		{
			outstream << '\n';
			charsOnThisLine = 0;
		}
		outstream.put(c);
		++charsOnThisLine;
	};

	while ( instream.good() )
	{
		unsigned char buffer[4]{0,0,0,0};
		instream.read(reinterpret_cast<char*>(buffer), 4);
		size_t bytesRead = instream.gcount();

		if ( bytesRead > 0 )
		{
			uint32_t word = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];

			if ( word == 0 )
			{
				WrappedPrint('z');
				continue;
			}

			if ( word == 0x20202020 )
			{
				WrappedPrint('y');
				continue;
			}

			unsigned char output[5];
			output[4] = 33u + word % alphabetSize;
			for ( int j = 3; j >= 0; --j )
			{
				word /= alphabetSize;
				output[j] = 33u + word % alphabetSize;
			}

			size_t writeThisMany = bytesRead + 1;
			for ( size_t j = 0; j < writeThisMany; ++j )
				WrappedPrint(output[j]);
		}
	}

	outstream << '\n';
	outstream.flush();
}

unsigned char* DecodeZY(unsigned char* start, const unsigned char* const end, std::ostream& outstream)
{
	static constexpr unsigned char zeros[4]{0,0,0,0};
	static constexpr unsigned char spaces[4]{0x20,0x20,0x20,0x20};
	
	bool again{ 1 };
	while ( again && (start != end) )
	{
		again = 0;

		if ( *start == 'z' )
		{
			outstream.write(reinterpret_cast<const char*>(zeros), 4);
			++start;
			again = 1;
		}
		else if ( *start == 'y' )
		{
			outstream.write(reinterpret_cast<const char*>(spaces), 4);
			++start;
			again = 1;
		}
	}

	return start;
}

unsigned char* Decode5(unsigned char* buf, std::ostream& outstream, size_t amount)
{
	uint64_t word{ 0 };
	static constexpr uint64_t powers[5]{ alphabetSize*alphabetSize*alphabetSize*alphabetSize,
	                                     alphabetSize*alphabetSize*alphabetSize,
	                                     alphabetSize*alphabetSize,
	                                     alphabetSize,
	                                     1 };
	
	for (size_t i = 0; i < 5; ++i)
	{
		unsigned char c{ *buf++ };
		if ( c == 'z' || c == 'y' )
			throw std::string{ "Decoding error: 'z' or 'y' in the middle of a group." };

		word += powers[i] * (c - 33u);
	}

	if ( word > UINT32_MAX )
		throw std::string{ "Decoding error: Over-big group." };

	for ( size_t i = 0; i < amount; ++i )
	{
		outstream.put(static_cast<unsigned char>(0xffu & (word >> 24)));
		word <<= 8;
	}

	return buf;
}

unsigned char* DiscardInvalids(unsigned char* start, unsigned char* const end)
{
	while ( (start != end) && ((*start == 'z') || (*start == 'y') || ((33u <= *start) && (*start < 33u + alphabetSize))) )
		++start;

	unsigned char* cursor{ start };

	while ( start != end )
	{
		if ( ((33u <= *start) && (*start < 33u + alphabetSize)) || (*start == 'z') || (*start == 'y') )
			*cursor++ = *start;

		++start;
	}

	return cursor;
}

void Decode(std::istream& instream, std::ostream& outstream)
{
	static constexpr size_t bufferSize{ 100 };
	unsigned char buffer[bufferSize];
	long long leftovers{ 0 };

	while ( instream.good() )
	{
		instream.read(reinterpret_cast<char*>(buffer + leftovers), bufferSize - leftovers);
		size_t bytesRead = instream.gcount();
		if ( bytesRead == 0 )
			continue; // break saattaisi olla oikeampi
		
		unsigned char* const end{ DiscardInvalids(buffer, buffer + leftovers + bytesRead) };
		unsigned char* ptr{ DecodeZY(buffer, end, outstream) };

		while ( ptr + 4 < end)
		{
			ptr = Decode5(ptr, outstream);
			ptr = DecodeZY(ptr, end, outstream);
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
			buffer[leftovers + i] = 33u + alphabetSize - 1u;

		Decode5(buffer, outstream, leftovers - 1);
	}

	outstream.flush();
}

void DisplayHelp(const char*)
{
	std::cout <<
	"The first filename's the input file, the second the output.\n"
	"If no filename is provided stdin or stdout is used instead.\n"
	"Switches:\n"
	"  -d, --decode\n"
	"  -w, --wrap\n"
	"  -h, --help\n";
}
