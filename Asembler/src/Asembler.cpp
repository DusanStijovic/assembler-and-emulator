#include "Asembler.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <exception>

Asembler* Asembler::asembler = nullptr;
long Asembler::locationCounter = 0;
long Asembler::lineCounter = -1;
long SymbolEntry::nextSymbolNumber = 0;
long Asembler::currentSectionNumber = -1;
std::string Asembler::currentSectionName = "";
std::string Asembler::currentLine = "";
bool Asembler::isEnd = false;
std::string Asembler::equRealloc = "equRealloc";

std::map<std::string, SymbolEntry> Asembler::symbolTable;
std::map<std::string, SectionEntry>  Asembler::sectionTable;
std::map<std::string, EquEntry> Asembler::equTable;
std::map<std::string, unsigned char> Asembler::instructionsCode = {
	{"halt", 0}, { "iret",1 }, { "ret",2 }, { "int", 3 }, { "call",4 }, { "jmp", 5 }, { "jeq", 6 }, { "jne", 7 }, {"jgt", 8}, {"push", 9},
	{"pop", 10}, {"xchg", 11}, {"mov", 12}, {"add", 13}, {"sub", 14}, {"mul", 15}, {"div", 16}, {"cmp", 17}, {"not", 18}, {"and", 19},
	{"or", 20}, {"xor", 21}, {"test", 22}, {"shl", 23}, {"shr", 24}
};


bool Asembler::checkProgramCall(int argc, char* argv[]) {

	std::regex inputFile("^.*\\.s$");
	std::regex outputFile("^.*\\.o$");
	std::regex option("^-o$");
	switch (argc) {
	case Asembler::AsemblerOptions::USE_DEFAULT_FILE_NAME:
		if (std::regex_match(argv[1], inputFile)) {
			Asembler::makeAsembler(argv[1]);
			return true;
		}
		break;
	case Asembler::AsemblerOptions::FILE_NAME_GIVEN:
		if (std::regex_match(argv[1], option) && std::regex_match(argv[2], outputFile) && std::regex_match(argv[3], inputFile)) {
			Asembler::makeAsembler(argv[3], argv[2]);
			return true;
		}
		if (std::regex_match(argv[1], inputFile) && std::regex_match(argv[2], option) && std::regex_match(argv[3], outputFile)) {
			Asembler::makeAsembler(argv[1], argv[3]);
			return true;
		}
		break;

	}
	std::string message = "";
	for (int i = 0; i < argc; i++) {
		message += argv[i];
		message += " ";
	}
	throw ArgumentsFormatNotRigth(message);

}
Asembler* Asembler::getAsembler() {
	if (Asembler::asembler != nullptr) {
		return Asembler::asembler;
	}
	throw AsemblerNoTFound();
}

bool Asembler::checkIfGivenDirective(std::string assemblyLine, std::string directive, void (*obradiSimbol)(std::string param)) {
	std::regex global(directive);
	std::smatch matches;
	if (std::regex_match(assemblyLine, matches, global)) {
		std::regex globalSimbol("\\s*([a-zA-Z_][a-zA-Z_0-9]*|(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]{1,4})\\s*,?");
		std::string simbols = matches.str(1);
		while (std::regex_search(simbols, matches, globalSimbol)) {
			obradiSimbol(matches.str(1));
			simbols = matches.suffix().str();
		}
		return true;
	}
	return false;
}
std::string Asembler::checkifLabelAtBeginig(std::string assemblyLine) {
	std::regex label("^\\s*([a-zA-Z_][a-zA-Z_0-9]*):");
	std::smatch labelName;
	if (std::regex_search(assemblyLine, labelName, label)) {
		Asembler::processLabelDirective(labelName.str(1));
		return std::string(labelName.suffix().str());
	}
	return assemblyLine;
}
bool Asembler::checkForNothingInLine(std::string assemblyLine) {
	std::regex nothing("^\\s*$");
	if (std::regex_match(assemblyLine, nothing)) return true;
	return false;
}
bool Asembler::checkIfSection(std::string assemblyLine) {
	std::regex section("^\\s*\\.section\\s+([a-zA-Z_][a-zA-Z_0-9]*):\\s*$");
	std::smatch sectionName;
	if (std::regex_match(assemblyLine, sectionName, section)) {
		processSectionDirective(sectionName.str(1));
		return true;
	}
	std::regex trySection("^\\s*\\.section.*\\s*$");
	if (std::regex_match(assemblyLine, trySection)) throw LineNotRecognized(Asembler::currentLine, Asembler::lineCounter);

	return false;
}
bool Asembler::checkIfEqu(std::string assemblyLine) {
	std::regex equ("^\\s*\\.equ\\s+([a-zA-Z_][a-zA-Z_0-9]*)\\s*,\\s*((?:[a-zA-Z_][a-zA-Z_0-9]*|(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]{1,4})(?:\\s*(?:\\+|-)\\s*(?:[a-zA-Z_][a-zA-Z_0-9]*|[0-9]+|(?:0x|0X)[0-9a-fA-F]{1,4}|\\((?:\\+|-)[0-9]+\\)))*\\s*)\\s*$");
	std::smatch matches;
	if (std::regex_match(assemblyLine, matches, equ)) {
		std::regex equSimbol("\\s*(\\+|-)?\\s*([a-zA-Z_][a-zA-Z_0-9]*|(?:0x|0X)[0-9a-fA-F]+|[0-9]+|\\((?:\\+|-)[0-9]+\\))\\s*");
		std::string simbol = matches.str(1);
		std::string simbols = matches.str(2);
		std::vector<EquEntryPart> equParts;

		while (std::regex_search(simbols, matches, equSimbol)) {
			EquEntryPart equPart;
			if (matches.str(1) != "") equPart.sign = (matches.str(1).at(0) == '+' ? Asembler::PLUS : Asembler::MINUS);
			else equPart.sign = PLUS;
			std::regex complex("^\\s*\\(((?:\\+|-)[0-9]+)\\)\\s*$");
			std::smatch tryC;
			std::string number = matches.str(2);
			if (std::regex_match(number, tryC, complex)) {
				equPart.symbol = tryC.str(1);
			}
			else equPart.symbol = matches.str(2);
			equParts.push_back(equPart);
			simbols = matches.suffix().str();
		}
		processEquDirective(simbol, equParts);
		return true;
	}
	std::regex checkEqu("^\\s*\\.equ.*\\s*$");
	if (std::regex_match(assemblyLine, checkEqu)) throw LineNotRecognized(Asembler::currentLine, Asembler::lineCounter);
	return false;
}
bool Asembler::checkIfEnd(std::string assemblyLine) {
	std::regex end("^\\s*\\.end\\s*$");
	if (std::regex_match(assemblyLine, end)) {
		processEndDirective();
		return true;
	}
	std::regex checkEnd("^\\s*\\.end.*\\s*$");
	if (std::regex_match(assemblyLine, checkEnd)) throw LineNotRecognized(Asembler::currentLine, Asembler::lineCounter);
	return false;
}
bool Asembler::checkIfSkip(std::string assemblyLine) {
	std::regex skip("^\\s*\\.skip\\s+((?:\\+)?[0-9]+|(?:0x|0X)[0-9a-fA-F]{1,4})\\s*");
	std::smatch skipsimbol;

	if (std::regex_match(assemblyLine, skipsimbol, skip)) {
		processSkipDirective(skipsimbol.str(1));
		return true;
	}
	std::regex checkSkip("^\\s*\\.skip.*\\s*$");
	if (std::regex_match(assemblyLine, checkSkip)) throw LineNotRecognized(Asembler::currentLine, Asembler::lineCounter);
	return false;
}
bool Asembler::checkIfLineIsDirective(std::string assemblyLine) {
	std::string global = "^\\s*\\.global\\s+((?:\\s*[a-zA-Z_][a-zA-Z_0-9]*\\s*,)*\\s*[a-zA-Z_][a-zA-Z_0-9]*\\s*)\\s*$";
	if (checkIfGivenDirective(assemblyLine, global, Asembler::processGlobalDirective)) return true;
	std::regex checkGlobal("^\\s*\\.global.*\\s*$"); if (std::regex_match(assemblyLine, checkGlobal))  throw LineNotRecognized(Asembler::currentLine, Asembler::lineCounter);

	std::string externD = "^\\s*\\.extern\\s+((?:\\s*[a-zA-Z_][a-zA-Z_0-9]*\\s*,)*\\s*[a-zA-Z_][a-zA-Z_0-9]*\\s*)\\s*$";
	if (checkIfGivenDirective(assemblyLine, externD, Asembler::processExternDirective)) return true;
	std::regex checkExtern("^\\s*\\.extern.*\\s*$"); if (std::regex_match(assemblyLine, checkExtern))  throw LineNotRecognized(Asembler::currentLine, Asembler::lineCounter);

	std::string byte = "^\\s*\\.byte\\s+((?:\\s*(?:[a-zA-Z_][a-zA-Z_0-9]*|(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]{1,4})\\s*,)*\\s*(?:[a-zA-Z_][a-zA-Z_0-9]*|(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]{1,4})\\s*)\\s*$";
	if (checkIfGivenDirective(assemblyLine, byte, Asembler::processByteDirective)) return true;
	std::regex checkByte("^\\s*\\.byte.*\\s*$"); if (std::regex_match(assemblyLine, checkByte))  throw LineNotRecognized(Asembler::currentLine, Asembler::lineCounter);

	std::string word = "^\\s*\\.word\\s+((?:\\s*(?:[a-zA-Z_][a-zA-Z_0-9]*|(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]{1,4})\\s*,)*\\s*(?:[a-zA-Z_][a-zA-Z_0-9]*|(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]{1,4})\\s*)\\s*$";
	if (checkIfGivenDirective(assemblyLine, word, Asembler::processWordDirective)) return true;
	std::regex checkWord("^\\s*\\.word.*\\s*$"); if (std::regex_match(assemblyLine, checkWord))  throw LineNotRecognized(Asembler::currentLine, Asembler::lineCounter);

	if (checkIfSkip(assemblyLine)) return true;
	if (checkIfSection(assemblyLine)) return true;
	if (checkIfEqu(assemblyLine)) return true;
	if (checkIfEnd(assemblyLine)) return true;



	return false;
}

