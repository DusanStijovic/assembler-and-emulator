#include "Emulator.h"
#include <fstream>
#include <string>
#include <regex>
#include <iostream>
#include <unistd.h>
#include <termios.h>

struct termios Emulator::oldAttributes;

std::vector<std::string> Emulator::inputFileNames;
std::map<std::string, unsigned int> Emulator::sectionStartAdress;

std::map<std::string, SectionInfo> Emulator::sectionInfoTable;
std::map<std::string, FileEntry> Emulator::fileTable;

Emulator *Emulator::emulator = nullptr;
Machine Emulator::machine;

std::map<InstructionName, InstructionFunction> Emulator::instructionFunction = {
	{HALT, instructionHALT}, {IRET, instructionIRET}, {RET, instructionRET}, {INT, instructionINT}, {CALL, instructionCALL}, {JMP, instructionJMP}, {JEQ, instructionJEQ}, {JNE, instructionJNE}, {JGT, instructionJGT}, {PUSH, instructionPUSH}, {POP, instructionPOP}, {XCHG, instructionXCHG}, {MOV, instructionMOV}, {ADD, instructionADD}, {SUB, instructionSUB}, {MUL, instructionMUL}, {DIV, instructionDIV}, {CMP, instructionCMP}, {NOT, instructionNOT}, {AND, instructionAND}, {OR, instructionOR}, {XOR, instructionXOR}, {TEST, instructionTEST}, {SHL, instructionSHL}, {SHR, instructionSHR}};

std::map<InstructionName, int> Emulator::instructionNumberOfOperands = {
	{HALT, 0}, {IRET, 0}, {INT, 1}, {CALL, 1}, {JMP, 1}, {JEQ, 1}, {JNE, 1}, {JGT, 1}, {PUSH, 1}, {POP, 1}, {XCHG, 2}, {MOV, 2}, {ADD, 2}, {SUB, 2}, {MUL, 2}, {DIV, 2}, {CMP, 2}, {NOT, 2}, {AND, 2}, {OR, 2}, {OR, 2}, {XOR, 2}, {TEST, 2}, {SHL, 2}, {SHR, 2}};

//Provera argumenata komandne linije
bool Emulator::checkIfArgumentsRight(int argc, char *argv[])
{
	std::regex isSectionStartPlace("^-place=([a-zA-Z][a-zA-Z0-9]*)@((?:\\+|-)?[0-9]+|(?:0x|0X)[0-9a-fA-F]{4})$");
	std::regex isInputFie("^.*\\.o$");
	for (int i = 1; i < argc; i++)
	{
		std::string section = argv[i];
		std::smatch sectionInfo;
		if (std::regex_match(section, sectionInfo, isSectionStartPlace))
		{
			if (sectionStartAdress.find(sectionInfo.str(1)) != sectionStartAdress.end())
				throw AlreadySetStartAdressForThatSection(sectionInfo.str(1));
			int startAdress = std::stoi(sectionInfo.str(2), nullptr, 0);
			if (startAdress < 0)
				throw AddressCantBeNegative();
			if (startAdress > HIGH_WORD)
				throw WordOutOfRange("Pocetna pozicija sekcije");
			sectionStartAdress[sectionInfo.str(1)] = 0xFFFF & startAdress;
			continue;
		}

		if (std::regex_match(argv[i], isInputFie))
		{
			inputFileNames.push_back(std::string(argv[i]));
			continue;
		}
		throw InputArgumentNotRight(argv[i]);
	}
	if (inputFileNames.size() == 0)
		throw NoInputFiles();
	Emulator::makeEmulator();
	return false;
}
Emulator *Emulator::makeEmulator()
{
	emulator = new Emulator();
	return emulator;
}
Emulator *Emulator::getEmulator()
{
	if (emulator != nullptr)
		return emulator;
	throw EmulatorDoesntExist();
}

