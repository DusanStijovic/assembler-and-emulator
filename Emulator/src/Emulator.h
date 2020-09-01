#ifndef emulator_h
#define emulator_h
#include <string>
#include <vector>
#include <map>
#include <set>
#include "Exception.h"
#include <chrono>
#include <unistd.h>
#include <termios.h>


enum class AdressMode { IMD, REGDIR, REGIND, REGINDPOM, MEM };
enum Registers {r0,r1,r2,r3,r4,r5,r6,r7,sp=6,pc=7,psw=0xf};

struct Operand {
	AdressMode addressMode;
	std::string symbol;
	char usedRegister;
	char operandSize;
	char usedPart;
	int operandAditionalInfo;
	int operandValue;
	Operand() {
		this->addressMode = AdressMode::IMD;
		usedRegister = 0; operandSize = 0; usedPart = 0; usedRegister = 0;
		operandAditionalInfo = 0; operandValue = 0;
	}
};

enum InstructionName {
	UNKNOWN = -1,HALT, IRET, RET, INT, CALL, JMP, JEQ, JNE, JGT, PUSH,
	POP, XCHG, MOV, ADD, SUB, MUL, DIV, CMP, NOT, AND,
	OR, XOR, TEST, SHL, SHR, MINOPCODE = 0, MAXOPCODE = 24
};



struct Instruction {
	InstructionName instructionName;
	char operandNumber;
	char operandSize;
	Operand operand1, operand2;
	unsigned char instructionDescriptionCode;
	Instruction() {
		instructionName = UNKNOWN; operandNumber = 0; operandSize = 0; instructionDescriptionCode = 0;
	}
};

enum class TypeOfUse { PC_REL, SYMBOL, SYMBOL_ONE_BYTE, EQU_SYMBOL, EQU_SYMBOL_SUBSTRACT, DECIDE_LATER };

struct SymbolValue {
	std::string symbol;
	char sign;
	bool isDefined;
	int sectionNumber;
	SymbolValue(std::string symbol, char sign) {
		this->symbol = symbol;
		this->sign = sign;
		isDefined = false;
	}
	SymbolValue() {
		this->symbol = "";
		this->sign = '+';
		isDefined = false;
	};
};
struct RealocEntry {
	TypeOfUse realocType;
	long location;
	long symbolTableReference;
	bool negativeSymbol;
	RealocEntry() {
		realocType = TypeOfUse::DECIDE_LATER; negativeSymbol = false; location = 0; symbolTableReference = 0;
	}
};


struct SymbolEntry {
	static long nextSymbolNumber;
	std::string name;
	long sectionNumber;
	long value;
	long size;
	bool isGlobal;	
	bool isEqueUndefined;
	long number;
	SymbolEntry() {
		name = ""; sectionNumber = 0; value = 0; isGlobal = false; number = 0; size = -1; isEqueUndefined = false;
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
		name = ""; startAdress = -1; size = -1; number = 0;
	}
};



struct FileEntry {
	std::map<std::string,SectionEntry> sectionTable;
	std::map<int, SectionEntry> sectionTableByNumber;
	std::map<std::string, SymbolEntry> symbolTable;
	std::map<int, SymbolEntry> symbolTableByNumber;
	std::vector<RealocEntry> equReallocation;
};


struct SectionInfo {
	int startAddress;
	int size;
	int offset;
	SectionInfo(){
		startAddress = 0; size = 0; offset = 0;
	}
};


struct Machine {
	char memory[65535] = {0};
	int pc;//Broj instrukcije
	char instructionPart[7] = { 0 };
	int registers[16];
	bool stop = false;
	bool writeDataOutRegister = false;
	bool timeForInterrupt = false;

};


typedef void (*InstructionFunction) (Instruction&);


class Emulator {
private:
	long timeBetweenInterrupt[8] = {500, 1000, 1500, 2000, 5000, 10 * 1000, 30 * 1000, 60 * 1000};
	enum WORD_RANGE {LOW_WORD = -32768 , HIGH_WORD = 65555};
	enum BYTE_RANGE { LOW_BYTE = 0, HIGH_BYTE = 255 };
	enum symbols {UNKNOWN = -1, UNDEFINED};
	enum MEMORY_MAPED_REGISTERS{DATA_OUT = 0xFF00, DATA_IN = 0xFF02, TIMER_CFG = 0xFF10};
	enum INTERRUPT_ENTRY{RESTART = 0, ERROR = 1, TIMER = 2, TERMINAL = 3};
	enum OPERAND_SIZE {ONE_BYTE = 1, TWO_BYTE = 2};
	enum MEMORY_ADDRESS { MAX_ADDRESS = 0xFFFF, MIN_ADDRESS = 0x00 };

	static Emulator* emulator;
	static Machine machine;

	static struct termios oldAttributes;


	static std::map<InstructionName, int> instructionNumberOfOperands;
	static std::map<InstructionName,InstructionFunction>instructionFunction;
	static std::map<std::string, SectionInfo> sectionInfoTable;
	static std::map<std::string, FileEntry> fileTable;
	static Emulator* makeEmulator();
	static std::vector<std::string> inputFileNames;
	static std::map<std::string, unsigned int> sectionStartAdress;