void Asembler::processGlobalDirective(std::string symbol) {
	auto element = symbolTable.find(symbol);
	auto section = sectionTable.find(symbol);
	if (section != sectionTable.end()) throw SectionCantBeGlobal(Asembler::currentLine, Asembler::lineCounter);

	if (element != symbolTable.end()) {
		if (element->second.sectionNumber == UNDEFINED) throw SymbolMakredToImportCantBeExported(Asembler::currentLine, Asembler::lineCounter);
		element->second.isGlobal = true;
	}
	else {
		SymbolEntry symbolTableEntry;
		symbolTableEntry.isGlobal = true; symbolTableEntry.name = symbol; symbolTableEntry.number = SymbolEntry::nextSymbolNumber++;
		symbolTableEntry.sectionNumber = UNKNOWN;
		symbolTable[symbol] = symbolTableEntry;
	}
}
void Asembler::processExternDirective(std::string symbol) {
	auto element = symbolTable.find(symbol);
	auto section = sectionTable.find(symbol);
	if (section != sectionTable.end()) throw SectionCantBeImported(Asembler::currentLine, Asembler::lineCounter);

	if (element != symbolTable.end()) {
		if (element->second.sectionNumber == UNKNOWN && element->second.isGlobal == true) throw SymbolMarkedToExportCantBeImported(Asembler::currentLine, Asembler::lineCounter);
		if (element->second.isDefined == true) throw DefinedSymbolCantBeImported(Asembler::currentLine, Asembler::lineCounter);
		element->second.isGlobal = true; element->second.sectionNumber = UNDEFINED;
	}
	else {
		SymbolEntry symbolTableEntry;
		symbolTableEntry.isGlobal = true; symbolTableEntry.name = symbol; symbolTableEntry.number = SymbolEntry::nextSymbolNumber++;
		symbolTableEntry.sectionNumber = UNDEFINED;
		symbolTable[symbol] = symbolTableEntry;
	}
}

void Asembler::newSymbolUse(std::string symbol, TypeOfUse typeOfUse, long valueToInsert) {
	auto element = symbolTable.find(symbol);
	if (element == symbolTable.end()) {
		SymbolEntry newSymbol;
		newSymbol.sectionNumber = UNKNOWN; newSymbol.name = symbol; newSymbol.number = SymbolEntry::nextSymbolNumber++;
		symbolTable[symbol] = newSymbol;
		element = symbolTable.find(symbol);
	}
	SymbolUseEntry newUse;
	newUse.sectionName = currentSectionName; newUse.sectionOffset = locationCounter; newUse.typeOfUse = typeOfUse;
	element->second.symbolUseTable.push_back(newUse);
	if (typeOfUse == TypeOfUse::PC_REL) {
		newWordLiteralUse(valueToInsert);
	}
	else {
		auto sectionEntry = sectionTable.find(currentSectionName);
		sectionEntry->second.code.push_back(0);	sectionEntry->second.size++; locationCounter++;
		if (typeOfUse == TypeOfUse::SYMBOL) { sectionEntry->second.code.push_back(0); sectionEntry->second.size++; locationCounter++; }
	}


}
void Asembler::newByteLiteralUse(long literal) {
	unsigned char putToCode = 0, bitsToSet = 0xFF & literal;
	setBitsInChar(putToCode, bitsToSet, 0, 8);
	auto sectionEntry = sectionTable.find(currentSectionName);
	sectionEntry->second.code.push_back(putToCode);	sectionEntry->second.size++;
	locationCounter++;
}
void Asembler::newWordLiteralUse(long literal) {
	auto sectionEntry = sectionTable.find(currentSectionName);
	literal &= 0xFFFF;
	unsigned char putToCode = 0, bitsToSet = (((unsigned long)literal) << ((sizeof(unsigned long) - 2 + 1) * 8)) >> ((sizeof(unsigned long) - 2 + 1) * 8);
	setBitsInChar(putToCode, bitsToSet, 0, 8);
	sectionEntry->second.code.push_back(putToCode); sectionEntry->second.size++;
	putToCode = 0; bitsToSet = (((unsigned long)literal) >> 8);
	setBitsInChar(putToCode, bitsToSet, 0, 8);
	sectionEntry->second.code.push_back(putToCode); sectionEntry->second.size++;
	locationCounter += 2;
}
void Asembler::setSymbolInSection(long literal, std::string sectionToSet, long startPosition, bool halSymbol)
{
	auto sectionEntry = sectionTable.find(sectionToSet);
	char lowByte = sectionEntry->second.code.at(startPosition);
	if (halSymbol == false) {
		char highByte = sectionEntry->second.code.at(startPosition + 1);
		long number = (long)(((unsigned long)highByte) << 8);
		number |= lowByte;
		literal += number;
		if (literal > HIGH_WORD || literal < LOW_WORD) throw WordOutOfRange();
		unsigned char putToCode = 0, bitsToSet = (((unsigned long)literal) << ((sizeof(unsigned long) - 2 + 1) * 8)) >> ((sizeof(unsigned long) - 2 + 1) * 8);
		setBitsInChar(putToCode, bitsToSet, 0, 8);
		sectionEntry->second.code.at(startPosition) = putToCode;
		putToCode = 0; bitsToSet = 0xFF & (((unsigned long)literal) >> 8);
		setBitsInChar(putToCode, bitsToSet, 0, 8);
		sectionEntry->second.code.at(startPosition + 1) = putToCode;
	}
	else {
		literal += lowByte;
		if (literal > HIGH_BYTE || literal < LOW_BYTE) throw ByteOutOfRange();
		sectionEntry->second.code.at(startPosition) = 0xFF & literal;
	}
}