//Citanje fajlova sa asemblerskim kodom
void Emulator::processInputFiles()
{
	for (auto &oneFileName : inputFileNames)
	{
		FileEntry newFile;
		fileTable[oneFileName] = newFile;
		std::fstream readFrom;
		readFrom.open(oneFileName, std::ios::in);
		if (!readFrom.is_open())
			throw FileDoesntOpen();
		std::string oneLine;
		std::getline(readFrom, oneLine);
		if (oneLine != "Dusan Stijovic")
		{
			throw NotRegularInpuFile();
		}
		std::getline(readFrom, oneLine);
		while (oneLine != "equ realoc")
		{
			Emulator::processSymbolTableLine(oneFileName, oneLine);
			std::getline(readFrom, oneLine);
		}
		std::getline(readFrom, oneLine);
		while (oneLine != "end equ realloc")
		{
			Emulator::processEquReallocTableLine(oneFileName, oneLine);
			std::getline(readFrom, oneLine);
		}
		std::getline(readFrom, oneLine);
		while (oneLine != "end")
		{
			std::getline(readFrom, oneLine);
			std::string sectionName = oneLine;

			std::getline(readFrom, oneLine);
			while (oneLine != "section code")
			{
				processSectionReallocTableLine(oneFileName, sectionName, oneLine);
				std::getline(readFrom, oneLine);
			}
			std::getline(readFrom, oneLine);
			while (oneLine != "end section")
			{
				processSectionCodeLine(oneFileName, sectionName, oneLine);
				std::getline(readFrom, oneLine);
			}
			std::getline(readFrom, oneLine);
		}
		readFrom.close();
	}
}
void Emulator::processSymbolTableLine(std::string fileName, std::string oneLine)
{
	auto &symbolTable = fileTable.find(fileName)->second.symbolTable;
	auto &symbolTableByNumber = fileTable.find(fileName)->second.symbolTableByNumber;
	auto &sectionTable = fileTable.find(fileName)->second.sectionTable;
	auto &sectionTableByNumber = fileTable.find(fileName)->second.sectionTableByNumber;
	SymbolEntry newSymbol;
	std::regex symbolLine("^\\s*(.*):(.*):(.*):(.*):(.*):(.*):\\s*$");
	std::smatch propreties;
	if (std::regex_match(oneLine, propreties, symbolLine))
	{
		newSymbol.name = propreties.str(1);
		newSymbol.sectionNumber = std::stoi(propreties.str(2));
		newSymbol.isGlobal = std::stoi(propreties.str(3));
		newSymbol.number = std::stoi(propreties.str(4));
		newSymbol.value = std::stoi(propreties.str(5));
		newSymbol.size = std::stoi(propreties.str(6));
		symbolTable[newSymbol.name] = newSymbol;
		symbolTableByNumber[newSymbol.number] = newSymbol;
		if (newSymbol.sectionNumber == newSymbol.number)
		{ //sekcija
			SectionEntry newSection;
			newSection.name = newSymbol.name;
			newSection.number = newSymbol.number;
			newSection.startAdress = -1;
			newSection.size = newSymbol.size;
			sectionTable[newSection.name] = newSection;
			sectionTableByNumber[newSection.number] = newSection;
		}
	}
}
void Emulator::processEquReallocTableLine(std::string fileName, std::string oneLine)
{
	auto &equReaaloc = fileTable.find(fileName)->second.equReallocation;
	auto &symbolTable = fileTable.find(fileName)->second.symbolTable;
	auto &symbolTableByNumber = fileTable.find(fileName)->second.symbolTableByNumber;
	std::regex sectionReallocLine("^\\s*(.*):(.*):(.*):(.*):\\s*$");
	std::smatch propreties;
	RealocEntry newRealloc;
	if (std::regex_match(oneLine, propreties, sectionReallocLine))
	{
		int type = std::stoi(propreties.str(1));
		switch (type)
		{ //Ovde bi uvek trebalo da bude EQU_SYMBOL
		case 0:
			newRealloc.realocType = TypeOfUse::PC_REL;
			break;
		case 1:
			newRealloc.realocType = TypeOfUse::SYMBOL;
			break;
		case 2:
			newRealloc.realocType = TypeOfUse::SYMBOL_ONE_BYTE;
			break;
		case 3:
			newRealloc.realocType = TypeOfUse::EQU_SYMBOL;
			break;
		}
		newRealloc.location = std::stoi(propreties.str(2));
		newRealloc.symbolTableReference = std::stoi(propreties.str(3));
		newRealloc.negativeSymbol = std::stoi(propreties.str(4));
		auto &symbolEntry = symbolTableByNumber.find(newRealloc.location)->second;
		std::string symbolName = symbolEntry.name;
		symbolEntry.isEqueUndefined = true;
		symbolTable.find(symbolName)->second.isEqueUndefined = true;
		equReaaloc.push_back(newRealloc);
	}
}
void Emulator::processSectionReallocTableLine(std::string fileName, std::string sectionName, std::string oneLine)
{
	auto &sectionTable = fileTable.find(fileName)->second.sectionTable;
	auto &sectionReallocTable = sectionTable.find(sectionName)->second.realocTable;
	std::regex sectionReallocLine("^\\s*(.*):(.*):(.*):(.*):\\s*$");
	std::smatch propreties;
	RealocEntry newRealloc;
	if (std::regex_match(oneLine, propreties, sectionReallocLine))
	{
		switch (std::stoi(propreties.str(1)))
		{
		case 0:
			newRealloc.realocType = TypeOfUse::PC_REL;
			break;
		case 1:
			newRealloc.realocType = TypeOfUse::SYMBOL;
			break;
		case 2:
			newRealloc.realocType = TypeOfUse::SYMBOL_ONE_BYTE;
			break;

		case 3:
			newRealloc.realocType = TypeOfUse::EQU_SYMBOL;
			break;
		}
		newRealloc.location = std::stoi(propreties.str(2));
		newRealloc.symbolTableReference = std::stoi(propreties.str(3));
		newRealloc.negativeSymbol = std::stoi(propreties.str(4));
		sectionReallocTable.push_back(newRealloc);
	}
}
void Emulator::processSectionCodeLine(std::string fileName, std::string sectionName, std::string oneLine)
{
	auto &sectioCode = fileTable.find(fileName)->second.sectionTable.find(sectionName)->second.code;
	std::regex codeRegex("^([0-9a-fA-F]{2}):\\s*");
	std::smatch code;
	while (std::regex_search(oneLine, code, codeRegex))
	{
		sectioCode.push_back(std::stoi(code.str(1), NULL, 16));
		oneLine = code.suffix();
	}
}

