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
			std::cerr << "base85: Failed to set stdout to binary mode.\n";
		if ( oldin == -1 )
			std::cerr << "base85: Failed to set stdin to binary mode.\n";
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
	int wrap{ 75 };
	bool decode{ false };
	bool help{ false };
};

static constexpr uint32_t alphabetSize{ 85 };

Parameters ParseArgs(int argc, char** argv);
void Encode(std::istream& instream, size_t wrap);
void Decode(std::istream& instream);
unsigned char* DiscardInvalids(unsigned char* start, unsigned char* const end);
unsigned char* Decode5(unsigned char* buf, size_t amount = 4);
unsigned char* DecodeZY(unsigned char* start, const unsigned char* const end);
void DisplayHelp();

int main(int argc, char** argv) try
{
	Parameters args = ParseArgs(argc, argv);
	if ( args.help )
	{
		DisplayHelp();
		return 0;
	}

	std::ios::sync_with_stdio(false); // toivotaan, että tämä nopeuttaa aiheuttamatta haittaa

#ifdef _WIN32
	BinaryMode engage;
#endif
	
	std::ifstream infile;

	if ( args.inputFile )
	{
		infile.open(args.inputFile, std::ios::binary);
		if ( !infile.good() )
			throw std::string{ "Unable to open input file." };
	}

	std::istream& instream{ args.inputFile ? infile : std::cin };

	if ( args.decode )
		Decode(instream);
	else
		Encode(instream, args.wrap);
	
	return 0;
}
catch ( std::ios_base::failure f )
{
	std::cerr << "base85: I/O error: " << f.what() << '\n';
	return 1;
}
catch ( std::string& v )
{
	std::cerr << "base85: " << v << '\n';
	return 1;
}
catch(const std::exception& e)
{
	std::cerr << "base85: An unanticipated exception occurred: " << e.what() << '\n';
	return 1;
}
catch( ... )
{
	std::cerr << "base85: An unknown exception occurred!\n";
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
		else if ( input == "-h" || input == "--help" || input == "--halp" || input == "-?"
#ifdef _WIN32
		|| input == "/?"
#endif
		) {
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
			throw (std::string{ "Unknown switch \"" } + input + "\"");
		}
		else
		{
			retval.inputFile = argv[i];
		}
	}

	return retval;
}

void Encode(std::istream& instream, size_t wrap)
{
	size_t charsOnThisLine{ 0 };

	auto WrappedPrint = [wrap, &charsOnThisLine](unsigned char c)
	{
		if ( (wrap != 0) && (charsOnThisLine == wrap) )
		{
#ifdef _WIN32
			std::cout.put('\r').put('\n');
#else
			std::cout.put('\n');
#endif
			charsOnThisLine = 0;
		}
		std::cout.put(c);
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

	std::cout << '\n';
	std::cout.flush();
}

void Decode(std::istream& instream)
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
		unsigned char* ptr{ DecodeZY(buffer, end) };

		while ( ptr + 4 < end)
		{
			ptr = Decode5(ptr);
			ptr = DecodeZY(ptr, end);
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

		Decode5(buffer, leftovers - 1);
	}

	std::cout.flush();
}

unsigned char* DiscardInvalids(unsigned char* start, unsigned char* const end)
{
	while ( (start != end) && ((*start == 'z') || (*start == 'y') || ((33 <= *start) && (*start < 33 + alphabetSize))) )
		++start;

	unsigned char* cursor{ start };

	while ( start != end )
	{
		if ( ((33 <= *start) && (*start < 33 + alphabetSize)) || (*start == 'z') || (*start == 'y') )
			*cursor++ = *start;

		++start;
	}

	return cursor;
}

unsigned char* DecodeZY(unsigned char* start, const unsigned char* const end)
{
	static constexpr unsigned char zeros[4]{0,0,0,0};
	static constexpr unsigned char spaces[4]{0x20,0x20,0x20,0x20};
	
	bool again{ 1 };
	while ( again && (start != end) )
	{
		again = 0;

		if ( *start == 'z' )
		{
			std::cout.write(reinterpret_cast<const char*>(zeros), 4);
			++start;
			again = 1;
		}
		else if ( *start == 'y' )
		{
			std::cout.write(reinterpret_cast<const char*>(spaces), 4);
			++start;
			again = 1;
		}
	}

	return start;
}

unsigned char* Decode5(unsigned char* buf, size_t amount)
{
	uint64_t word{ 0 };
	
	for (size_t i = 0; i < 5; ++i)
	{
		unsigned char c{ *buf++ };
		if ( c == 'z' || c == 'y' )
			throw std::string{ "Decoding error: 'z' or 'y' in the middle of a group." };

		word = word * alphabetSize + c - 33u;
	}

	if ( word > UINT32_MAX )
		throw std::string{ "Decoding error: Over-big group." };

	for ( size_t i = 0; i < amount; ++i )
	{
		std::cout.put(static_cast<unsigned char>(0xffu & (word >> 24)));
		word <<= 8;
	}

	return buf;
}

void DisplayHelp()
{
	std::cout <<
	"Encode or decode base85 to stdout. If no filename is provided then stdin is\n"
	"used for input. Usage:\n\n"

	"base85 [OPTIONS] [INPUTFILE]\n\n"

	"Options:\n"
	"  -d, --decode      Self explanatory.\n"
	"  -w N, --wrap N    Split encoding output to lines N characters long.\n"
	"  -h, -?, --help    Display this help.\n\n"

	"When decoding a binary file please remember to redirect the output to a file."
	<< std::endl;
}