void Asembler::processByteDirective(std::string symbol) {
	if (currentSectionNumber == UNKNOWN) throw SectionNotStarted(Asembler::currentLine, Asembler::lineCounter);
	std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
	std::regex isHexa("^(?:0x|0X)[0-9a-fA-F]+$");

	if (std::regex_match(symbol, isDigit)) {
		long literal = std::stoi(symbol, nullptr, 0);
		if (literal > HIGH_BYTE or literal < LOW_BYTE) throw ByteOutOfRange(Asembler::currentLine, Asembler::lineCounter);
		newByteLiteralUse(literal);
	}
	else {
		newSymbolUse(symbol, TypeOfUse::SYMBOL_ONE_BYTE);
	}

}
void Asembler::processWordDirective(std::string symbol) {
	if (currentSectionNumber == UNKNOWN) throw SectionNotStarted(Asembler::currentLine, Asembler::lineCounter);
	std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
	if (std::regex_match(symbol, isDigit)) {
		long literal = std::stoi(symbol, nullptr, 0);
		if (literal > HIGH_WORD or literal < LOW_WORD) throw WordOutOfRange(Asembler::currentLine, Asembler::lineCounter);
		newWordLiteralUse(literal);
	}
	else {
		newSymbolUse(symbol, TypeOfUse::SYMBOL);
	}
}
void Asembler::processLabelDirective(std::string label) {
	if (sectionTable.find(label) != sectionTable.end()) throw CantHaveSymbolNameSameAsSectionName(Asembler::currentLine, Asembler::lineCounter);
	if (currentSectionNumber == UNKNOWN) throw SectionNotStarted(Asembler::currentLine, Asembler::lineCounter);
	if (locationCounter > HIGH_WORD || locationCounter < LOW_WORD) throw WordOutOfRange();
	auto element = symbolTable.find(label);
	if (element == symbolTable.end()) {
		SymbolEntry newSymbol;
		newSymbol.name = label; newSymbol.sectionNumber = currentSectionNumber; newSymbol.value = locationCounter; newSymbol.isDefined = true;
		newSymbol.number = SymbolEntry::nextSymbolNumber++;
		symbolTable[label] = newSymbol;
	}
	else {
		if (element->second.isDefined) throw AlreadyDefinedSymbolWithThatName(Asembler::currentLine, Asembler::lineCounter);
		if (element->second.sectionNumber == UNDEFINED) throw CantDefinedImportedSymbol(Asembler::currentLine, Asembler::lineCounter);
		element->second.isDefined = true; element->second.value = locationCounter; element->second.sectionNumber = currentSectionNumber;
	}
}
void Asembler::processSectionDirective(std::string section) {
	if (symbolTable.find(section) != symbolTable.end() && sectionTable.find(section) == sectionTable.end()) throw CantHaveSectionNameSameAsSymbolName(Asembler::currentLine, Asembler::lineCounter);
	auto element = sectionTable.find(section);
	if (element == sectionTable.end()) {
		SectionEntry sectionEntry;
		sectionEntry.name = section; sectionEntry.number = SymbolEntry::nextSymbolNumber++;
		sectionTable[section] = sectionEntry;
		element = sectionTable.find(section);
		SymbolEntry newSymbol;
		newSymbol.name = section; newSymbol.isDefined = true; newSymbol.sectionNumber = sectionEntry.number;
		newSymbol.number = sectionEntry.number;
		symbolTable[section] = newSymbol;
	}
	auto symbol = symbolTable.find(section);
	currentSectionName = section;
	currentSectionNumber = element->second.number;
	locationCounter = element->second.size;
}
void Asembler::processEndDirective() {
	isEnd = true;
}
void Asembler::processEquDirective(std::string simbol, std::vector<EquEntryPart> equParts) {
	auto element = symbolTable.find(simbol);
	if (element != symbolTable.end()) {
		if (element->second.isDefined == true) throw AlreadyDefinedSymbolWithThatName(Asembler::currentLine, Asembler::lineCounter);
		if (element->second.sectionNumber == UNDEFINED) throw CantDefinedImportedSymbol(Asembler::currentLine, Asembler::lineCounter);
		element->second.isDefined = true; element->second.sectionNumber = currentSectionNumber;
	}
	else {
		SymbolEntry newSymbol;
		newSymbol.name = simbol; newSymbol.number = SymbolEntry::nextSymbolNumber++;
		newSymbol.isDefined = true; newSymbol.sectionNumber = currentSectionNumber;
		symbolTable[simbol] = newSymbol;
	}
	EquEntry equEntry(simbol, equParts); equEntry.lineNumber = lineCounter; equEntry.equLine = currentLine;
	equTable[simbol] = equEntry;
}
void Asembler::processSkipDirective(std::string simbol) {
	if (currentSectionNumber == UNKNOWN) throw SectionNotStarted(Asembler::currentLine, Asembler::lineCounter);
	long numberOfBytes = std::stoi(simbol, nullptr, 0);
	if (numberOfBytes < 0) throw std::exception();
	auto sectionEntry = sectionTable.find(currentSectionName);
	for (int i = 0; i < numberOfBytes; i++) {
		sectionEntry->second.code.push_back(0); ;
	}
	sectionEntry->second.size += numberOfBytes;
	locationCounter += numberOfBytes;
}

bool Asembler::checkIfInstruction(std::string assemblyLine) {
	std::regex noOperands("^\\s*(halt|iret|ret)\\s*$");
	if (checkInstructionOperandNumber(assemblyLine, noOperands, makeInstructionWithoutParameter)) return true;
	std::regex oneOperand("^\\s*(int|push|pop)(b|w)?\\s+(.*)\\s*$");
	if (checkInstructionOperandNumber(assemblyLine, oneOperand, makeInstructionWithOneParameter)) return true;
	std::regex oneOperandJump("^\\s*(jmp|call|jeq|jne|jgt)w?\\s+(.*)\\s*$");
	if (checkInstructionOperandNumber(assemblyLine, oneOperandJump, makeInstructionWithOneParameterJump)) return true;
	std::regex twoOperand("^\\s*(xchg|mov|add|sub|mul|div|cmp|not|and|or|xor|test|shl|shr)(w|b)?\\s+(.*),(.*)\\s*$");
	if (checkInstructionOperandNumber(assemblyLine, twoOperand, makeInstructionWithTwoParameter)) return true;
	return false;
}
bool Asembler::checkInstructionOperandNumber(std::string assemblyLine, std::regex instructionDescription, bool(*makeInstruction) (std::smatch parsedInstruction)) {
	std::smatch instruction;
	if (std::regex_match(assemblyLine, instruction, instructionDescription)) {
		if (!makeInstruction(instruction)) throw InstructionArgumentsNotRight(Asembler::currentLine, Asembler::lineCounter);
		return true;
	}
	return false;
}