//Spajanje fajlova sa asemblerskim kodom u izvrsni fajl i ucitavanje u memoriju
void Emulator::linkFiles()
{
	checkIFHaveMultipleDefinedSymbols(); //Mogli smo dok ucitavamo fajlove :(
	decideSectionsSize();				 //Isto smo mogli prilikom ucitavanja :(
	decideSectionsStartAddress();
	checkIfSectionsOverlap();
	assignSectionsPartStartAddress();
	moveSymbolsBySectionOffset();
	resolveUndefinedEQUSymbols();
	processReallocEntries();
}
bool Emulator::checkIFHaveMultipleDefinedSymbols()
{
	static std::set<std::string> allDefinedSymbols;
	for (auto oneFile : fileTable)
	{
		auto symbolTable = oneFile.second.symbolTable;
		for (auto symbol : symbolTable)
		{ //Sekcije smeju da se ponavaljaju u vise fajlova
			if ((symbol.second.sectionNumber != UNDEFINED || symbol.second.isEqueUndefined == true) && symbol.second.number != symbol.second.sectionNumber)
			{
				if (allDefinedSymbols.find(symbol.second.name) != allDefinedSymbols.end())
					throw SymbolAlreadyDefined(symbol.first);
				allDefinedSymbols.insert(symbol.second.name);
			}
		}
	}
	return false;
}
void Emulator::decideSectionsSize()
{
	for (auto &oneFile : fileTable)
	{
		auto &sections = oneFile.second.sectionTable;
		for (auto &oneSection : sections)
		{
			if (sectionInfoTable.find(oneSection.first) == sectionInfoTable.end())
			{
				SectionInfo newInfo;
				newInfo.startAddress = -1;
				sectionInfoTable[oneSection.first] = newInfo;
			}
			auto &sectionInfo = sectionInfoTable.find(oneSection.first)->second;
			sectionInfo.size += oneSection.second.size;
		}
	}
}
void Emulator::decideSectionsStartAddress()
{
	int maxAddress = 0;
	for (auto oneSectionStartAddress : sectionStartAdress)
	{
		if (sectionInfoTable.find(oneSectionStartAddress.first) == sectionInfoTable.end())
			throw SectionUnknown(oneSectionStartAddress.first);
		auto &sectionInfo = sectionInfoTable.find(oneSectionStartAddress.first)->second;
		sectionInfo.startAddress = oneSectionStartAddress.second;
		if (sectionInfo.startAddress > maxAddress)
		{
			maxAddress = sectionInfo.startAddress + sectionInfo.size;
		}
	}
	for (auto &oneSection : sectionInfoTable)
	{
		if (oneSection.second.startAddress == UNKNOWN && oneSection.first != "UND")
		{
			oneSection.second.startAddress = maxAddress;
			maxAddress += oneSection.second.size;
			if (maxAddress > HIGH_WORD)
				throw WordOutOfRange("Sekcija ne moze da stane!");
		}
	}
}
void Emulator::checkIfSectionsOverlap()
{
	for (auto section : sectionInfoTable)
	{
		if (section.first != "UND" && section.second.size != 0)
		{
			int sectionStart = section.second.startAddress;
			int sectionEnd = section.second.startAddress + section.second.size;
			for (auto sectionToCompare : sectionInfoTable)
			{
				if (sectionToCompare.first != "UND" && section.first != sectionToCompare.first && sectionToCompare.second.size != 0)
				{
					int sectionToCompareStart = sectionToCompare.second.startAddress;
					int sectionToCompareEnd = sectionToCompare.second.startAddress + sectionToCompare.second.size;
					if (sectionStart < sectionToCompareEnd && sectionStart >= sectionToCompareStart)
						throw SectionsOverlaps();
					if (sectionEnd <= sectionToCompareEnd && sectionEnd > sectionToCompareStart)
						throw SectionsOverlaps();
				}
			}
		}
	}
}
void Emulator::assignSectionsPartStartAddress()
{
	for (auto &oneFile : fileTable)
	{
		for (auto &oneSection : oneFile.second.sectionTable)
		{
			auto &sectionInfo = sectionInfoTable.find(oneSection.first)->second;
			oneSection.second.startAdress = sectionInfo.startAddress + sectionInfo.offset;
			sectionInfo.offset += oneSection.second.size;
		}
	}
}
void Emulator::moveSymbolsBySectionOffset()
{
	updateTablesByNumber();
	for (auto &oneFile : fileTable)
	{
		for (auto &oneSymbol : oneFile.second.symbolTable)
		{
			if (oneSymbol.second.sectionNumber != UNKNOWN && oneSymbol.second.sectionNumber != UNDEFINED)
			{
				auto section = oneFile.second.sectionTableByNumber.find(oneSymbol.second.sectionNumber)->second;
				oneSymbol.second.value += section.startAdress;
			}
		}
	}
	updateTablesByNumber();
}
void Emulator::resolveUndefinedEQUSymbols()
{
	std::map<std::string, SymbolValue> symbolsValue;
	for (auto &oneFile : fileTable)
	{
		for (auto &oneEquRealloc : oneFile.second.equReallocation)
		{
			SymbolValue symbolValue;
			auto symbolRef = oneFile.second.symbolTableByNumber.find(oneEquRealloc.symbolTableReference);
			auto symbolToDefined = oneFile.second.symbolTableByNumber.find(oneEquRealloc.location);
			symbolValue.symbol = symbolRef->second.name;
			symbolValue.sectionNumber = symbolRef->second.sectionNumber; //Treba da bude undf proveri
			if (oneEquRealloc.negativeSymbol)
				symbolValue.sign = -1;
			else
				symbolValue.sign = 1;
			symbolsValue[symbolToDefined->second.name] = symbolValue;
		}
		for (auto &oneSymbol : oneFile.second.symbolTable)
		{
			if (!oneSymbol.second.isEqueUndefined)
			{
				if (oneSymbol.second.sectionNumber != UNDEFINED)
				{
					SymbolValue symbolValue;
					symbolValue.symbol = std::to_string(oneSymbol.second.value);
					symbolValue.sign = 1;
					symbolValue.isDefined = true;
					symbolValue.sectionNumber = oneSymbol.second.sectionNumber;
					symbolsValue[oneSymbol.second.name] = symbolValue;
				}
			}
		}
	}
	//Proci sada i razresiti sve;
	bool nextChance = true;
	while (nextChance)
	{
		nextChance = false;
		for (auto &oneSymbol : symbolsValue)
		{
			SymbolValue &symbolValue = oneSymbol.second;
			if (!symbolValue.isDefined)
			{
				std::string dependecy = symbolValue.symbol;
				if (symbolsValue.find(dependecy) == symbolsValue.end())
				{
					throw UnknownSymbol();
				}
				auto &symbolDependency = symbolsValue.find(dependecy)->second;
				if (symbolDependency.isDefined)
				{
					nextChance = true;
					symbolValue.isDefined = true;
					int newSign = symbolValue.sign * symbolDependency.sign;
					int newValue = std::stoi(symbolDependency.symbol) * newSign;
					symbolValue.symbol = std::to_string(newValue);
					symbolValue.sign = 1; //proveri ovo i u asembleru kako si radio
					symbolValue.sectionNumber = symbolDependency.sectionNumber;
				}
			}
		}
	}
	for (auto &oneSymbol : symbolsValue)
	{
		SymbolValue &symbolValue = oneSymbol.second;
		if (!symbolValue.isDefined)
		{
			throw CantResolveEquSymbol();
		}
	}
	for (auto &oneFile : fileTable)
	{
		for (auto &oneSymbol : oneFile.second.symbolTable)
		{
			if (oneSymbol.second.sectionNumber == UNDEFINED && oneSymbol.second.number != 0)
			{
				if (symbolsValue.find(oneSymbol.first) == symbolsValue.end())
					throw SymbolNotDefined(oneSymbol.first);
				int symbolValue = std::stoi(symbolsValue.find(oneSymbol.first)->second.symbol);
				if(symbolValue < 0 && symbolsValue.find(oneSymbol.first)->second.sectionNumber != UNKNOWN) throw AddressCantBeNegative();
				oneSymbol.second.value += symbolValue;
				oneSymbol.second.sectionNumber = symbolsValue.find(oneSymbol.first)->second.sectionNumber;
			}
		}
	}
	for (auto &oneFile : fileTable)
	{
		for (auto &oneSymbol : oneFile.second.symbolTable)
		{
			if (oneSymbol.second.sectionNumber == UNDEFINED && oneSymbol.second.number != 0)
			{
				throw SymbolNotDefined();
			}
		}
	}
	updateTablesByNumber();
}
void Emulator::processReallocEntries()
{
	for (auto &oneFile : fileTable)
	{
		for (auto &oneSection : oneFile.second.sectionTable)
		{
			for (auto &oneRealloc : oneSection.second.realocTable)
			{
				switch (oneRealloc.realocType)
				{
				case TypeOfUse::SYMBOL:
				{
					if(oneRealloc.negativeSymbol == true && 
								oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.sectionNumber != UNKNOWN &&
								oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.sectionNumber != oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.number
								) throw SomeError("Equ simbol razresen ali je negativan i nije apsolutan");
					int location = oneRealloc.location;
					int value = oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.value;
					unsigned char low = oneSection.second.code.at(location);
					char high = oneSection.second.code.at(location + 1);
					long number = ((long)high) << 8;
					number |= (low & 0xFF);					
					if(oneRealloc.negativeSymbol == true) value *= -1;
					number += value;
					if (number > WORD_RANGE::HIGH_WORD || number < WORD_RANGE::LOW_WORD)
						throw WordOutOfRange("Vrednost simbola preko 16 bita");
					char newHigh = 0xFF & (number >> 8);
					char newLow = 0xFF & number;
					oneSection.second.code.at(location) = newLow;
					oneSection.second.code.at(location + 1) = newHigh;
				}
				break;

				case TypeOfUse::SYMBOL_ONE_BYTE:
				{
					if(oneRealloc.negativeSymbol == true && 
								oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.sectionNumber != UNKNOWN &&
								oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.sectionNumber != oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.number
								) throw SomeError("Equ simbol razresen ali je negativan i nije apsolutan");
					int location = oneRealloc.location;
					int value = oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.value;
					int number = oneSection.second.code.at(location);
					number = number & 0xFF; //Uvek pozitivan
					if(oneRealloc.negativeSymbol == true) value *= -1;
					number += value;
            		// Ako ispadne da je negativan greska, ako je jedan byte mora da bude pozitivan broj
					if (number > BYTE_RANGE::HIGH_BYTE || number < BYTE_RANGE::LOW_BYTE)
						throw WordOutOfRange("Vrednost simbola preko 8 bita");
					oneSection.second.code.at(location) = number & 0xFF;
				}
				break;
				case TypeOfUse::PC_REL:
				{
					if(oneRealloc.negativeSymbol == true && 
								oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.sectionNumber != UNKNOWN &&
								oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.sectionNumber != oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.number
								) throw SomeError("Equ simbol razresen ali je negativan i nije apsolutan");
					int location = oneRealloc.location;
					int value = oneFile.second.symbolTableByNumber.find(oneRealloc.symbolTableReference)->second.value;
					unsigned char low = oneSection.second.code.at(location);
					char high = oneSection.second.code.at(location + 1);
					long number = ((long)high) << 8;
					number |= low & 0xFF;
		            if(oneRealloc.negativeSymbol == true) value *= -1;
					number += value;
					number -= location + oneSection.second.startAdress;
					if (number > WORD_RANGE::HIGH_WORD || number < WORD_RANGE::LOW_WORD)
						throw WordOutOfRange("Vrednost simbola preko 16 bita");
					char newHigh = 0xFF & (number >> 8);
					char newLow = 0xFF & number;
					oneSection.second.code.at(location) = newLow;
					oneSection.second.code.at(location + 1) = newHigh;
				}
				break;
				default:
					throw UnknownReallocType();
					break;
				}
			}
		}
	}
}
void Emulator::writeFilesToMemmory()
{
	for (auto oneFile : fileTable)
	{
		for (auto oneSection : oneFile.second.sectionTable)
		{
			int startAddress = oneSection.second.startAdress;
			for (int i = 0; i < oneSection.second.code.size(); i++)
			{
				machine.memory[startAddress + i] = oneSection.second.code.at(i);
			}
		}
	}
}

