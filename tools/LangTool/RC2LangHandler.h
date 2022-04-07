#pragma once

#include <fstream>

//This name is kinda wrong, since the format is valid for Rac1-Rac3 (maybe even Deadlocked?)
//and should fully work for Rac1 and Rac2
//In ?Rac3? (it's in deadlocked for sure) Insomniac moved to some multibyte character scheme
//that needs something else
//this could be cleaned up by having a common header parsing function, then a 
//per-game (+ optionally per-locale for asia) "string->UTF-8" and reverse routine
//I'll clean that up some day, got some hw to do bro
namespace RC2LangHandler {
	/**
	 * @brief Parses a language file from a buffer into an output stream
	 * @param langFileBuffer Language file buffer
	 * @param output Output stream
	 * @param read32 Callback to read 32-bit-sized data (for endianness)
	 * @param japanese Is the input file japanese?
	*/
	void parseLangFile(const char* langFileBuffer, std::ofstream& output, uint32_t(*read32)(const char* addr), bool japanese);
	void emitLangFile(const char* langFileBuffer, std::ofstream& output, bool japanese);
};