	static void processSymbolTableLine(std::string fileName, std::string oneLine);
	static void processEquReallocTableLine(std::string fileName, std::string oneLine);
	static void processSectionReallocTableLine(std::string fileName, std::string sectionName, std::string oneLine);
	static void processSectionCodeLine(std::string fileName, std::string sectionName, std::string oneLine);

   static bool checkIFHaveMultipleDefinedSymbols();
   static void decideSectionsSize();
   void decideSectionsStartAddress();
   void checkIfSectionsOverlap();
   void assignSectionsPartStartAddress();
   void moveSymbolsBySectionOffset();
   void resolveUndefinedEQUSymbols();
   void processReallocEntries();
   
   void updateTablesByNumber();
   void updateTablesByName();

   void extractInstructionCode(Instruction&);
   void extractInstructionSize(Instruction&);

   void getOpernadDescription(Operand&,int, int);
   void getAdditionalOperandInfo(Operand&,int,int&, int size);
   int readOperandFromMemmory(int fromWhere, int size);
   int writeOperandToMemmory(int where, Operand &operand);

   Instruction fetchInstruction();
   void fetchOperands(Instruction&);
   void fetchOperand(Operand&);
   void writeOperand(Operand&);
   static void incPC();


   static void instructionHALT(Instruction& instruction);
   static void instructionIRET(Instruction& instruction);
   static void instructionRET(Instruction& instruction);
   static void instructionINT(Instruction& instruction);
   static void instructionCALL(Instruction& instruction);
   static void instructionJMP(Instruction& instructon);
   static void instructionJEQ(Instruction& instructon);
   static void instructionJNE(Instruction& instructon);
   static void instructionJGT(Instruction& instructon);
   static void instructionPUSH(Instruction& instructon);
   static void instructionPOP(Instruction& instructon);
   static void instructionXCHG(Instruction& instructon);
   static void instructionMOV(Instruction& instructon);
   static void instructionADD(Instruction& instructon);
   static void instructionSUB(Instruction& instructon);
   static void instructionMUL(Instruction& instructon);
   static void instructionDIV(Instruction& instructon);
   static void instructionCMP(Instruction& instructon);
   static void instructionNOT(Instruction& instructon);
   static void instructionAND(Instruction& instruction);
   static void instructionOR(Instruction& instruction);
   static void instructionXOR(Instruction& instructon);
   static void instructionTEST(Instruction& instructon);
   static void instructionSHL(Instruction& instructon);
   static void instructionSHR(Instruction& instructon);

   static void push(int whatToPush);
   static int pop();

   static void writeToMemmoryOneByte(int where, int whatToWrite);
   static void writeToMemmoryTwoByte(int where, int whatToWrite);
   static char readFromMemmoryOneByte(int fromWhere);
   static int readFromMemmoryTwoByte(int fromWhere);


   void resetProcessor();

   static void updateZeroFlag(int result, int size);
   static void updateNegativeFlag(int result, int size);
   static void updateOverFlowFlag(int old1, int old2, int result, int size);
   static void updateCarryFlagShiftRight(int old1, int old2, int result, int size);
   static void updateCarryFlagShiftLeft(int old1, int old2, int result, int size);
   static void updateZNFlags(int result, int  size);
   static void updateOCFlagsSub(int old1, int old2, int result, int size);
   static void updateOCFlagsAdd(int old1, int old2, int result, int size);

   static void updateSign(int& result);

   static bool getZ();
   static bool getO();
   static bool getC();
   static bool getN();
   static bool getTr();
   static bool getTl();
   static bool getI();


   static bool setZ();
   static bool setO();
   static bool setC();
   static bool setN();
   static bool setTr();
   static bool setTl();
   static bool setI();

   static bool unsetZ();
   static bool unsetO();
   static bool unsetC();
   static bool unsetN();
   static bool unsetTr();
   static bool unsetTl();
   static bool unsetI();

   //Prekidne rutine

   static void callInterruptRoutine(char number);


   //Periferije

   void setTerminalAtrubutes();
   void returnToOldTerminanlAtributes();

   //Terminal



   void writeDataOutRegister();
   void writeToDataInRegister(int whatTyped);
   bool checkIfUserTypedSomething();
   

   //Tajmer


   bool checkIfTimerElapsed(std::chrono::steady_clock::time_point &, long long& elapsedTime);
  

   //DODAJ DA AZURIRAS I TABELE PO BROJU PREMA TABELA PO IMENU
public:

	static bool checkIfArgumentsRight(int argc, char* argv[]);
	static Emulator *getEmulator();

	void processInputFiles();
	void linkFiles();
	void writeFilesToMemmory();
	void simulate();

	void executeInstruction(Instruction&);



	static int getAddress(int address) {
		if (address > HIGH_WORD || address < LOW_WORD) throw WordOutOfRange("Adresa van opsega");
		return address & 0xFFFF;
	}

	~Emulator() {
		delete  emulator;
		emulator = nullptr;
	}

};


#endif // !emulator_h