void Emulator::updateTablesByNumber()
{
	for (auto &oneFile : fileTable)
	{
		for (auto &oneSymbol : oneFile.second.symbolTable)
		{
			oneFile.second.symbolTableByNumber[oneSymbol.second.number] = oneSymbol.second;
		}
		for (auto &oneSection : oneFile.second.sectionTable)
		{
			oneFile.second.sectionTableByNumber[oneSection.second.number] = oneSection.second;
		}
	}
}
void Emulator::updateTablesByName()
{
	for (auto &oneFile : fileTable)
	{
		for (auto &oneSymbol : oneFile.second.symbolTableByNumber)
		{
			oneFile.second.symbolTable[oneSymbol.second.name] = oneSymbol.second;
		}
		for (auto &oneSection : oneFile.second.sectionTableByNumber)
		{
			oneFile.second.sectionTable[oneSection.second.name] = oneSection.second;
		}
	}
}

void Emulator::resetProcessor()
{
	unsigned char low = machine.memory[0]; //Stavi unsigned char, bolje nego posle da andujes
	char high = machine.memory[1];
	int address = (int)high;
	address = (address << 8) | (low & 0xFF);
	machine.registers[pc] = address & 0xFFFF;
	machine.registers[psw] = 0;
	machine.registers[sp] = 0xFFFF + 1;
	machine.memory[TIMER_CFG] = 0;
	setTerminalAtrubutes();
}

void Emulator::simulate()
{
	std::cout << "Simulacija je uspesno zapoceta" << std::endl;
	resetProcessor();
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	long long elapsedTime = 0;
	while (true)
	{
		Instruction instruction = fetchInstruction();
		executeInstruction(instruction);
		if (machine.stop == true)
			break;
		if (machine.writeDataOutRegister)
		{
			writeDataOutRegister();
		}
		if (getI())
			continue;
		if (!getTr())
		{
			if (checkIfTimerElapsed(begin, elapsedTime))
				callInterruptRoutine(TIMER);
		}
		if (!getTl())
		{
			if (checkIfUserTypedSomething())
				callInterruptRoutine(TERMINAL);
		}
	}
	std::cout << std::endl
			  << "Kraj simulacije!" << std::endl;
	returnToOldTerminanlAtributes();
}