bool Asembler::makeInstructionWithoutParameter(std::smatch parsedInstruction) {
	Instruction instructionWithoutParameter;
	instructionWithoutParameter.instructionName = parsedInstruction.str(1);
	setBitsInChar(instructionWithoutParameter.instructionDescriptionCode, instructionsCode[parsedInstruction.str(1)], 0, 5);
	setBitsInChar(instructionWithoutParameter.instructionDescriptionCode, 0, 5, 3);
	processInstructionWithoutParameter(instructionWithoutParameter);
	return true;
}
bool Asembler::makeInstructionWithOneParameter(std::smatch parsedInstruction) {
	Instruction instructionWithOneParameter;
	instructionWithOneParameter.instructionName = parsedInstruction.str(1);
	Operand operand;
	if (!checkAndSetAdressMode(operand, parsedInstruction.str(3), false)) return false;
	instructionWithOneParameter.operand1 = operand; instructionWithOneParameter.operandNumber = 1;
	instructionWithOneParameter.excplicitSize = true; instructionWithOneParameter.operandSize = TWO_BYTES;
	if (parsedInstruction.str(2) == "b" && parsedInstruction.str(1) == "push" || parsedInstruction.str(1) == "pop") throw OperandSizeNotRight(currentLine, lineCounter);
	if (parsedInstruction.str(2) == "b") { instructionWithOneParameter.excplicitSize = true; instructionWithOneParameter.operandSize = ONE_BYTE; }
	if (parsedInstruction.str(2) == "w") { instructionWithOneParameter.excplicitSize = true; instructionWithOneParameter.operandSize = TWO_BYTES; }
	if (parsedInstruction.str(1) == "int") {
		if (instructionWithOneParameter.excplicitSize == true) {
			if (instructionWithOneParameter.operandSize == TWO_BYTES) {
				if (operand.addressMode == AdressMode::REGDIR && operand.operandSize == ONE_BYTE_OPERAND) throw OperandSizeNotRight(currentLine, lineCounter);
				operand.operandSize = TWO_BYTE_OPERAND;
				instructionWithOneParameter.operandSize = TWO_BYTES;
			}
			else {
				if (operand.addressMode == AdressMode::REGDIR && operand.operandSize == TWO_BYTE_OPERAND) throw OperandSizeNotRight(currentLine, lineCounter);
				if (operand.addressMode == AdressMode::IMD) {
					std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
					if (std::regex_match(operand.symbol, isDigit)) {
						long literal = std::stoi(operand.symbol, nullptr, 0);
						if (literal > HIGH_BYTE || literal < LOW_BYTE) throw OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
					}
				}
				operand.operandSize = ONE_BYTE_OPERAND;
				instructionWithOneParameter.operandSize = ONE_BYTE;
			}
		}
		else {
			if (operand.addressMode == AdressMode::REGDIR) {
				instructionWithOneParameter.excplicitSize = true;
				instructionWithOneParameter.operandSize = operand.operandSize - 1;
			}
			else {
				if (operand.addressMode == AdressMode::IMD) {
					std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
					if (std::regex_match(operand.symbol, isDigit)) {
						long number = std::stoi(operand.symbol, nullptr, 0);
						if (number > HIGH_BYTE || number < LOW_BYTE) {
							instructionWithOneParameter.excplicitSize = true;
							instructionWithOneParameter.operandSize = TWO_BYTES;
						}
						else {
							instructionWithOneParameter.excplicitSize = true;
							instructionWithOneParameter.operandSize = ONE_BYTE;
						}
					}
					else {
						instructionWithOneParameter.excplicitSize = true;
						instructionWithOneParameter.operandSize = TWO_BYTES;
					}

				}
				else {
					instructionWithOneParameter.excplicitSize = true;
					instructionWithOneParameter.operandSize = ONE_BYTE;
				}
			}
		}
	}
	setBitsInChar(instructionWithOneParameter.instructionDescriptionCode, instructionsCode[parsedInstruction.str(1)], 0, 5);
	setBitsInChar(instructionWithOneParameter.instructionDescriptionCode, instructionWithOneParameter.operandSize, 5, 1);
	setBitsInChar(instructionWithOneParameter.instructionDescriptionCode, 0, 6, 2);
	processInstructionWithOneParameter(instructionWithOneParameter);
	return true;
}
bool Asembler::makeInstructionWithTwoParameter(std::smatch parsedInstruction) {
	Instruction instructionWuthTwoParameter;
	instructionWuthTwoParameter.instructionName = parsedInstruction.str(1);
	instructionWuthTwoParameter.operandSize = ONE_BYTE;
	if (parsedInstruction.str(2) == "w") { instructionWuthTwoParameter.operandSize = TWO_BYTES; instructionWuthTwoParameter.excplicitSize = true; }
	if (parsedInstruction.str(2) == "b") { instructionWuthTwoParameter.operandSize = ONE_BYTE; instructionWuthTwoParameter.excplicitSize = true; }
	Operand operandLeft;
	if (!checkAndSetAdressMode(operandLeft, parsedInstruction.str(3), false)) return false;
	Operand operandRight;
	if (!checkAndSetAdressMode(operandRight, parsedInstruction.str(4), false)) return false;
	instructionWuthTwoParameter.operand1 = operandLeft; instructionWuthTwoParameter.operand2 = operandRight;
	instructionWuthTwoParameter.operandNumber = 2;
	setBitsInChar(instructionWuthTwoParameter.instructionDescriptionCode, instructionsCode[parsedInstruction.str(1)], 0, 5);
	processInstructionWithTwoParameter(instructionWuthTwoParameter);
	return true;
}
bool Asembler::makeInstructionWithOneParameterJump(std::smatch parsedInstruction) {
	Instruction instructionWithOneParameter;
	instructionWithOneParameter.instructionName = parsedInstruction.str(1);
	Operand operand;
	if (!checkAndSetAdressMode(operand, parsedInstruction.str(2), true)) return false;
	instructionWithOneParameter.operand1 = operand;	instructionWithOneParameter.operandNumber = 1;
	instructionWithOneParameter.excplicitSize = true; instructionWithOneParameter.operandSize = TWO_BYTES;
	setBitsInChar(instructionWithOneParameter.instructionDescriptionCode, instructionsCode[parsedInstruction.str(1)], 0, 5);
	setBitsInChar(instructionWithOneParameter.instructionDescriptionCode, instructionWithOneParameter.operandSize, 5, 1);
	setBitsInChar(instructionWithOneParameter.instructionDescriptionCode, 0, 6, 2);
	processInstructionWithOneParameter(instructionWithOneParameter);
	return true;
}

void Asembler::processInstructionWithoutParameter(Instruction instruction) {
	if (currentSectionNumber == UNKNOWN) throw SectionNotStarted(Asembler::currentLine, Asembler::lineCounter);
	auto section = sectionTable.find(currentSectionName);
	section->second.code.push_back(instruction.instructionDescriptionCode);
	section->second.size++; locationCounter++;
}
void Asembler::processInstructionWithOneParameter(Instruction instruction) {
	if (currentSectionNumber == UNKNOWN) throw SectionNotStarted(Asembler::currentLine, Asembler::lineCounter);
	if (instruction.instructionName == "pop" && instruction.operand1.addressMode == AdressMode::IMD)  throw IMDCantBeDestination(Asembler::currentLine, Asembler::lineCounter);
	if (instruction.instructionName == "pop" && instruction.operand1.addressMode == AdressMode::REGDIR && instruction.operand1.operandSize == ONE_BYTE_OPERAND) throw OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
	if (instruction.instructionName == "push" && instruction.operand1.addressMode == AdressMode::REGDIR && instruction.operand1.operandSize == ONE_BYTE_OPERAND) throw OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
	if (instruction.instructionName == "int" && instruction.operand1.addressMode == AdressMode::REGDIR && instruction.operand1.operandSize == TWO_BYTE_OPERAND) throw OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
	std::regex isJump("^(?:call|j.*)$");
	if (std::regex_match(instruction.instructionName, isJump)) {
		Operand operand = instruction.operand1;
		if (operand.addressMode == AdressMode::REGDIR && operand.operandSize == ONE_BYTE_OPERAND) throw  OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
	}
	auto element = sectionTable.find(currentSectionName);
	element->second.code.push_back(instruction.instructionDescriptionCode); element->second.size++; locationCounter++;
	element->second.code.push_back(instruction.operand1.operandDescriptionCode); element->second.size++; locationCounter++;
	processOperand(instruction.operand1);
}
void Asembler::processInstructionWithTwoParameter(Instruction instruction) {
	if (currentSectionNumber == UNKNOWN) throw SectionNotStarted(Asembler::currentLine, Asembler::lineCounter);
	if (instruction.operand2.addressMode == AdressMode::IMD && instruction.instructionName!="cmp" && instruction.instructionName!="test") throw IMDCantBeDestination(Asembler::currentLine, Asembler::lineCounter);
	if (instruction.instructionName == "xchg" && instruction.operand1.addressMode == AdressMode::IMD) throw IMDCantBeDestination(Asembler::currentLine, Asembler::lineCounter);
	Operand& operandLeft = instruction.operand1, & operandRight = instruction.operand2;
	if (instruction.excplicitSize == true) {
		decideInstructionOperandsSize(instruction);
	}
	else {
		if (operandLeft.addressMode == AdressMode::REGDIR) {
			instruction.excplicitSize = true;
			instruction.operandSize = operandLeft.operandSize - 1;
		}
		else {
			if (operandRight.addressMode == AdressMode::REGDIR) {
				instruction.excplicitSize = true;
				instruction.operandSize = operandRight.operandSize - 1;
			}
			else {
				if(operandLeft.addressMode == AdressMode::IMD || operandRight.addressMode == AdressMode::IMD){
					if (operandLeft.addressMode == AdressMode::IMD) {
						std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
						if (std::regex_match(operandLeft.symbol, isDigit)) {
							long number = std::stoi(operandLeft.symbol, nullptr, 0);
							if (number > HIGH_BYTE || number < LOW_BYTE) {
								instruction.excplicitSize = true;
								instruction.operandSize = TWO_BYTES;
							}
							else {
								instruction.excplicitSize = true;
								instruction.operandSize = ONE_BYTE;
							}
						}
						else {
							instruction.excplicitSize = true;
							instruction.operandSize = TWO_BYTES;
						}

					} 
					if (operandRight.addressMode == AdressMode::IMD && instruction.operandSize == ONE_BYTE) {
						std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
						if (std::regex_match(operandRight.symbol, isDigit)) {
							long number = std::stoi(operandRight.symbol, nullptr, 0);
							if (number > HIGH_BYTE || number < LOW_BYTE) {
								instruction.excplicitSize = true;
								instruction.operandSize = TWO_BYTES;
							}
							else {
								instruction.excplicitSize = true;
								instruction.operandSize = ONE_BYTE;
							}
						}
						else {
							instruction.excplicitSize = true;
							instruction.operandSize = TWO_BYTES;				
						}
					}
				}
				else {
					instruction.excplicitSize = true;
					instruction.operandSize = ONE_BYTE;
				}
			}			
		}
		decideInstructionOperandsSize(instruction);
	}
	setBitsInChar(instruction.instructionDescriptionCode, instruction.operandSize, 5, 1);
	setBitsInChar(instruction.instructionDescriptionCode, 0, 6, 2);
	auto element = sectionTable.find(currentSectionName);
	element->second.code.push_back(instruction.instructionDescriptionCode); element->second.size++; locationCounter++;
	element->second.code.push_back(operandLeft.operandDescriptionCode); element->second.size++; locationCounter++;
	processOperand(operandLeft, calculateInstructionOperandSize(operandRight));
	element->second.code.push_back(operandRight.operandDescriptionCode); element->second.size++; locationCounter++;
	processOperand(operandRight);
}

