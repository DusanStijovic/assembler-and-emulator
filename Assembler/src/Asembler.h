#ifndef _asembler_h
#define _asembler_h
#include "Exception.h"
#include <vector>
#include <regex>
#include <map>



enum class TypeOfUse { PC_REL, SYMBOL, SYMBOL_ONE_BYTE, EQU_SYMBOL, EQU_SYMBOL_SUBSTRACT, DECIDE_LATER };


struct RealocEntry {
	TypeOfUse realocType;
	long location;
	long symbolTableReference;
	bool negativeSymbol;
	RealocEntry() {
		realocType = TypeOfUse::DECIDE_LATER; negativeSymbol = false; location = 0; symbolTableReference = 0;
	}
};

struct SymbolUseEntry {
	long sectionOffset;
	std::string sectionName;
	TypeOfUse typeOfUse;
	SymbolUseEntry() {
		sectionOffset = 0; sectionName = ""; typeOfUse = TypeOfUse::DECIDE_LATER;
	}
};
struct SymbolEntry {
	static long nextSymbolNumber;
	std::string name;
	long sectionNumber;
	long value;
	long size;
	bool isGlobal;
	bool isDefined;
	std::vector<SymbolUseEntry> symbolUseTable;
	long number;
	SymbolEntry() {
		name = ""; sectionNumber = 0; value = 0; isGlobal = false; isDefined = false; number = 0; size = -1;
	}
};

struct SectionEntry {
	std::string name;
	long startAdress;
	long size;
	std::vector<char> code;
	std::vector<RealocEntry>realocTable;
	long number;
	SectionEntry() {
		name = ""; startAdress = 0; size = 0; number = 0;
	}
};

struct EquEntryPart {
	std::string symbol;
	char sign;
	EquEntryPart(std::string symbol, char sign) {
		this->symbol = symbol;
		this->sign = sign;
	}
	EquEntryPart() {
		this->symbol = "";
		this->sign = '+';

	};
};
struct EquEntry {
	std::string symbol;
	std::vector<EquEntryPart> symbolValue;
	int value;
	int realocSectionNumber;
	bool isDefined;
	EquEntry(std::string symbol, std::vector<EquEntryPart> symbolValue) {
		this->symbol = symbol;
		this->symbolValue = symbolValue; equLine = "0"; lineNumber = 0; isDefined = false; value = 0;
	}
	EquEntry() { symbol = ""; isDefined = false; }
	std::string equLine;
	long lineNumber;
};

enum class AdressMode { IMD, REGDIR, REGIND, REGINDPOM, MEM };
struct Operand {
	AdressMode addressMode;
	std::string symbol;
	char usedRegister;
	char operandSize;
	char usedPart;
	unsigned char operandDescriptionCode;
	Operand() {
		this->addressMode = AdressMode::IMD;
		usedRegister = 0; operandSize = 0; usedPart = 0; usedRegister = 0;
		operandDescriptionCode = 0;
	}
};
struct Instruction {
	std::string instructionName;
	char operandNumber;
	char operandSize;
	bool excplicitSize;
	Operand operand1, operand2;
	unsigned char instructionDescriptionCode;
	Instruction() {
		instructionName = ""; operandNumber = 0; operandSize = 0; excplicitSize = false; instructionDescriptionCode = 0;
	}
};

class Asembler {
private:
	static std::map<std::string, unsigned char> instructionsCode;
	enum AsemblerOptions { USE_DEFAULT_FILE_NAME = 2, FILE_NAME_GIVEN = 4 };
	enum DefaultSections { UNKNOWN = -1, UNDEFINED = 0 };
	enum ByteRange { LOW_BYTE = 0, HIGH_BYTE = 255, LOW_WORD = -32768, HIGH_WORD =  65535 };
	enum SpecialRegister { PC = 7 };
	enum PC_REL_OFFSET_VALUE { PC_REL_ONE_OPERAND = -2 };
	enum InstructionSize { ONE_BYTE = 0, TWO_BYTES = 1 };
	enum OperandSize { ONE_BYTE_OPERAND = 1, TWO_BYTE_OPERAND = 2 };
	enum Sign :char { PLUS = 1, MINUS = -1 };

	static std::string equRealloc;
	const static int expectedNumberOfArguments = 3;

