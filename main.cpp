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

static bool BinaryModeGood;
#endif // _WIN32

struct Parameters
{
	char* inputFile{ nullptr };
	char* alphaFile{ nullptr };
	int wrap{ 75 };
	bool decode{ false };
	bool help{ false };
	bool version{ false };
	bool disableZ{ false };
	bool disableY{ false };
	bool z85{ false };
	bool ref{ false };
};

static Parameters Options;
static constexpr unsigned char AlphaEncDefault[]{ R"++(!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuzy)++" };
static constexpr unsigned char AlphaEncZ85[]{     R"++(0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#)++" };
static unsigned char AlphaEncCustom[87];
static const unsigned char* AlphaEnc{ AlphaEncDefault };
static int AlphaDec[256];

Parameters ParseArgs(int argc, char** argv);
void InitAlphabet(Parameters& args);
void Encode(std::istream& instream, size_t wrap);
void Decode(std::istream& instream);
unsigned char* DiscardInvalid(unsigned char* start, unsigned char* const end);
unsigned char* Decode5(unsigned char* buf, size_t amount = 4);
unsigned char* DecodeZY(unsigned char* start, const unsigned char* const end);
void DisplayHelp();

inline void DisplayVersion() { std::cout << "This executable was compiled on " __DATE__ " " __TIME__; }
inline void DisplayReferences() { std::cout << "https://en.wikipedia.org/wiki/Ascii85\nhttps://rfc.zeromq.org/spec/32/"; }


int main(int argc, char** argv) try
{
	Options = ParseArgs(argc, argv);
	
	if ( Options.help )
	{
		DisplayHelp();
		if ( Options.version || Options.ref )
			std::cout << "\n\n";
	}

	if ( Options.ref )
	{
		DisplayReferences();
		if ( Options.version )
			std::cout << "\n\n";
	}

	if ( Options.version )
	{
		DisplayVersion();
	}

	if ( Options.help || Options.version || Options.ref )
	{
		std::cout << std::endl;
		return 0;
	}

	InitAlphabet(Options);

	std::ios::sync_with_stdio(false); // toivotaan, että tämä nopeuttaa aiheuttamatta haittaa

#ifdef _WIN32
	BinaryMode engage;
	BinaryModeGood = engage.oldout != -1;
#endif
	
	std::ifstream infile;

	if ( Options.inputFile )
	{
		infile.open(Options.inputFile, std::ios::binary);
		if ( !infile.good() )
			throw (std::string{ "Unable to open input file \"" } + Options.inputFile + "\".");
	}

	std::istream& instream{ Options.inputFile ? infile : std::cin };

	if ( Options.decode )
		Decode(instream);
	else
		Encode(instream, Options.wrap);
	
	return 0;
}
catch ( const std::ios_base::failure& f )
{
	std::cerr << "base85: I/O error: " << f.what() << '\n';
	return 1;
}
catch ( const std::string& v )
{
	std::cerr << "base85: " << v << '\n';
	return 1;
}
catch ( const std::exception& e )
{
	std::cerr << "base85: An unanticipated exception occurred: " << e.what() << '\n';
	return 1;
}
catch ( ... )
{
	std::cerr << "base85: An unknown exception occurred!\n";
	return 1;
}

Parameters ParseArgs(int argc, char** argv)
{
	Parameters retval;
	std::string input;
	bool takingOpts{ true };

	auto trySetInputfile = [&retval, warned = false](char* arg) mutable
	{
		if ( !retval.inputFile )
		{
			retval.inputFile = arg;
		}
		else if ( !warned )
		{
			std::cerr << "base85: Several input files specified. Using only the first one.\n";
			warned = true;
		}
	};

	for ( int i = 1; i < argc; ++i )
	{
		input = argv[i];
		if ( input.empty() )
			throw std::string{ "WTF? Somehow an empty string snuck in as a command line argument." };

		if ( takingOpts )
		{
			if ( input == "-d" || input == "--decode" )
			{
				retval.decode = true;
			}
			else if ( input == "-v" || input == "--version" )
			{
				retval.version = true;
			}
			else if ( input == "--" )
			{
				takingOpts = false;
			}
			else if ( input == "-z" )
			{
				retval.disableZ = true;
			}
			else if ( input == "-y" )
			{
				retval.disableY = true;
			}
			else if ( input == "--z85" )
			{
				retval.z85 = true;
			}
			else if ( input == "--ref" )
			{
				retval.ref = true;
			}
			else if ( input == "-h" || input == "--help" || input == "--halp"
#ifdef _WIN32
			|| input == "/?" || input == "-?"
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
					catch ( const std::exception& )
					{
						error = true;
					}
				}
				if ( error )
					throw std::string{ "Couldn't parse the wrap parameter." };
			}
			else if ( input == "-a" || input == "--alphabet" )
			{
				if ( i + 1 < argc )
					retval.alphaFile = argv[++i];
				else
					throw std::string{ "No file name specified for custom alphabet." };
			}
			else if ( input.size() > 0 && input[0] == '-' )
			{
				throw (std::string{ "Unknown switch \"" } + input + "\"");
			}
			else
			{
				trySetInputfile(argv[i]);
			}
		}
		else
		{
			trySetInputfile(argv[i]);
		}
	}

	return retval;
}