void Asembler::decideInstructionOperandsSize(Instruction& instruction) {
	Operand& operandLeft = instruction.operand1, & operandRight = instruction.operand2;
	if (instruction.operandSize == ONE_BYTE) {
		if (operandLeft.addressMode == AdressMode::REGDIR && operandLeft.operandSize == TWO_BYTE_OPERAND) throw OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
		if (operandRight.addressMode == AdressMode::REGDIR && operandRight.operandSize == TWO_BYTE_OPERAND) throw OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
		if (operandLeft.addressMode == AdressMode::IMD) {
			std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
			if (std::regex_match(operandLeft.symbol, isDigit)) {
				long literal = std::stoi(operandLeft.symbol, nullptr, 0);
				if (literal > HIGH_BYTE || literal < LOW_BYTE) throw OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
			}
		}
		if (operandRight.addressMode == AdressMode::IMD) {
			std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
			if (std::regex_match(operandRight.symbol, isDigit)) {
				long literal = std::stoi(operandRight.symbol, nullptr, 0);
				if (literal > HIGH_BYTE || literal < LOW_BYTE) throw OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
			}
		}
		operandLeft.operandSize = ONE_BYTE_OPERAND; operandRight.operandSize = ONE_BYTE_OPERAND;
	}
	else {
		if (operandLeft.addressMode == AdressMode::REGDIR && operandLeft.operandSize == ONE_BYTE_OPERAND) throw OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
		if (operandRight.addressMode == AdressMode::REGDIR && operandRight.operandSize == ONE_BYTE_OPERAND) throw OperandSizeNotRight(Asembler::currentLine, Asembler::lineCounter);
		operandLeft.operandSize = TWO_BYTE_OPERAND; operandRight.operandSize = TWO_BYTE_OPERAND;
	}

}
int Asembler::calculateInstructionOperandSize(Operand& operand) {
	int startSize = 1;
	switch (operand.addressMode) {
	case AdressMode::IMD:
		startSize += operand.operandSize;
		break;
	case AdressMode::MEM: case AdressMode::REGINDPOM:
		startSize += 2;
	}
	return startSize;
}

void Asembler::processOperand(Operand& operand, int nextOperandSize) {
	std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
	switch (operand.addressMode) {
	case AdressMode::IMD:
		if (std::regex_match(operand.symbol, isDigit)) {
			long number = std::stoi(operand.symbol, nullptr, 0);
			if (number > HIGH_WORD || number < LOW_WORD) throw WordOutOfRange(Asembler::currentLine, Asembler::lineCounter);
			if (operand.operandSize == ONE_BYTE_OPERAND) {
				newByteLiteralUse(number);
			}
			else {
				newWordLiteralUse(number);
			}
		}
		else {
			if (operand.operandSize == ONE_BYTE_OPERAND) {
				newSymbolUse(operand.symbol, TypeOfUse::SYMBOL_ONE_BYTE);
			}
			else {
				newSymbolUse(operand.symbol, TypeOfUse::SYMBOL);
			}
		}
		break;
	case AdressMode::MEM:

		if (std::regex_match(operand.symbol, isDigit)) {
			long number = std::stoi(operand.symbol, nullptr, 0);
			if (number > HIGH_WORD || number < LOW_WORD) throw WordOutOfRange(Asembler::currentLine, Asembler::lineCounter);
			newWordLiteralUse(number);
		}
		else {
			newSymbolUse(operand.symbol, TypeOfUse::SYMBOL);
		}
		break;
	case AdressMode::REGINDPOM:
		if (std::regex_match(operand.symbol, isDigit)) {
			long number = std::stoi(operand.symbol, nullptr, 0);
			if (number > HIGH_WORD || number < LOW_WORD) throw WordOutOfRange(Asembler::currentLine, Asembler::lineCounter);
			newWordLiteralUse(number);
		}
		else {
			if (operand.usedRegister == PC) {
				int valueToAdd = -nextOperandSize + PC_REL_ONE_OPERAND;
				newSymbolUse(operand.symbol, TypeOfUse::PC_REL, valueToAdd);
			}
			else {
				newSymbolUse(operand.symbol, TypeOfUse::SYMBOL);
			}
		}
	}
}
bool Asembler::checkAndSetAdressMode(Operand& operand, std::string adressModeDescription, bool isJump) {
	std::string addDolar = "\\$";
	std::string addStar = "";
	if (isJump) addDolar = "";
	if (isJump) addStar = "\\*";
	std::smatch parsedOperand;

	std::regex neposredno("^\\s*" + addDolar + "((?:\\+|-)?[0-9]+|(0x|0X)[0-9a-fA-F]{1,4}|[a-zA-Z_][a-zA-Z_0-9]*)\\s*$");
	if (std::regex_match(adressModeDescription, parsedOperand, neposredno)) return setImdAdressMode(operand, parsedOperand);
	std::regex regDir("^\\s*" + addStar + "%(psw|sp|pc|r[0-7])(h|l)?\\s*$");
	if (std::regex_match(adressModeDescription, parsedOperand, regDir)) return setRegDirAdressMode(operand, parsedOperand);
	std::regex regInd("^\\s*" + addStar + "\\(%(psw|sp|pc|r[0-7])\\)\\s*$");
	if (std::regex_match(adressModeDescription, parsedOperand, regInd)) return setRegIndAdressMode(operand, parsedOperand);
	std::regex regIndPom("^\\s*" + addStar + "([a-zA-Z_][a-zA-Z_0-9]*|(?:\\+|-)?[0-9]+|(0x|0X)[0-9a-fA-F]{1,4})\\(%(psw|sp|pc|r[0-7])\\)\\s*$");
	if (std::regex_match(adressModeDescription, parsedOperand, regIndPom)) return setRegIndPomAdressMode(operand, parsedOperand);
	std::regex memDir("^\\s*" + addStar + "([a-zA-Z_][a-zA-Z_0-9]*|(?:\\+|-)?[0-9]+|(0x|0X)[0-9a-fA-F]{1,4})\\s*$");
	if (std::regex_match(adressModeDescription, parsedOperand, memDir)) return setMemDirAdressMode(operand, parsedOperand);
	return false;
}
char Asembler::decideRegisterNumber(std::string registerNumberDescription) {
	if (registerNumberDescription == "psw") return 15;
	if (registerNumberDescription == "pc") return 7;
	if (registerNumberDescription == "sp") return 6;
	std::string regNumber = "";
	regNumber += registerNumberDescription.at(1);
	return std::stoi(regNumber);

}
bool Asembler::setImdAdressMode(Operand& operand, std::smatch parsedOperand) {
	operand.addressMode = AdressMode::IMD;
	setBitsInChar(operand.operandDescriptionCode, (unsigned char)AdressMode::IMD, 0, 3);
	setBitsInChar(operand.operandDescriptionCode, 0, 3, 5);
	operand.symbol = parsedOperand.str(1);
	return true;
}
bool Asembler::setRegDirAdressMode(Operand& operand, std::smatch parsedOperand) {
	operand.addressMode = AdressMode::REGDIR;
	setBitsInChar(operand.operandDescriptionCode, (unsigned char)AdressMode::REGDIR, 0, 3);
	operand.usedRegister = decideRegisterNumber(parsedOperand.str(1));
	setBitsInChar(operand.operandDescriptionCode, operand.usedRegister, 3, 4);
	operand.operandSize = TWO_BYTE_OPERAND;
	if (parsedOperand.str(2) == "l") { operand.usedPart = 0; operand.operandSize = ONE_BYTE_OPERAND; }
	if (parsedOperand.str(2) == "h") { operand.usedPart = 1; operand.operandSize = ONE_BYTE_OPERAND; }
	setBitsInChar(operand.operandDescriptionCode, operand.usedPart, 7, 1);
	return true;
}
bool Asembler::setRegIndAdressMode(Operand& operand, std::smatch parsedOperand) {
	operand.addressMode = AdressMode::REGIND;
	setBitsInChar(operand.operandDescriptionCode, (unsigned char)AdressMode::REGIND, 0, 3);
	operand.usedRegister = decideRegisterNumber(parsedOperand.str(1));
	setBitsInChar(operand.operandDescriptionCode, operand.usedRegister, 3, 4);
	operand.operandSize = ONE_BYTE_OPERAND;
	setBitsInChar(operand.operandDescriptionCode, operand.usedPart, 7, 1);
	return true;
}
bool Asembler::setRegIndPomAdressMode(Operand& operand, std::smatch parsedOperand) {
	operand.addressMode = AdressMode::REGINDPOM;
	setBitsInChar(operand.operandDescriptionCode, (unsigned char)AdressMode::REGINDPOM, 0, 3);
	operand.usedRegister = decideRegisterNumber(parsedOperand.str(3));
	setBitsInChar(operand.operandDescriptionCode, operand.usedRegister, 3, 4);
	operand.operandSize = ONE_BYTE_OPERAND;
	setBitsInChar(operand.operandDescriptionCode, operand.usedPart, 7, 1);
	operand.symbol = parsedOperand.str(1);
	return true;
}
bool Asembler::setMemDirAdressMode(Operand& operand, std::smatch parsedOperand) {
	operand.addressMode = AdressMode::MEM;
	setBitsInChar(operand.operandDescriptionCode, (unsigned char)AdressMode::MEM, 0, 3);
	operand.operandSize = ONE_BYTE_OPERAND;
	setBitsInChar(operand.operandDescriptionCode, 0, 3, 5);
	operand.symbol = parsedOperand.str(1);
	return true;
}