	const char* outputFileName;
	const char* inputFileName;
	static Asembler* asembler;
	static long locationCounter;
	static bool isEnd;

	static long lineCounter;
	static std::string currentLine;
	static long currentSectionNumber;
	static std::string currentSectionName;
	static std::map<std::string, SymbolEntry> symbolTable;
	static std::map<std::string, SectionEntry> sectionTable;
	static std::map<std::string, EquEntry> equTable;

	static void makeAsembler(char* inputFile, const char* output = "duca.out");
	Asembler(const char* inputFile, const char* outputFile);

	bool checkIfLineIsDirective(std::string assemblyLine);
	bool checkIfGivenDirective(std::string assemblyLine, std::string directive, void(*obradiSimbol)(std::string param));
	std::string checkifLabelAtBeginig(std::string assemblyLine);
	bool checkForNothingInLine(std::string assemblyLine);
	bool checkIfSection(std::string assemblyLine);
	bool checkIfEqu(std::string assemblyLine);
	bool checkIfEnd(std::string assemblyLine);
	bool checkIfSkip(std::string assemblyLine);

	static void newSymbolUse(std::string symbol, TypeOfUse typeOfUse, long symbolValue = 0);
	static void newByteLiteralUse(long simbol);
	static void newWordLiteralUse(long simbol);
	static void setSymbolInSection(long number, std::string sectionToSet, long startPosition, bool halfSymbol);

	static void processGlobalDirective(std::string symbol);
	static void processExternDirective(std::string symbol);
	static void processByteDirective(std::string symbol);
	static void processWordDirective(std::string symbol);
	static void processLabelDirective(std::string label);
	static void processSectionDirective(std::string section);
	static void processEndDirective();
	static void processEquDirective(std::string simbol, std::vector<EquEntryPart> equParts);
	static void processSkipDirective(std::string simbol);


	bool checkIfInstruction(std::string assemblyLine);
	bool checkInstructionOperandNumber(std::string assemblyLine, std::regex instructionDescription, bool(*makeInstruction)(std::smatch parsedInstruction));

	static bool makeInstructionWithoutParameter(std::smatch parsedInstruction);
	static bool makeInstructionWithOneParameter(std::smatch parsedInstruction);
	static bool makeInstructionWithTwoParameter(std::smatch parsedInstruction);
	static bool makeInstructionWithOneParameterJump(std::smatch parsedInstuction);

	static void processInstructionWithoutParameter(Instruction instruction);
	static void processInstructionWithOneParameter(Instruction instruction);
	static void processInstructionWithTwoParameter(Instruction instruction);

	static void decideInstructionOperandsSize(Instruction& instruction);
	static int calculateInstructionOperandSize(Operand& operand);

	static void processOperand(Operand& operand, int nextOperandSize = 0);
	static bool checkAndSetAdressMode(Operand& operand, std::string adressModeDescription, bool isJump);
	static char decideRegisterNumber(std::string registerNumberDescription);
	static bool setImdAdressMode(Operand& instruction, std::smatch parsedOperand);
	static bool setRegDirAdressMode(Operand& instruction, std::smatch parsedOperand);
	static bool setRegIndAdressMode(Operand& instruction, std::smatch parsedOperand);
	static bool setRegIndPomAdressMode(Operand& instruction, std::smatch parsedOperand);
	static bool setMemDirAdressMode(Operand& instruction, std::smatch parsedOperand);

	static void resolveEquSymbols();
	static void checkIfHaveUndefinedSymbols();
	static void ResolveSymbolValuesAndMakeRealocationEntries();


	static void setBitsInChar(unsigned char& destination, unsigned char bitsToSet, unsigned char positionFromStart, unsigned char numberOfBitsToSet) {
		if (positionFromStart + numberOfBitsToSet > 8) return;
		destination |= bitsToSet << (8 - positionFromStart - numberOfBitsToSet);
	}

	void writeSectionTable(std::fstream& outFile);
	void writeSymbolTable(std::fstream& outFile);
	void writeSectionsRealocations(std::fstream& outFile);
	void writeSectionsCode(std::fstream& outfile);
	void writeFileForEmulator();

public:
	static bool checkProgramCall(int argc, char* argv[]);
	static Asembler* getAsembler();
	void processInputFile();
	void writeToOutputFile();
	static void deleteAsembler();

};

#endif