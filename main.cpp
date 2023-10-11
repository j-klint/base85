#include <iostream>
#include <fstream>
#include <string>

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
	std::ios::sync_with_stdio(false); // toivotaan, että tämä nopeuttaa aiheuttamatta haittaa
	std::ifstream infile;

	Parameters args = ParseArgs(argc, argv);

	if ( args.inputFile )
	{
		infile.open(args.inputFile, std::ios::binary);
		if ( !infile.good() )
		{
			std::cerr << "Unable to open input file.\n";
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
	std::cerr << "Error: " << v;
	return 1;
}
catch(const std::exception& e)
{
	std::cerr << "Exception: " << e.what();
	return 1;
}

Parameters ParseArgs(int argc, char** argv)
{
	Parameters retval;
	std::string input;

	for ( int i = 1; i < argc; ++i )
	{
		input = argv[i];

		if ( input == "-d" )
		{
			retval.decode = true;
		}
		else if ( input == "-w" )
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
				throw std::string{ "Couldn't parse -w parameter." };
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

	return 0;
}

int Decode(std::istream& )//instream)
{
	std::cerr << "Decode function not done yet.\n";
	return 0;
}