#ifndef exception_h_duca
#define exception_h_duca
#include <exception>
#include <string>

class InputArgumentNotRight : public std::exception {
	
private:
	std::string message;

public:
	InputArgumentNotRight(std::string message, std::string option = "Argument nije u odgovarajucem formatu: ") {
		this->message = option;
		this->message += message;
		this->message += "\n";
	}
	virtual const char* what() const noexcept  {
		return message.c_str();
	}

	
	
};


class AlreadySetStartAdressForThatSection : public InputArgumentNotRight {
public:
	AlreadySetStartAdressForThatSection(std::string message) :InputArgumentNotRight(message, "Za datu sekciju je vec setovana adresa") {}
};

class EmulatorDoesntExist : public std::exception {


	const char* what()const noexcept override {
		return "Emulato ne postoji!\n";
	}
};

class SomeError: public std::exception {
protected:
	std::string message;
public:
	SomeError(std::string aditionalMessage = "Greska, linija nije prepoznata:") {
		this->message = aditionalMessage;
		this->message += "\n";
	}
	SomeError() {}
	virtual const char* what() const noexcept {
		return this->message.c_str();
	};
};




class WordOutOfRange : public std::exception {
private:
	std::string message;
public:
	WordOutOfRange(std::string message) {
		this->message = "Van opsega, Word je opsega od -32768 do 32767.Uzrok: ";
		this->message += message;
		this->message += "\n";
	}

	const char* what()const noexcept override {
		return message.c_str();
	}
};

class SectionsOverlaps : public SomeError {

public:
	SectionsOverlaps(): SomeError("Sekcije se preklapaju.") {}
};


class UnknownSymbol: public SomeError {

public:
	UnknownSymbol() : SomeError("Nepoznat simbol") {}
};


class UnknownReallocType: public SomeError {

public:
	UnknownReallocType() : SomeError("Nepoznat simbol") {}
};



class SymbolNotDefined: public SomeError {

public:
	SymbolNotDefined() : SomeError("Simbol nije definisan") {}
	SymbolNotDefined(std::string symbolName) : SomeError("Simbol nije definisan " + symbolName + "\n") {}
    

};



class CantResolveEquSymbol: public SomeError {
public:
	CantResolveEquSymbol() : SomeError("Ne mogu se razresiti equ simboli") {}
};




class SymbolAlreadyDefined : public SomeError {
public:
	SymbolAlreadyDefined(std::string simbol) : SomeError("Simbol vec definisan " + simbol) {}
};



class AddressCantBeNegative : public SomeError {
public:
	AddressCantBeNegative() : SomeError("Adresa ne moze biti negativna") {}
};




class NoInputFiles : public SomeError {
public:
	NoInputFiles() : SomeError("Nema nijednog ulaznog fajla") {}
};

class FileDoesntOpen : public SomeError {
public:
	FileDoesntOpen() : SomeError("Ulazni fajl nije uspesno otvoren") {}
};


class NotRegularInpuFile : public SomeError {
public:
	NotRegularInpuFile() : SomeError("Ulazni fajl nema dobar pocetak") {}
};

class SectionUnknown : public SomeError {
public:
	SectionUnknown(std::string sekcija) : SomeError("Sekcija ne postoji u ulaznom fajlu " + sekcija) {}
};



class UnknownInstruction : public SomeError {
public: 
	UnknownInstruction(std::string opcode) : SomeError("Nepoznata instrukcija: " + opcode){}

};



class InstructionAddressOutOfRange : public SomeError {
public:
	InstructionAddressOutOfRange() : SomeError("Adresa instrukcije van opsega") {}
};

#endif