void Asembler::resolveEquSymbols() {
	bool newDefined = true;
	std::map<std::string, std::map<int, int>> symbolSectionUse;
	while (newDefined) {
		newDefined = false;;
		for (auto& oneEquSymbol : equTable) {
			if (oneEquSymbol.second.isDefined) continue;
			std::string simbolToDefine = oneEquSymbol.second.symbol;
			std::vector<EquEntryPart> newEquParts;
			bool everyPartIsDefined = true;
			for (auto& equPart : oneEquSymbol.second.symbolValue) {
				std::string simbolPart = equPart.symbol;
				std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
				if (!std::regex_match(simbolPart, isDigit)) {
					auto anotherSimbol = equTable.find(simbolPart);
					if (anotherSimbol != equTable.end()) {
						if (anotherSimbol->second.isDefined) {
							for (auto& newPart : anotherSimbol->second.symbolValue) {
								EquEntryPart equEntryPart; equEntryPart.sign = equPart.sign * newPart.sign;
								equEntryPart.symbol = newPart.symbol;
								newEquParts.push_back(equEntryPart);
							}
						}
						else {
							everyPartIsDefined = false;
							newEquParts.push_back(equPart);
						}
					}
					else {
						auto symbolInSimbolTable = symbolTable.find(simbolPart);
						if (symbolInSimbolTable == symbolTable.end()) throw SymbolNotDefined(simbolPart);
						if (symbolInSimbolTable->second.isDefined == false && symbolInSimbolTable->second.sectionNumber != UNDEFINED) throw SymbolNotDefined(simbolPart);
						newEquParts.push_back(equPart);
					}
				}
				else newEquParts.push_back(equPart);
			}
			if (everyPartIsDefined == true) {
				newDefined = true;
				oneEquSymbol.second.isDefined = true;
			}
			oneEquSymbol.second.symbolValue = newEquParts;
		}
	}

	for (auto& oneEquSymbol : equTable) {
		if (oneEquSymbol.second.isDefined == false) throw EquSymbolCantBeResolved(oneEquSymbol.second.equLine, oneEquSymbol.second.lineNumber);
	}

	for (auto& oneEquSymbol : equTable) {
		std::string simbolToDefine = oneEquSymbol.second.symbol;
		std::vector<EquEntryPart> newEquParts;
		int value = 0;
		for (auto& equPart : oneEquSymbol.second.symbolValue) {
			std::string simbolPart = equPart.symbol;
			std::regex isDigit("^(?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]+$");
			if (std::regex_match(simbolPart, isDigit)) {
				long number = std::stoi(simbolPart, nullptr, 0);
				if (number > HIGH_WORD || number < LOW_WORD) throw WordOutOfRange();
				value += number * equPart.sign;
				if (value > HIGH_WORD || value < LOW_WORD) throw WordOutOfRange();
				symbolSectionUse[simbolToDefine][UNKNOWN] = 0;
			}
			else {
				auto element = symbolTable.find(simbolPart);
				if (element->second.isDefined) {
					value += equPart.sign * element->second.value;
					symbolSectionUse[simbolToDefine][element->second.sectionNumber] += equPart.sign;
					if (value > HIGH_WORD || value < LOW_WORD) throw WordOutOfRange();
				}
				else {
					newEquParts.push_back(equPart);
					symbolSectionUse[simbolToDefine][element->second.sectionNumber]++;
				}
			}
		}
		oneEquSymbol.second.symbolValue = newEquParts;
		oneEquSymbol.second.value = value;
	}

	long sectionNumber = UNKNOWN;
	bool found = false;

	SectionEntry equSection; equSection.name = equRealloc;
	sectionTable[equRealloc] = equSection;
	auto equRealocTable = sectionTable.find(equRealloc);

	for (auto& oneSymbol : symbolSectionUse) {
		for (auto& oneUse : oneSymbol.second) {
			if (oneUse.second != 0) {
				if (oneUse.second == 1) {
					if (found == true) {
						auto element = equTable.find(oneSymbol.first)->second;
						throw EquDefinitionError(element.equLine, element.lineNumber);
					}
					found = true;
					sectionNumber = oneUse.first;
				}
				else {
					auto element = equTable.find(oneSymbol.first)->second;
					throw EquDefinitionError(element.equLine, element.lineNumber);
				}
			}
		}

		auto symbolToDefine = symbolTable.find(oneSymbol.first);
		auto equSymbol = equTable.find(oneSymbol.first);
		symbolToDefine->second.value = equSymbol->second.value;
		symbolToDefine->second.sectionNumber = sectionNumber;
		if (sectionNumber != UNKNOWN) {
			RealocEntry realocEntry;
			realocEntry.realocType = TypeOfUse::EQU_SYMBOL; realocEntry.location = symbolToDefine->second.number;
			if (sectionNumber == UNDEFINED && symbolToDefine->second.isGlobal) {
				long externSymbolDependency = symbolTable.find(equSymbol->second.symbolValue.at(0).symbol)->second.number;
				realocEntry.symbolTableReference = externSymbolDependency;
				if (equSymbol->second.symbolValue.at(0).sign == Sign::MINUS) realocEntry.negativeSymbol = true;
				equRealocTable->second.realocTable.push_back(realocEntry);
			}
		}
		found = false;
		sectionNumber = UNKNOWN;
	}

}
void Asembler::checkIfHaveUndefinedSymbols() {
	for (auto const& oneSymbol : symbolTable) {
		if (oneSymbol.second.number != 0) {
			if (oneSymbol.second.isDefined == false && oneSymbol.second.isGlobal == false) throw SymbolNotDefined(oneSymbol.second.name);
			if (oneSymbol.second.isDefined == false && oneSymbol.second.sectionNumber == UNKNOWN) throw SymbolNotDefined(oneSymbol.second.name);
		}
	}
}
void Asembler::ResolveSymbolValuesAndMakeRealocationEntries() {
	for (auto& oneSymbol : symbolTable) {
		long symbolValue = oneSymbol.second.value;
		if (oneSymbol.second.sectionNumber == UNKNOWN) {
			for (auto& oneUse : oneSymbol.second.symbolUseTable) {
				if ((symbolValue > HIGH_BYTE || symbolValue < LOW_BYTE) && oneUse.typeOfUse == TypeOfUse::SYMBOL_ONE_BYTE) throw ByteOutOfRange();
				if (oneUse.typeOfUse == TypeOfUse::PC_REL) {
					RealocEntry realocEntry; realocEntry.location = oneUse.sectionOffset;
					realocEntry.realocType = TypeOfUse::SYMBOL; realocEntry.symbolTableReference = sectionTable.find(oneUse.sectionName)->second.number;
					realocEntry.negativeSymbol = true;
					sectionTable.find(oneUse.sectionName)->second.realocTable.push_back(realocEntry);
					setSymbolInSection(symbolValue - oneUse.sectionOffset, oneUse.sectionName, oneUse.sectionOffset, oneUse.typeOfUse == TypeOfUse::SYMBOL_ONE_BYTE ? true : false);
				}
				else {
					setSymbolInSection(symbolValue, oneUse.sectionName, oneUse.sectionOffset, oneUse.typeOfUse == TypeOfUse::SYMBOL_ONE_BYTE ? true : false);
				}
			}
		}
		else {
			if (oneSymbol.second.isGlobal) {
				for (auto& oneUse : oneSymbol.second.symbolUseTable) {
					if (oneUse.typeOfUse == TypeOfUse::PC_REL && sectionTable.find(oneUse.sectionName)->second.number == oneSymbol.second.sectionNumber) {
						setSymbolInSection(symbolValue - oneUse.sectionOffset, oneUse.sectionName, oneUse.sectionOffset, oneUse.typeOfUse == TypeOfUse::SYMBOL_ONE_BYTE ? true : false);
					}
					else {
						//nema potrebe posto ne ubacujemo nista u kod
						//if ((symbolValue > HIGH_BYTE || symbolValue < LOW_BYTE) && oneUse.typeOfUse == TypeOfUse::SYMBOL_ONE_BYTE) throw ByteOutOfRange();
						RealocEntry realocEntry; realocEntry.location = oneUse.sectionOffset;
						realocEntry.realocType = oneUse.typeOfUse; realocEntry.symbolTableReference = oneSymbol.second.number;
						sectionTable.find(oneUse.sectionName)->second.realocTable.push_back(realocEntry);
					}
				}
			}
			else {
				for (auto& oneUse : oneSymbol.second.symbolUseTable) {
					if (oneUse.typeOfUse == TypeOfUse::PC_REL && sectionTable.find(oneUse.sectionName)->second.number == oneSymbol.second.sectionNumber) {
						setSymbolInSection(symbolValue - oneUse.sectionOffset, oneUse.sectionName, oneUse.sectionOffset, oneUse.typeOfUse == TypeOfUse::SYMBOL_ONE_BYTE ? true : false);
					}
					else {
						if ((symbolValue > HIGH_BYTE || symbolValue < LOW_BYTE) && oneUse.typeOfUse == TypeOfUse::SYMBOL_ONE_BYTE) throw ByteOutOfRange();
						setSymbolInSection(symbolValue, oneUse.sectionName, oneUse.sectionOffset, oneUse.typeOfUse == TypeOfUse::SYMBOL_ONE_BYTE ? true : false);
						RealocEntry realocEntry; realocEntry.location = oneUse.sectionOffset;
						realocEntry.realocType = oneUse.typeOfUse;
						if (oneSymbol.second.sectionNumber == UNDEFINED) {
							auto equSymbol = equTable.find(oneSymbol.second.name);
							realocEntry.negativeSymbol = (equSymbol->second.symbolValue.at(0).sign == Sign::MINUS ? true : false);
							auto symbolDependecy = symbolTable.find(equSymbol->second.symbolValue.at(0).symbol);
							realocEntry.symbolTableReference = symbolDependecy->second.number;
						}
						else {
							realocEntry.symbolTableReference = oneSymbol.second.sectionNumber;
						}
						sectionTable.find(oneUse.sectionName)->second.realocTable.push_back(realocEntry);
					}
				}
			}
		}
	}
}

