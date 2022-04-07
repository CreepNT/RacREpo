// LangTool.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <fstream>
#include <cstdio>

#include "RC2LangHandler.h"

using namespace std;

//compare string with literal safely
#define STRINGCMP(tgt, lit) strncmp(tgt, lit, sizeof(lit))

enum class ToolMode {
	UNPACK,
	REPACK,
	INVALID
};

ostream& operator<<(ostream& os, const ToolMode& mode) {
	switch (mode) {
	case ToolMode::UNPACK:
		os << "Unpack";
		break;
	case ToolMode::REPACK:
		os << "Repack";
		break;
	case ToolMode::INVALID:
	default:
		os << "INVALID";
		break;
	}
	return os;
}



static const char* toolName = "LangTool";

uint32_t read32BE(const char* buf) {
	const unsigned char* addr = reinterpret_cast<const unsigned char*>(buf);
	return (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | (addr[3]);;
}

uint32_t read32LE(const char* buf) {
	const unsigned char* addr = reinterpret_cast<const unsigned char*>(buf);
	return (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | (addr[0]);
}

void printUsage() {
	cerr << "USAGE: " << toolName << " <-u|-p> <input> [output]" << endl << endl;
	cerr << "input\t- path to the input file" << endl;
	cerr << "output\t- path to the output file" << endl;
	cerr << "  If output is not specified, the output file will be '<input>.txt' if unpacking or '<input>.bin' if packing." << endl << endl;
	cerr << "Available operations (only specify one):" << endl;
	cerr << "\t-u\t- Unpack language file" << endl;
	cerr << "\t-p\t- Repack language file" << endl;
	cerr << endl << "Available options:" << endl;
	cerr << "\t-ps3\t- Parse a PS3 file (big-endian mode)" << endl;
}

bool parseArgs(int argc, char** argv, ToolMode& parsedMode, FILE*& inputFd, size_t& inputSize, ofstream& outputFd, uint32_t (*&read32)(const char*)) {
	const char* input = NULL;
	std::string output;

	for (int i = 1; i < argc; i++) {
		if (STRINGCMP(argv[i], "-u") == 0) {
			if (parsedMode != ToolMode::INVALID) {
				cerr << toolName << ": conflicting operation types" << endl;
				printUsage();
				return false;
			}
			parsedMode = ToolMode::UNPACK;
		} else if (STRINGCMP(argv[i], "-p") == 0) {
			if (parsedMode != ToolMode::INVALID) {
				cerr << toolName << ": more than one operation provided" << endl;
				printUsage();
				return false;
			}
			parsedMode = ToolMode::REPACK;
		} else if (STRINGCMP(argv[i], "-ps3") == 0) {
			read32 = read32BE;
		} else if (argv[i][0] == '-') { //always safe
			cerr << toolName << ": unknown option '" << argv[i] << "'" << endl;
			printUsage();
			return false;
		} else if (input == NULL) {
			input = argv[i];
		} else if (output.length() == 0) {
			output = argv[i];
		} else {
			cerr << toolName << ": more than one operation provided" << endl;
			printUsage();
			return false;
		}
	}

	if (read32 == nullptr) {
		read32 = read32LE;
	}

	if (input == NULL) {
		cerr << toolName << ": missing required argument <input>" << endl;
		printUsage();
		return false;
	}

	if (parsedMode == ToolMode::INVALID) {
		cerr << toolName << ": missing required 'mode' argument" << endl;
		printUsage();
		return false;
	}

	if (output.length() == 0) { //output = input + ".EXT
		if (parsedMode == ToolMode::UNPACK) {
			output += ".txt";
		} else if (parsedMode == ToolMode::REPACK) {
			output += ".bin";
		}
	}

	//TODO make crossplatform
	struct _stat64 instat;
	if (_stat64(input, &instat) != 0) {
		cerr << toolName << ": failed to get stats for '" << input << "': " << errno << endl;
		return false;
	}
	inputSize = (size_t)instat.st_size;

	inputFd = fopen(input, "rb");
	if (!inputFd) {
		cerr << toolName << ": failed to open '" << input << "': " << errno << endl;
		return false;
	}

	outputFd.open(output.c_str(), std::ios::out | std::ios::binary);
	if (!outputFd.is_open()) {
		cerr << toolName << ": failed to open '" << output << "': " << errno << endl;
		(void)fclose(inputFd);
		return false;
	}
	
	cout << parsedMode << "ing '" << input << "' into '" << output << "'..." << endl;
	return true;
}

int main(int argc, char** argv) {
	cerr << "LangTool by CreepNT" << endl << endl;
	if (argc >= 1) {
		toolName = argv[0];
	}

	if (argc < 3) {
		cerr << toolName << ": not enough arguments" << endl;
		printUsage();
		return EXIT_SUCCESS;
	}

	//Parse arguments
	ToolMode mode = ToolMode::INVALID;
	FILE* input = NULL;
	ofstream output;
	size_t inputSize = 0;
	uint32_t(*read32_func)(const char*) = nullptr;

	if (!parseArgs(argc, argv, mode, input, inputSize, output, read32_func)) {
		return EXIT_FAILURE;
	}

	if (inputSize >= (16 * 1024 * 1024)) { //16MiB
		cerr << "File too large - exiting." << endl;
		return EXIT_FAILURE;
	}

	char* inputBuffer = (char*)malloc(inputSize);
	if (!inputBuffer) {
		cerr << "Failed to allocate memory for input file." << endl;
		fclose(input);
		return EXIT_FAILURE;
	}

	size_t readSize = fread(inputBuffer, 1, inputSize, input);
	fclose(input);
	if (readSize != inputSize) {
		cerr << "Read " << readSize << "bytes instead of expected " << inputSize << "!";
		return EXIT_FAILURE;
	}

	switch (mode) {
	case ToolMode::UNPACK: {
		RC2LangHandler::parseLangFile(inputBuffer, output, read32_func, false);
		break;
	}
	case ToolMode::REPACK: {
		RC2LangHandler::emitLangFile(inputBuffer, output, false);
	}
	}

	return 0;
}