void Emulator::extractInstructionCode(Instruction &instruction)
{
	unsigned char instructionDescription = machine.instructionPart[0];
	unsigned char opCodeMask = 0xF8;
	unsigned char opCode = (opCodeMask & instructionDescription) >> 3;
	if (opCode < MINOPCODE || opCode > MAXOPCODE)
	{
		callInterruptRoutine(ERROR);
	}
	instruction.instructionName = (InstructionName)opCode;
	return;
}
void Emulator::extractInstructionSize(Instruction &instruction)
{
	unsigned char instructionDescription = machine.instructionPart[0];
	unsigned char sizeMask = 0x4;
	char size = (instructionDescription & sizeMask) >> 2;
	instruction.operandSize = size + 1;
	return;
}

Instruction Emulator::fetchInstruction()
{
	char opCode = machine.memory[getAddress(machine.registers[pc])];
	machine.instructionPart[0] = opCode;
	Instruction instruction;
	extractInstructionCode(instruction);
	extractInstructionSize(instruction);
	incPC();
	fetchOperands(instruction);
	return instruction;
}
void Emulator::incPC()
{
	int currentPC = machine.registers[Registers::pc] & 0xFFFF;
	machine.registers[Registers::pc] = (currentPC + 1) & 0xFFFF;
	if (machine.registers[Registers::pc] == 0)
		throw InstructionAddressOutOfRange();
}

void Emulator::fetchOperands(Instruction &instruction)
{
	if (instructionNumberOfOperands[instruction.instructionName] == 0)
		return;
	char operanDescription = machine.memory[getAddress(machine.registers[pc])];
	machine.instructionPart[1] = operanDescription;
	Operand left;
	int offset = 0;
	left.operandSize = instruction.operandSize;
	getOpernadDescription(left, 1, offset);
	getAdditionalOperandInfo(left, 1, offset, instruction.operandSize);

	if (instructionNumberOfOperands[instruction.instructionName] == 1)
	{
		fetchOperand(left);
		instruction.operand1 = left;
		instruction.operandNumber = 1;
		machine.writeDataOutRegister = false;
		return;
	}
	machine.instructionPart[2 + offset] = machine.memory[getAddress(machine.registers[pc])];
	Operand right;
	right.operandSize = instruction.operandSize;
	getOpernadDescription(right, 2, offset);
	getAdditionalOperandInfo(right, 2, offset, instruction.operandSize);
	fetchOperand(left);
	machine.writeDataOutRegister = false;
	fetchOperand(right);
	instruction.operand1 = left;
	instruction.operand2 = right;
	instruction.operandNumber = 2;
	return;
}
void Emulator::getOpernadDescription(Operand &operand, int operandNumber, int offset)
{
	unsigned char adressModeMask = 0xE0;
	unsigned char adressMode = ((unsigned char)(adressModeMask & machine.instructionPart[operandNumber + offset])) >> 5;
	operand.addressMode = (AdressMode)adressMode;
	if (adressMode > 4)
		callInterruptRoutine(ERROR);
	unsigned char usedRegisterMask = 0x1E;
	char usedRegister = ((unsigned char)(usedRegisterMask & machine.instructionPart[operandNumber + offset])) >> 1;
	operand.usedRegister = usedRegister;	
	unsigned char partUsedMask = 0x1;
	char partUsed = ((unsigned char)(partUsedMask & machine.instructionPart[operandNumber + offset]));
	operand.usedPart = partUsed;
	incPC();
}
void Emulator::getAdditionalOperandInfo(Operand &operand, int operandNumber, int &offset, int size)
{
	switch (operand.addressMode)
	{
	case AdressMode::IMD:
		machine.instructionPart[operandNumber + offset + 1] = machine.memory[getAddress(machine.registers[pc])];
		incPC();
		if (size == TWO_BYTE)
		{
			machine.instructionPart[operandNumber + offset + 2] = machine.memory[getAddress(machine.registers[pc])];
			incPC();
		}
		break;
	case AdressMode::REGINDPOM:
	case AdressMode::MEM:
		machine.instructionPart[operandNumber + offset + 1] = machine.memory[getAddress(machine.registers[pc])];
		incPC();
		machine.instructionPart[operandNumber + offset + 2] = machine.memory[getAddress(machine.registers[pc])];
		incPC();
		break;
	default:
		return;
	}

	if (size == ONE_BYTE && operand.addressMode == AdressMode::IMD)
	{
		operand.operandAditionalInfo = machine.instructionPart[operandNumber + offset + 1] & 0xFF;
		offset = 1;
	}
	else
	{
		unsigned char low = machine.instructionPart[operandNumber + offset + 1];
		int high = machine.instructionPart[operandNumber + offset + 2];
		operand.operandAditionalInfo = ((((long)high) << 8) | (low & 0xFF));
		offset = 2;
	}
}

void Emulator::fetchOperand(Operand &operand)
{
	switch (operand.addressMode)
	{
	case AdressMode::IMD:
	{
		operand.operandValue = operand.operandAditionalInfo;
		break;
	}
	case AdressMode::REGDIR:
	{
		unsigned int registar = machine.registers[operand.usedRegister];
		if (operand.operandSize == ONE_BYTE)
		{
			if (operand.usedPart == 0)
			{
				operand.operandValue = registar & 0xFF;
			}
			else
			{
				operand.operandValue = (registar & 0xFF00) >> 8;
			}
		} else 
			operand.operandValue = registar;
		return;
		break;
	}
	case AdressMode::MEM:
	{
		int memoryAddres = operand.operandAditionalInfo & 0xFFFF;
		operand.operandValue = readOperandFromMemmory(memoryAddres, operand.operandSize);
		if (memoryAddres == MEMORY_MAPED_REGISTERS::DATA_OUT)
			machine.writeDataOutRegister = true;
		if(memoryAddres == MEMORY_MAPED_REGISTERS::TIMER_CFG)
			machine.timeForInterrupt = true;
		break;	
	}
	case AdressMode::REGIND:
	{
		int memoryAddres = getAddress(machine.registers[operand.usedRegister]);
		operand.operandValue = readOperandFromMemmory(memoryAddres, operand.operandSize);
		if (memoryAddres == MEMORY_MAPED_REGISTERS::DATA_OUT)
			machine.writeDataOutRegister = true;
		if(memoryAddres == MEMORY_MAPED_REGISTERS::TIMER_CFG)
			machine.timeForInterrupt = true;	
		break;
	}
	case AdressMode::REGINDPOM:
	{
		int memoryAddres = getAddress(machine.registers[operand.usedRegister]);
		unsigned int address = getAddress(getAddress(memoryAddres) + operand.operandAditionalInfo);
		if (address > MAX_ADDRESS || address < MIN_ADDRESS)
			throw WordOutOfRange("REGINDPOM adresiranje, adresa van granice sa pomerajem");
		operand.operandValue = readOperandFromMemmory(address, operand.operandSize);
		if (address == MEMORY_MAPED_REGISTERS::DATA_OUT)
			machine.writeDataOutRegister = true;
		if(memoryAddres == MEMORY_MAPED_REGISTERS::TIMER_CFG)
			machine.timeForInterrupt = true;
		break;
	}
	default:
		break;
	}
}
int Emulator::readOperandFromMemmory(int fromWhere, int size)
{
	unsigned char low = machine.memory[getAddress(fromWhere)];
	int high = 0;
	if (size == TWO_BYTE)
	{
		int address = getAddress(fromWhere);
		if (address + 1 > MAX_ADDRESS)
			throw WordOutOfRange("Memorijsko adresiranje, adresa van granice za dva bajta");
		high = machine.memory[address + 1];
		high = high << 8;
	}
	high |= (((unsigned int)low) & 0xFF);
	return high;
}