void Asembler::processInputFile() {
	std::fstream inputFile;
	inputFile.open(this->inputFileName);
	if (inputFile.is_open() == false) throw FileError();

	std::string	asemblyLine;
	while (std::getline(inputFile, asemblyLine)) {
		if (isEnd == true) break;
		Asembler::currentLine = asemblyLine + "\n";
		lineCounter++;
		asemblyLine = Asembler::checkifLabelAtBeginig(asemblyLine);
		if (checkForNothingInLine(asemblyLine)) continue;
		if (checkIfInstruction(asemblyLine)) continue;
		if (checkIfLineIsDirective(asemblyLine)) continue;
		throw LineNotRecognized(Asembler::currentLine, Asembler::lineCounter);
	}
	if (isEnd == false) throw EndDirectiveNotFound();

	resolveEquSymbols();
	checkIfHaveUndefinedSymbols();
	ResolveSymbolValuesAndMakeRealocationEntries();
}
void Asembler::writeToOutputFile() {
	std::fstream outFile;
	outFile.open(outputFileName, std::ios_base::out);
	if (outFile.is_open() == false) throw FileError();
	//Asembler::writeSectionTable(outFile);
	writeSymbolTable(outFile);
	writeSectionsRealocations(outFile);
	writeSectionsCode(outFile);
	writeFileForEmulator();
	std::cout<<"Fajl" << Asembler::inputFileName << " je uspesno preveden" << std::endl;

}
void Asembler::writeSectionTable(std::fstream& outFile) {
	outFile << "Sections" << std::endl;
	outFile << "sectionName" << ":" << "startAdress" << ":" << "Size" << ":" << "number" << std::endl;
	for (auto& oneSection : sectionTable) {
		outFile << oneSection.second.name << ":" << oneSection.second.startAdress << ":" << oneSection.second.size << ":";
		outFile << oneSection.second.number << std::endl;
	}
}
void Asembler::writeSymbolTable(std::fstream& outFile) {
	if (symbolTable.size() == 0) return;
	std::size_t maxLength[5] = { 14, 17, 9, 10, 8 };
	for (auto& oneSection : sectionTable) {
		if (oneSection.second.name != equRealloc) {
			auto oneSymbol = symbolTable.find(oneSection.second.name);
			oneSymbol->second.size = oneSection.second.size;
		}
	}
	for (auto& oneSymbol : symbolTable) {
		std::string oneField = oneSymbol.second.name;
		if (oneField.size() > maxLength[0]) maxLength[0] = oneField.size();
		oneField = std::to_string(oneSymbol.second.sectionNumber);
		if (oneField.size() > maxLength[1]) maxLength[1] = oneField.size();
		oneField = std::to_string(oneSymbol.second.value);
		if (oneField.size() > maxLength[2]) maxLength[2] = oneField.size();
		oneField = std::to_string(oneSymbol.second.number);
		if (oneField.size() > maxLength[3]) maxLength[3] = oneField.size();
		oneField = std::to_string(oneSymbol.second.size);
		if (oneField.size() > maxLength[4]) maxLength[4] = oneField.size();
	}
	std::string symboltableOut = "";
	std::string linWithBreaks = "", lineWithBreaksMiddle;
	for (auto& oneEntry : maxLength) {
		oneEntry += 2;
	}
	int lineLength = maxLength[0] + maxLength[1] + maxLength[2] + maxLength[3] + maxLength[4] + 8 + 5;
	for (int i = 0; i < lineLength / 2; i++) {
		linWithBreaks += "_ ";
		lineWithBreaksMiddle += "- ";
	}
	outFile << linWithBreaks << std::endl;
	outFile << std::setw((lineLength - 14) / 2) << " " << " Symbols Table" << std::setw((lineLength - 14) / 2) << " ";
	if ((lineLength - 14) % 2 != 0) outFile << " ";
	outFile << std::endl;
	outFile << linWithBreaks << std::endl;
	outFile << std::setw((maxLength[0] - 14) / 2) << " " << "  SymbolName  " << std::setw((maxLength[0] - 14) / 2) << " ";
	if (((maxLength[0] - 14) % 2 != 0)) outFile << " ";
	outFile << ":";
	outFile << std::setw((maxLength[1] - 17) / 2) << " " << "  SectionNumber  " << std::setw((maxLength[1] - 17) / 2) << " ";
	if (((maxLength[1] - 17) % 2 != 0)) outFile << " ";
	outFile << ":";
	outFile << std::setw((maxLength[2] - 9) / 2) << " " << "  Value  " << std::setw((maxLength[2] - 9) / 2) << " ";
	if (((maxLength[2] - 9) % 2 != 0)) outFile << " ";
	outFile << ":";
	outFile << "IsGlobal:";
	outFile << std::setw((maxLength[3] - 10) / 2) << " " << "  Number  " << std::setw((maxLength[3] - 10) / 2) << " ";
	if (((maxLength[3] - 10) % 2 != 0)) outFile << " ";
	outFile << ":";
	outFile << std::setw((maxLength[4] - 8) / 2) << " " << "  Size  " << std::setw((maxLength[4] - 8) / 2) << " ";
	if (((maxLength[4] - 8) % 2 != 0)) outFile << " ";
	outFile << std::endl;
	for (auto& oneSymbol : symbolTable) {
		outFile << lineWithBreaksMiddle << std::endl;
		std::string forLength;
		outFile << std::setw((maxLength[0] - oneSymbol.second.name.size()) / 2) << std::right << " " << oneSymbol.second.name << std::setw((maxLength[0] - oneSymbol.second.name.size()) / 2) << " ";
		if ((maxLength[0] - oneSymbol.second.name.size()) % 2 != 0)
			outFile << " ";
		outFile << ":";
		forLength = std::to_string(oneSymbol.second.sectionNumber);
		outFile << std::setw((maxLength[1] - forLength.size()) / 2) << std::right << " " << oneSymbol.second.sectionNumber << std::setw((maxLength[1] - forLength.size()) / 2) << " ";
		if ((maxLength[1] - forLength.size()) % 2 != 0)
			outFile << " ";
		outFile << ":";
		forLength = std::to_string(oneSymbol.second.value);
		outFile << std::setw((maxLength[2] - forLength.size()) / 2) << std::right << " " << oneSymbol.second.value << std::setw((maxLength[2] - forLength.size()) / 2) << " ";
		if ((maxLength[2] - forLength.size()) % 2 != 0)
			outFile << " ";
		outFile << ":";
		outFile << std::setw(8) << (oneSymbol.second.isGlobal == 1 ? "  true  " : "  false ") << ":";
		forLength = std::to_string(oneSymbol.second.number);
		outFile << std::setw((maxLength[3] - forLength.size()) / 2) << std::right << " " << oneSymbol.second.number << std::setw((maxLength[3] - forLength.size()) / 2) << " ";
		if ((maxLength[3] - forLength.size()) % 2 != 0)
			outFile << " ";
		outFile << ":";
		forLength = std::to_string(oneSymbol.second.size);
		outFile << std::setw((maxLength[4] - forLength.size()) / 2) << std::right << " " << oneSymbol.second.size << std::setw((maxLength[4] - forLength.size()) / 2) << " ";
		if ((maxLength[4] - forLength.size()) % 2 != 0)
			outFile << " ";
		outFile << std::endl;
	}
	outFile << lineWithBreaksMiddle << std::endl << std::endl;
}
void Asembler::writeSectionsRealocations(std::fstream& outFile) {
	outFile << "Section Realocation" << std::endl;
	for (auto& oneSection : sectionTable) {
		if (oneSection.second.name != "UND" && oneSection.second.realocTable.size() != 0) {
			int maxLength[3] = { 6, 7, 22 };
			for (auto& oneRealocEntry : oneSection.second.realocTable) {
				std::string length = std::to_string(oneRealocEntry.location);
				if (length.size() > maxLength[1]) maxLength[1] = length.size();
				length = std::to_string(oneRealocEntry.symbolTableReference);
				if (length.size() > maxLength[2]) maxLength[2] = length.size();

			}
			for (auto& oneElement : maxLength)
				oneElement += 2;
			int lineLength = maxLength[0] + maxLength[1] + maxLength[2] + 4 + 3;
			std::string lineWithBreaks = "", lineWithBreaksMiddle = "";
			for (int i = 0; i < lineLength / 2; i++) {
				lineWithBreaks += "_ ";
				lineWithBreaksMiddle += "- ";
			}
			outFile << lineWithBreaks << std::endl;
			outFile << "Ime sekcije: " << oneSection.second.name << std::endl;
			outFile << lineWithBreaks << std::endl;
			outFile << std::setw((maxLength[0] - 6) / 2) << " " << " Type " << std::setw((maxLength[0] - 6) / 2) << " ";
			if ((maxLength[0] - 6) % 2 != 0) outFile << " ";
			outFile << ":";
			outFile << std::setw((maxLength[1] - 7) / 2) << " " << " Ofset " << std::setw((maxLength[1] - 7) / 2) << " ";
			if ((maxLength[0] - 7) % 2 != 0) outFile << " ";
			outFile << ":";
			outFile << "Sign";
			outFile << ":";
			outFile << std::setw((maxLength[2] - 22) / 2) << " " << " SymbolTableReference " << std::setw((maxLength[2] - 22) / 2) << " ";
			if ((maxLength[0] - 22) % 2 != 0) outFile << " ";
			outFile << std::endl;
			for (auto& oneRealocEntry : oneSection.second.realocTable) {
				outFile << lineWithBreaksMiddle << std::endl;
				std::string length = std::to_string((int)oneRealocEntry.realocType);
				outFile << std::setw((maxLength[0] - length.size()) / 2) << " " << (int)oneRealocEntry.realocType << std::setw((maxLength[0] - length.size()) / 2) << " ";
				if ((maxLength[0] - length.size()) % 2 != 0) outFile << " ";
				outFile << ":";
				length = std::to_string(oneRealocEntry.location);
				outFile << std::setw((maxLength[1] - length.size()) / 2) << " " << oneRealocEntry.location << std::setw((maxLength[1] - length.size()) / 2) << " ";
				if ((maxLength[1] - length.size()) % 2 != 0) outFile << " ";
				outFile << ":";
				outFile << std::setw(2) << " " << (oneRealocEntry.negativeSymbol == true ? '-' : '+') << " :";
				length = std::to_string(oneRealocEntry.symbolTableReference);
				outFile << std::setw((maxLength[2] - length.size()) / 2) << " " << oneRealocEntry.symbolTableReference << std::setw((maxLength[2] - length.size()) / 2) << " ";
				if ((maxLength[2] - length.size()) % 2 != 0) outFile << " ";
				outFile << std::endl;
			}
			outFile << lineWithBreaksMiddle << std::endl << std::endl << std::endl;
		}
	}

	outFile << std::endl;
	outFile << "           Reallocation legend         " << std::endl;
	outFile << "_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _" << std::endl;
	outFile << "       Type        :      TypeName     " << std::endl;
	outFile << "        0          :      PC_REL       " << std::endl;
	outFile << "        1          :      SYMBOL       " << std::endl;
	outFile << "        2          :  SYMBOL_ONE_BYTE  " << std::endl;
	outFile << "        3          :     EQU_SYMBOL    " << std::endl;


}
void Asembler::writeSectionsCode(std::fstream& outFile) {
	outFile << "Section code" << std::endl;
	for (auto& oneSection : sectionTable) {
		if (oneSection.second.name != equRealloc && oneSection.second.name != "UND" && oneSection.second.code.size() != 0) {
			outFile << "Ime sekcije: " << oneSection.second.name << std::endl;
			int  i = 1;
			for (auto& code : oneSection.second.code) {
				outFile << std::setw(2) << std::setfill('0') << std::hex << (0xFF & (unsigned int)(code)) << " ";
				if (i % 40 == 0) outFile << std::endl;
				i++;
			}
			outFile << std::endl << std::endl;
		}
	}
}
void Asembler::writeFileForEmulator() {
	std::fstream outEmulatorFile;
	outEmulatorFile.open("e" + std::string(outputFileName), std::ios::out);
	outEmulatorFile << "Dusan Stijovic" << std::endl;
	for (auto oneSymbol : symbolTable) { 
		if (oneSymbol.second.isGlobal == true || oneSymbol.second.sectionNumber == oneSymbol.second.number) {
			outEmulatorFile << oneSymbol.second.name << ":" << oneSymbol.second.sectionNumber << ":" << oneSymbol.second.isGlobal << ":";
			outEmulatorFile << oneSymbol.second.number << ":" << oneSymbol.second.value << ":" << oneSymbol.second.size << ":" << std::endl;
		}
	}
	auto equReallocTable = sectionTable.find(equRealloc);
	outEmulatorFile << "equ realoc" << std::endl;
	for (auto realloc : equReallocTable->second.realocTable) {
		outEmulatorFile << (int)realloc.realocType << ":" << realloc.location << ":" << realloc.symbolTableReference << ":" << realloc.negativeSymbol << ":" << std::endl;
	}
	outEmulatorFile << "end equ realloc" << std::endl;
	for (auto section : sectionTable) {
		if (section.second.name != "UND" && section.second.name != equRealloc) {
			outEmulatorFile << "new section" << std::endl;
			outEmulatorFile << section.second.name << std::endl;
			for (auto realloc : section.second.realocTable) {
				outEmulatorFile << (int)realloc.realocType << ":" << realloc.location << ":" << realloc.symbolTableReference << ":" << realloc.negativeSymbol << ":" << std::endl;
			}
			outEmulatorFile << "section code" << std::endl;
			int i = 0;
			for (auto code : section.second.code) {
				outEmulatorFile << std::setw(2) << std::setfill('0') << std::hex << (0xFF & (unsigned int)(code)) << ":";
				i++;
				if (i % 40 == 0) outEmulatorFile << std::endl;
				
			}
			outEmulatorFile << std::dec;
			outEmulatorFile << std::endl;
			outEmulatorFile << "end section" << std::endl;
		}
	}
	outEmulatorFile << "end" << std::endl;
}

void Asembler::makeAsembler(char* inputFile, const char* outputFile) {
	Asembler::asembler = new Asembler(inputFile, outputFile);

}
void Asembler::deleteAsembler() {
	delete asembler;
	asembler = nullptr;
}
Asembler::Asembler(const char* inputFile, const char* outputFile) {
	this->outputFileName = outputFile;
	this->inputFileName = inputFile;
	SectionEntry firstSection; firstSection.name = "UND"; firstSection.number = 0;
	sectionTable["UND"] = firstSection;
	SymbolEntry newSymbol;
	newSymbol.isDefined = true; newSymbol.sectionNumber = UNDEFINED; newSymbol.name = "UND"; newSymbol.size = 0; newSymbol.number = firstSection.number;
	symbolTable["UND"] = newSymbol;
	SymbolEntry::nextSymbolNumber++;
}