void InitAlphabet(Parameters& args)
{
	if ( args.z85 )
	{
		AlphaEnc = AlphaEncZ85;
		args.disableY = true;
		args.disableZ = true;
	}
	else if ( args.alphaFile )
	{
		AlphaEnc = AlphaEncCustom;
		std::ifstream alphfile(args.alphaFile, std::ios::binary);
		if ( !alphfile.good() )
				throw (std::string{ "Unable to open alphabet file \"" } + args.alphaFile + "\".");
		
		alphfile.read(reinterpret_cast<char*>(AlphaEncCustom), 87);
		auto bytesRead = alphfile.gcount();
		if ( bytesRead < 85 )
			throw std::string{ "Not enough characters in the alphabet file." };
		if ( bytesRead < 87 )
			args.disableY = true;
		if ( bytesRead < 86 )
			args.disableZ = true;
	}

	auto setDecoder = [abort = args.decode](int index)
	{
		if ( AlphaDec[AlphaEnc[index]] == -1 )
		{
			AlphaDec[AlphaEnc[index]] = index;
		}
		else
		{
			std::string warning{ (std::string{ "Duplicate alphabet entry '" } + char(AlphaEnc[index]) + "'. Not gonna decode." ) };
			if ( abort )
				throw warning;
			else
				std::cerr << "base85: " << warning << '\n';
		}
	};

	for ( int i = 0; i < 256; ++i )
		AlphaDec[i] = -1;
	for ( int i = 0; i < 85; ++i )
		setDecoder(i);
	if ( !args.disableZ )
		setDecoder(85);
	if ( !args.disableY )
		setDecoder(86);
}

void Encode(std::istream& instream, size_t wrap)
{
	auto WrappedPrint = [wrap, charsOnThisLine = 0ull](unsigned char c) mutable
	{
		if ( (wrap != 0) && (charsOnThisLine == wrap) )
		{
#ifdef _WIN32
			if ( BinaryModeGood )
				std::cout.put('\r');
#endif
			std::cout.put('\n');
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

			if ( !Options.disableZ && word == 0 )
			{
				WrappedPrint(AlphaEnc[85]);
				continue;
			}

			if ( !Options.disableY && word == 0x20202020 )
			{
				WrappedPrint(AlphaEnc[86]);
				continue;
			}

			unsigned char output[5];
			output[4] = AlphaEnc[word % 85];
			for ( int j = 3; j >= 0; --j )
			{
				word /= 85;
				output[j] = AlphaEnc[word % 85];
			}

			for ( size_t j = 0; j < bytesRead + 1; ++j )
				WrappedPrint(output[j]);
		}
	}

#ifdef _WIN32
	if ( BinaryModeGood )
		std::cout.put('\r');
#endif
	std::cout.put('\n');
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
			continue;
		
		unsigned char* const end{ DiscardInvalid(buffer + leftovers, buffer + leftovers + bytesRead) };
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
			buffer[leftovers + i] = AlphaEnc[84];

		Decode5(buffer, leftovers - 1);
	}

	std::cout.flush();
}

unsigned char* DiscardInvalid(unsigned char* start, unsigned char* const end)
{
	while ( (start != end) && (AlphaDec[*start] != -1) )
		++start;

	unsigned char* cursor{ start };

	while ( start != end )
	{
		if ( AlphaDec[*start] != -1 )
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

		if ( !Options.disableZ && (*start == AlphaEnc[85]) )
		{
			std::cout.write(reinterpret_cast<const char*>(zeros), 4);
			++start;
			again = 1;
		}
		else if ( !Options.disableY && (*start == AlphaEnc[86]) )
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
		if ( (!Options.disableZ && c == AlphaEnc[85]) || (!Options.disableY && c == AlphaEnc[86]) )
			throw (std::string{ "Decoding error: Abbreviation '" } + char(c) + "' in the middle of a group.");

		word = word * 85 + AlphaDec[c];
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
"To encode or decode Base85/Ascii85 to stdout from a file or stdin.\n\n"

"Usage: base85 [Option]... [FILENAME] [Option]...\n\n"

"Options:\n"
"  -d,   --decode  Decode instead of encoding.\n"
"  -w N, --wrap N  Insert a line break for every N characters of encoded\n"
"                  output proper. Default " << Parameters{}.wrap <<
                                           ". Use 0 to disable.\n"
"  -z              Disable the 'z' abbreviation for groups of zeroes.\n"
"  -y              Disable the 'y' abbreviation for groups of spaces.\n"
"                  Note: If either of these have been disabled during encoding,\n"
"                        decoding will still work with them enabled, too.\n"
"  --z85           Use the Z85 alphabet. Overrides -a, forces -z -y, but\n"
"                  ignores the Z85 spec regarding input and output lengths.\n"
"  --              End of options. All arguments which come after this are\n"
"                  to be considered file names.\n\n"

"  -a FILE, --alphabet FILE  Read custom alphabet from file FILE. Has to be at\n"
"                            least 85 bytes long. Bytes 86 and 87, if existant,\n"
"                            will be used for the 'z' and 'y' abbreviations,\n"
"                            respectively. If there are duplicate entries,\n"
"                            then decoding will not be attempted.\n\n"
#ifdef _WIN32
"  -?, -h, --help     This help\n"
"  -v,     --version  Version info\n"
"          --ref      References\n\n"
#else
"  -h, --help     This help\n"
"  -v, --version  Version info\n"
"      --ref      References\n\n"
#endif

"If no input FILENAME is provided, then stdin is used for input. If no custom\n"
"alphabet nor Z85 are specified, the standard one from '!' to 'u' in ASCII\n"
"order plus 'z' and 'y' will be used. During decoding all bytes not in the\n"
"alphabet will be ignored (i.e. skipped). If you want the output written to\n"
"a file, then please use the redirection operator \">\" appropriately.";
}