void Emulator::writeOperand(Operand &operand)
{
	switch (operand.addressMode)
	{
	case AdressMode::REGDIR:
	{
		int inRegister = operand.operandValue;
		if (operand.operandSize == ONE_BYTE)
		{
			inRegister = 0;
			if (operand.usedPart == 0)
			{
				inRegister = operand.operandValue & 0xFF;
				machine.registers[operand.usedRegister] &= (-1 ^ 0xFF);
			}
			else
			{
				inRegister = (operand.operandValue) << 8;
				machine.registers[operand.usedRegister] &= 0x00FF;
			}
		}
		else
		{
			machine.registers[operand.usedRegister] = 0;
		}
		machine.registers[operand.usedRegister] |= inRegister;
		updateSign(machine.registers[operand.usedRegister]);
		break;  
	}

	case AdressMode::MEM:
	{
		int memoryAddres = operand.operandAditionalInfo;
		writeOperandToMemmory(memoryAddres, operand);
		break;
	}
	case AdressMode::REGIND:
	{
		int memoryAddres = getAddress(machine.registers[operand.usedRegister]);
		writeOperandToMemmory(memoryAddres, operand);
		break;
	}
	case AdressMode::REGINDPOM:
	{
		int memoryAddres = machine.registers[operand.usedRegister];
		int address = getAddress(memoryAddres) + operand.operandAditionalInfo;
		if (address > MAX_ADDRESS || address < MIN_ADDRESS)
			throw WordOutOfRange("Adresa sa pomerajem van opsega");
		writeOperandToMemmory(address, operand);
		break;
	}
	default:
		break;
	}
}
int Emulator::writeOperandToMemmory(int where, Operand &operand)
{
	unsigned char low = operand.operandValue & 0xFF;
	machine.memory[getAddress(where)] = low;
	char high = 0;
	if (operand.operandSize == TWO_BYTE)
	{
		int address = getAddress(where) + 1;
		if (address > MAX_ADDRESS)
			throw WordOutOfRange("Adresa van opsega za upis dva operanada!");
		high = (((unsigned char)operand.operandValue) & 0xFF00) >> 8;
		machine.memory[getAddress(address)] = high;
	}
	return 0;
}

