

#include "RC2LangHandler.h"

//TODO: support japanese maybe?

#include <vector>
#include <iostream>
#include <iomanip>

using std::endl;
using std::cerr;
using std::size_t;

const char* const Rac2ToUTF8LUT[] = {
	NULL, "\n", NULL, NULL, NULL, NULL, NULL, NULL,				//0x00-0x07
	"{8}", "{9}", "{A}", "{B}", "{C}", "{D}", "{E}", "{F}",		//0x08-0x0F | Colors
	"&", "@", "^", "#", "{L1}", "{R1}", "{L2}", "{R2}",			//0x10-0x17 | Buttons part 1
	"{RSTICK}", "{LSTICK}", NULL, NULL, NULL, NULL, NULL, NULL,	//0x18-0x1F | Buttons part 2
	" ", "!", "\"", "#", "™", "%", "&", "'",					//0x20-0x27 | Visible ASCII part 1
	"(", ")", NULL , "+", ",", "-", ".", "/",					//0x28-0x2F | part 2
	"0", "1", "2", "3", "4", "5", "6", "7",						//0x30-0x37 | part 3
	"8", "9", ":", ";", "<", "" , ">", "?",						//0x38-0x3F | part 4 /*HACK - eat = for italian*/
	"@", "A", "B", "C", "D", "E", "F", "G",						//0x40-0x47 | part 5
	"H", "I", "J", "K", "L", "M", "N", "O",						//0x48-0x4F | part 6
	"P", "Q", "R", "S", "T", "U", "V", "W",						//0x50-0x57 | part 7
	"X", "Y", "Z", "[", NULL, "]", "^", "_",					//0x58-0x5F | part 8
	NULL , "a", "b", "c", "d", "e", "f", "g",					//0x60-0x67 | part 9
	"h", "i", "j", "k", "l", "m", "n", "o",						//0x68-0x6F | part 10
	"p", "q", "r", "s", "t", "u", "v", "w",						//0x70-0x77 | part 11
	"x", "y", "z", NULL, NULL, NULL, NULL, NULL,				//0x78-0x7F | part 12
	"Á", "À", "Â", "Ä", "á", "à", "â", "ä",						//0x80-0x87 | Accents part 1
	"É", "È", "Ê", "Ë", "é", "è", "ê", "ë",						//0x88-0x8F | part 2
	"Í", "Ì", "Î", "Ï", "í", "ì", "î", "ï",						//0x90-0x97 | part 3
	"Ó", "Ò", "Ô", "Ö", "ó", "ò", "ô", "ö",						//0x98-0x9F | part 4
	"Ú", "Ù", "Û", "Ü", "ú", "ù", "û", "ü",						//0xA0-0xA7 | part 5
	"Ñ", "ñ", "Ç", "ç", "ß", "…", "¡", "¿",						//0xA8-0xAF | part 6
	"œ", NULL, NULL, NULL, NULL, NULL, NULL, NULL,				//0xB0-0xB7 | part 7
	//HACK - translators sometimes use non-existent characters; silently swallow by replacing with nothingness
	NULL, NULL, "", "", //0xB8-0xBB - eat 0xBA (°) for german/spanish, eat 0xBB (») for french
};
#define LUT_TABLE_SIZE (sizeof(Rac2ToUTF8LUT) / sizeof(char*))

static_assert(LUT_TABLE_SIZE-1 == 0xBB, "Bad LUT");

/*
struct StringEntryTable {
	uint32_t offset; //From start of file
	uint32_t stringId;
	char*	 string; //Gets calculated on load - set to 0xFFFFFFFF
	uint32_t unk;    //Seems unused (for padding?) - set to 0
};

struct LangFile {
	uint32_t numEntries; //Size of header table
	uint32_t fileSize; //Size of file, including this header
	StringEntryTable headerTable[numEntries];
}

All strings are padded for seemingly no reason???
Game doesn't use the following characters:
ù = [ \ ] _ 
*/

//Game doesn't use the following characters:
//* = [ \ ] _ ` { | } ~
//%% in printf not allowed?

bool Rac2ToUTF8String(const char* src, std::string& dst) {
	for (size_t i = 0; *src != '\0'; src++, i++) {
		unsigned char ch = *((unsigned char*)src);
		if ((ch < LUT_TABLE_SIZE) && (Rac2ToUTF8LUT[ch] != NULL)) {
			dst += Rac2ToUTF8LUT[ch];
			//cerr << Rac2ToUTF8LUT[ch];
		} else {
			cerr << "Invalid character 0x" << std::hex << (unsigned)ch << " at position " << i << "." << endl;
			return false;
		}
	}
	return true;
}

void RC2LangHandler::parseLangFile(const char* langFileBuffer, std::ofstream& output, uint32_t (*read32)(const char* addr), bool japanese) {
	if (japanese) {
		cerr << "Japanese is not supported.";
		return;
	}
	
	size_t numEntries = read32(langFileBuffer);
	size_t fileSize = read32(langFileBuffer + 4);

	const char* const entriesTable = langFileBuffer + 8;
	auto getOffsetForEntry = [&](size_t entry) {
		return (size_t)read32(&entriesTable[entry * 16]);
	};

	auto getStringIdForEntry = [&](size_t entry) {
		return (size_t)read32(&entriesTable[entry * 16 + 4]);
	};

	for (size_t i = 0; i < numEntries; i++) {
		const size_t stringOffset = getOffsetForEntry(i);
		const size_t stringId = getStringIdForEntry(i);
		const char* fileString = &langFileBuffer[stringOffset];
		std::string convertedString;

		if (!Rac2ToUTF8String(fileString, convertedString)) {
			cerr << "Failed to convert string 0x" << std::hex << stringId << " at offset 0x" << stringOffset << "." << endl;
		} else {
			output << "{0x" << std::hex << std::setw(4) << std::setfill('0') << stringId << "}" << convertedString << "~" << endl;
		}
	}
}

typedef struct _String {
	size_t id;
	std::vector<char> str;
} String;

void RC2LangHandler::emitLangFile(const char* langFileBuffer, std::ofstream& output, bool japanese) {
	std::vector<String> strings;
	strings.reserve(8000); //RAM usage go brrr ig

	while (/* didn't reach EOF */ 1) {
		while (*langFileBuffer != '{' && *langFileBuffer != '\0') {
			langFileBuffer++;
		}

		if (*langFileBuffer == '\0') {
			break;
		}

		
	}
}