void Emulator::executeInstruction(Instruction &instruction)
{
	auto iterator = instructionFunction.find(instruction.instructionName);
	if (iterator == instructionFunction.end())
		throw UnknownInstruction(instruction.instructionName + "");
	iterator->second(instruction);
	if (instructionNumberOfOperands.find(instruction.instructionName)->second == 2)
	{
		if (instruction.instructionName != InstructionName::CMP && instruction.instructionName != TEST)
			writeOperand(instruction.operand2);
	}
}
void Emulator::instructionHALT(Instruction &instruction)
{
	machine.stop = true;
}
void Emulator::instructionIRET(Instruction &instruction)
{
	int whatPoped = pop();
	machine.registers[psw] = whatPoped & 0xFFFF;
	instructionRET(instruction);
}
void Emulator::instructionRET(Instruction &instruction)
{
	int whatPoped = pop();
	machine.registers[pc] = whatPoped & 0xFFFF;
}
void Emulator::instructionINT(Instruction &instruction)
{
	push(machine.registers[pc]);
	push(machine.registers[psw]);
	machine.registers[psw] |= 0x8000;
	int address = ((instruction.operand1.operandValue & 0xFFFF) % 8) * 2;
	machine.registers[pc] = readFromMemmoryTwoByte(address);
}
void Emulator::instructionCALL(Instruction &instruction)
{
	push(machine.registers[pc]);
	machine.registers[pc] = instruction.operand1.operandValue & 0xFFFF;
}
void Emulator::instructionJMP(Instruction &instruction)
{
	machine.registers[pc] = instruction.operand1.operandValue & 0xFFFF;
}
void Emulator::instructionJEQ(Instruction &instruction)
{
	if (getZ())
	{
		machine.registers[pc] = instruction.operand1.operandValue & 0xFFFF;
	}
}
void Emulator::instructionJNE(Instruction &instruction)
{
	if (!getZ())
	{
		machine.registers[pc] = instruction.operand1.operandValue & 0xFFFF;
	}
}
void Emulator::instructionJGT(Instruction &instruction)
{
	if (!getN() && !getZ())
	{
		machine.registers[pc] = instruction.operand1.operandValue & 0xFFFF;
	}
}
void Emulator::instructionPUSH(Instruction &instruction)
{
	push(instruction.operand1.operandValue);
}
void Emulator::instructionPOP(Instruction &instruction)
{
	int ret = pop();
	instruction.operand1.operandValue = ret && 0xFFFF;
}
void Emulator::instructionXCHG(Instruction &instruction)
{
	int temp = instruction.operand1.operandValue;
	instruction.operand1.operandValue = instruction.operand2.operandValue;
	instruction.operand2.operandValue = temp;
}
void Emulator::instructionMOV(Instruction &instruction)
{
	instruction.operand2.operandValue = instruction.operand1.operandValue;
	updateZNFlags(instruction.operand2.operandValue, instruction.operandSize);
}
void Emulator::instructionADD(Instruction &instructon)
{
	int old2 = instructon.operand2.operandValue;
	instructon.operand2.operandValue += instructon.operand1.operandValue;
	updateSign(instructon.operand2.operandValue);
	updateZNFlags(instructon.operand2.operandValue, instructon.operandSize);
	updateOCFlagsAdd(instructon.operand1.operandValue, old2, instructon.operand2.operandValue, instructon.operandSize);
}
void Emulator::instructionSUB(Instruction &instructon)
{
	int old2 = instructon.operand2.operandValue;
	updateSign(instructon.operand2.operandValue);
	instructon.operand2.operandValue -= instructon.operand1.operandValue;
	updateZNFlags(instructon.operand2.operandValue, instructon.operandSize);
	int operand1 =  instructon.operand1.operandValue;
	if(instructon.operandSize == TWO_BYTE)  operand1 *=-1;
	updateOCFlagsSub(operand1, old2, instructon.operand2.operandValue, instructon.operandSize);
}
void Emulator::instructionMUL(Instruction &instructon)
{
	int old2 = instructon.operand2.operandValue;
	instructon.operand2.operandValue *= instructon.operand1.operandValue;
	updateSign(instructon.operand2.operandValue);
	updateZNFlags(instructon.operand2.operandValue, instructon.operandSize);
}
void Emulator::instructionDIV(Instruction &instructon)
{
	int old2 = instructon.operand2.operandValue;
	if (instructon.operand1.operandValue == 0)
	{
		callInterruptRoutine(ERROR);
	}
	instructon.operand2.operandValue /= instructon.operand1.operandValue;
	updateSign(instructon.operand2.operandValue);
	updateZNFlags(instructon.operand2.operandValue, instructon.operandSize);
}
void Emulator::instructionCMP(Instruction &instructon)
{
	int result = instructon.operand2.operandValue - instructon.operand1.operandValue;
	updateSign(result);
	updateZNFlags(result, instructon.operandSize);
	updateOCFlagsSub(instructon.operand1.operandValue, instructon.operand2.operandValue, result, instructon.operandSize);
}
void Emulator::instructionNOT(Instruction &instructon)
{
	instructon.operand2.operandValue = !instructon.operand1.operandValue;
	updateSign(instructon.operand2.operandValue);
	updateZNFlags(instructon.operand2.operandValue, instructon.operandSize);
}
void Emulator::instructionAND(Instruction &instruction)
{
	instruction.operand2.operandValue &= instruction.operand1.operandValue;
	updateSign(instruction.operand2.operandValue);
	updateZNFlags(instruction.operand2.operandValue, instruction.operandSize);
}
void Emulator::instructionOR(Instruction &instruction)
{
	instruction.operand2.operandValue |= instruction.operand1.operandValue;
	updateSign(instruction.operand2.operandValue);
	updateZNFlags(instruction.operand2.operandValue, instruction.operandSize);
}
void Emulator::instructionXOR(Instruction &instructon)
{
	instructon.operand2.operandValue ^= instructon.operand1.operandValue;
	updateSign(instructon.operand2.operandValue);
	updateZNFlags(instructon.operand2.operandValue, instructon.operandSize);
}
void Emulator::instructionTEST(Instruction &instructon)
{
	int result = instructon.operand1.operandValue & instructon.operand2.operandValue;
	updateSign(result);
	updateZNFlags(result, instructon.operandSize);
}
void Emulator::instructionSHL(Instruction &instructon)
{
	if (instructon.operand1.operandValue < 0)
		callInterruptRoutine(ERROR);
	int old2 = instructon.operand2.operandValue;
	instructon.operand2.operandValue <<= instructon.operand1.operandValue;

	updateZNFlags(instructon.operand2.operandValue, instructon.operandSize);
	updateCarryFlagShiftLeft(instructon.operand1.operandValue, old2, instructon.operand2.operandValue, instructon.operandSize);
}
void Emulator::instructionSHR(Instruction &instructon)
{
	if (instructon.operand1.operandValue < 0)
		callInterruptRoutine(ERROR);
	int old2 = instructon.operand2.operandValue;
	instructon.operand2.operandValue >>= instructon.operand1.operandValue;
	updateZNFlags(instructon.operand2.operandValue, instructon.operandSize);
	updateCarryFlagShiftRight(instructon.operand1.operandValue, old2, instructon.operand2.operandValue, instructon.operandSize);
}

void Emulator::push(int whatToPush)
{
	if (getAddress(machine.registers[sp] - 2) < 0)
		throw WordOutOfRange("Stek je pun ne moze se staviti nista na njega");
	machine.registers[sp] -= 2;
	writeToMemmoryTwoByte(getAddress(machine.registers[sp]), whatToPush);
}
int Emulator::pop()
{
	if (getAddress(machine.registers[sp] + 2) > MAX_ADDRESS + 1)
		throw WordOutOfRange("Stek je prazan, ne moze se skinuti nista na njega");
	int whatPoped = readFromMemmoryTwoByte(machine.registers[6]);
	machine.registers[6] += 2;
	return whatPoped;
}
void Emulator::writeToMemmoryOneByte(int where, int whatToWrite)
{
	int memmoryAdress = where;
	unsigned char low = (whatToWrite & 0xFF);
	machine.memory[getAddress(memmoryAdress)] = low;
}
void Emulator::writeToMemmoryTwoByte(int where, int whatToWrite)
{
	int memmoryAdress = where;
	char high = (whatToWrite >> 8) & 0xFF;
	unsigned char low = (whatToWrite & 0xFF);
	machine.memory[getAddress(memmoryAdress)] = low;
	memmoryAdress = (memmoryAdress & 0xFFFF) + 1;
	machine.memory[getAddress(memmoryAdress)] = high;
}
char Emulator::readFromMemmoryOneByte(int fromWhere)
{
	int memmoryAdress = fromWhere;
	unsigned char low = machine.memory[getAddress(memmoryAdress)];
	return low;
}
int Emulator::readFromMemmoryTwoByte(int fromWhere)
{
	if (fromWhere + 1 > MAX_ADDRESS)
		throw WordOutOfRange("Adresa van opsega prilikom citanja dva bajta");
	int memmoryAdress = fromWhere;
	char low = machine.memory[getAddress(memmoryAdress)];
	char high = machine.memory[getAddress(memmoryAdress) + 1];
	int whatPoped = (((int)high) << 8) | (low & 0xFF);
	return whatPoped;
}

void Emulator::updateZeroFlag(int result, int size)
{
	if (result == 0)
		setZ();
	else
		unsetZ();
}
void Emulator::updateNegativeFlag(int result, int size)
{
	if (size == ONE_BYTE)
	{
		if(result & 0x80) 
			setN();
		else 	
			unsetN();
		return;
	}
	if (result & 0x8000)
		setN();
	else
		unsetN();
}
void Emulator::updateOverFlowFlag(int old1, int old2, int result, int size)
{
	if (size == ONE_BYTE){
		unsetO();
		return;
	}
	bool isNegative = (0x8000 & result) != 0;
	if (old1 > 0 && old2 > 0 && isNegative)
	{
		setO();
		return;
	}
	if (old1 < 0 && old2 < 0 && !isNegative)
	{
		setO();
		return;
	}
	unsetO();
}
void Emulator::updateCarryFlagShiftLeft(int old1, int old2, int result, int size){
	    if(old1 == 0){
			unsetC();
			return;
		} 
		if(old1 >= 16 && old2  != 0 ){
			setC();
			return;
		} 
		int help = old2 >> (sizeof(int) - old1);
		if( help != 0 ) setC();
		else unsetC();

}

void Emulator::updateCarryFlagShiftRight(int old1, int old2, int result, int size){
		if(old1 == 0){
			unsetC();
			return;
		}		
		if(old1 >= 16 && old2  != 0 ) {
			setC();
			return;
		}
		int help = old2 << (sizeof(int) - old1);
		if( help != 0 ) setC();
		else unsetC();
}

void Emulator::updateZNFlags(int result, int size)
{
	updateNegativeFlag(result, size);
	updateZeroFlag(result, size);
}

void Emulator::updateOCFlagsAdd(int old1, int old2, int result, int size)
{
	int max = 0xFFFF;
	if (size == ONE_BYTE)
		max = 0xFF;
	if (old1 + old2 > max)
		setC();
	else
		unsetC();
	updateOverFlowFlag(old1, old2, result, size);
}

void Emulator::updateSign(int &result)
{
	unsigned char isNegative = (result & 0x8000) != 0 ? true : false;
	if (isNegative)
	{
		result |= (-1 ^ 0xFFFF);
	}
	else
	{
		result &= 0xFFFF;
	}
}

void Emulator::updateOCFlagsSub(int old1, int old2, int result, int size)
{
	if (old1 < old2)
		setC();
	else
		unsetC();
	updateOverFlowFlag(old1, old2, result, size);
}

bool Emulator::getZ()
{
	return machine.registers[psw] & 0x1;
}
bool Emulator::getO()
{
	return ((unsigned int)(machine.registers[psw] & 0x2)) >> 1;
}
bool Emulator::getC()
{
	return ((unsigned int)(machine.registers[psw] & 0x4)) >> 2;
}
bool Emulator::getN()
{
	return ((unsigned int)(machine.registers[psw] & 0x8)) >> 3;
}
bool Emulator::getTr()
{
	return ((unsigned int)(machine.registers[psw] & 0x2000)) >> 13;
}
bool Emulator::getTl()
{
	return ((unsigned int)(machine.registers[psw] & 0x4000)) >> 14;
}
bool Emulator::getI()
{
	return (machine.registers[psw] & 0x8000) != 0;
}
bool Emulator::setZ()
{
	machine.registers[psw] |= 0x1;
	return false;
}
bool Emulator::setO()
{
	machine.registers[psw] |= 0x2;
	return false;
}
bool Emulator::setC()
{
	machine.registers[psw] |= 0x4;
	return false;
}
bool Emulator::setN()
{
	machine.registers[psw] |= 0x8;
	return false;
}
bool Emulator::setTr()
{
	machine.registers[psw] |= 0x2000;
	return false;
}
bool Emulator::setTl()
{
	machine.registers[psw] |= 0x4000;
	return false;
}
bool Emulator::setI()
{
	machine.registers[psw] |= 0x8000;
	return false;
}
bool Emulator::unsetZ()
{
	machine.registers[psw] &= 0xFFFE;
	return false;
}
bool Emulator::unsetO()
{
	machine.registers[psw] &= 0xFFFD;
	return false;
}
bool Emulator::unsetC()
{
	machine.registers[psw] &= 0xFFFB;
	return false;
}
bool Emulator::unsetN()
{
	machine.registers[psw] &= 0xFFF7;
	return false;
}
bool Emulator::unsetTr()
{
	machine.registers[psw] &= 0xDFFF;
	return false;
}
bool Emulator::unsetTl()
{
	machine.registers[psw] &= 0xBFFF;
	return false;
}
bool Emulator::unsetI()
{
	machine.registers[psw] &= 0x7FFE;
	return false;
}

void Emulator::setTerminalAtrubutes()
{

	tcgetattr(STDIN_FILENO, &Emulator::oldAttributes);

	struct termios attributes;
	tcgetattr(STDIN_FILENO, &attributes);

	attributes.c_lflag = ~ICANON;
	attributes.c_cc[VMIN] = 0;
	attributes.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &attributes);
}
void Emulator::returnToOldTerminanlAtributes()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &Emulator::oldAttributes);
}

void Emulator::writeDataOutRegister()
{
	std::cout << machine.memory[MEMORY_MAPED_REGISTERS::DATA_OUT];
}

void Emulator::writeToDataInRegister(int whatTyped)
{
	machine.memory[MEMORY_MAPED_REGISTERS::DATA_IN] = whatTyped;
}

bool Emulator::checkIfUserTypedSomething()
{
	int typedSomething = 0;
	int whatTyped = 2;
	typedSomething = read(STDIN_FILENO, &whatTyped, 1);
	if (typedSomething)
	{
		writeToDataInRegister(whatTyped);
		return true;
	}
	return false;
}
bool Emulator::checkIfTimerElapsed(std::chrono::steady_clock::time_point &start, long long &elapsedTime)
{
	if (machine.timeForInterrupt == true)
	{
		machine.timeForInterrupt = false;
		start = std::chrono::steady_clock::now();
		elapsedTime = 0;
	}
	int period = Emulator::timeBetweenInterrupt[machine.memory[MEMORY_MAPED_REGISTERS::TIMER_CFG] & 0x7];
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	elapsedTime += std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
	start = now;
	if (elapsedTime / 1000000 > period)
	{
		elapsedTime = 0;
		return true;
	}
	return false;
}

void Emulator::callInterruptRoutine(char number)
{
	Instruction instruction;
	instruction.operand1.operandValue = number;
	instructionINT(instruction);
}
