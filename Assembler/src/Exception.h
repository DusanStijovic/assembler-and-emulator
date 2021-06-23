#ifndef _exception_h
#define _exception_h
#include <exception>
#include <string>

class AsemblerNoTFound : public std::exception {
public:
	virtual const char* what() const noexcept {
		return "Asembler nije napravljen\n";
	};
};


class FileError : public std::exception {
public:
	virtual const char* what() const noexcept {
		return "Greska sa fajlom\n";
	};
};
class ArgumentsFormatNotRigth : public std::exception {
	std::string message;
public:
	ArgumentsFormatNotRigth(std::string message) {
		this->message = "Ulazni parametri nisu odgovarajuci\nFormat: asembler [-o imeIzlaznog.o] imeUlazno.s\nVas ulaz:" + message + "\n";
	}
	virtual const char* what() const noexcept {
		return this->message.c_str();
	};
};
class InstructionArgumentsNotRight : public std::exception {
	std::string message;

public:
	InstructionArgumentsNotRight(std::string line, long number) {
		this->message = "Greska, operandi instrukcije:";
		this->message += std::to_string(number);
		this->message += ". ";
		this->message += line;
	}

	virtual const char* what() const noexcept {
		return this->message.c_str();
	};

};

class LineNotRecognized : public std::exception {
protected:
	std::string message;
public:
	LineNotRecognized(const std::string& line, long number, std::string aditionalMessage = "Greska, linija nije prepoznata:") {
		this->message = aditionalMessage;
		this->message += std::to_string(number);
		this->message += ". ";
		this->message += line;
	}
	LineNotRecognized() { this->message = "Nepoznata greska"; }
	virtual const char* what() const noexcept {
		return this->message.c_str();
	};
};
class SectionCantBeGlobal : public LineNotRecognized {
public:
	SectionCantBeGlobal(std::string line, long number) : LineNotRecognized(line, number, "Sekcija ne moze biti globalna") {
	}
};
class SectionCantBeImported : public LineNotRecognized {
public:
	SectionCantBeImported(std::string line, long number) : LineNotRecognized(line, number, "Sekcija se ne moze izvoziti") {
	}
};
class DefinedSymbolCantBeImported : public LineNotRecognized {
public:
	DefinedSymbolCantBeImported(std::string line, long number) : LineNotRecognized(line, number, "Definisan simbol se ne moze uvoziti") {
	}
};
class SymbolMarkedToExportCantBeImported : public LineNotRecognized {
public:
	SymbolMarkedToExportCantBeImported(std::string line, long number) : LineNotRecognized(line, number, "Simbol koji se izvozi se ne moze uvoziti") {
	}
};
class SymbolMakredToImportCantBeExported : public LineNotRecognized {
public:
	SymbolMakredToImportCantBeExported(std::string line, long number) : LineNotRecognized(line, number, "Simbol koji se uvozi se ne moze izvoziti") {
	}
};
class ByteOutOfRange : public LineNotRecognized {
public:
	ByteOutOfRange(std::string line, long number) : LineNotRecognized(line, number, "Byte je opsega od 0 do 255") {
	}
	ByteOutOfRange() {
		message = "Van opsega, Byte je opsega od 0 do 255";
	}
};
class WordOutOfRange : public LineNotRecognized {
public:
	WordOutOfRange(std::string line, long number) : LineNotRecognized(line, number, "Word je opsega od -32768 do 32767") {
	}
	WordOutOfRange() {
		message = "Van opsega, Word je opsega od -32768 do 32767";
	}
};
class SectionNotStarted : public LineNotRecognized {
public:
	SectionNotStarted(std::string line, long number) : LineNotRecognized(line, number, "Sekcija nije zapoceta") {
	}
};
class CantUseSectionNameAsSymbolValue : public LineNotRecognized {
public:
	CantUseSectionNameAsSymbolValue(std::string line, long number) : LineNotRecognized(line, number, "Ime sekcije se ne moze koristiti kao simbol") {
	}
};
class AlreadyDefinedSymbolWithThatName : public LineNotRecognized {
public:
	AlreadyDefinedSymbolWithThatName(std::string line, long number) : LineNotRecognized(line, number, "Vec postoji simbol definisan sa datim imenom") {
	}
};
class CantHaveSectionNameSameAsSymbolName : public LineNotRecognized {
public:
	CantHaveSectionNameSameAsSymbolName(std::string line, long number) : LineNotRecognized(line, number, "Ime sekcije ne moze biti isto kao ime simbola") {
	}
};

class CantHaveSymbolNameSameAsSectionName : public LineNotRecognized {
public:
	CantHaveSymbolNameSameAsSectionName(std::string line, long number) : LineNotRecognized(line, number, "Ime simbola ne moze biti isto kao ime sekcije") {
	}
};
class IMDCantBeDestination : public LineNotRecognized {
public:
	IMDCantBeDestination(std::string line, long number) : LineNotRecognized(line, number, "Neposredna vrednost ne moze biti odrediste") {
	}
};
class OperandSizeNotRight : public LineNotRecognized {
public:
	OperandSizeNotRight(std::string line, long number) : LineNotRecognized(line, number, "Velicina operanda nije odgovarajuca") {
	}
};
class CantDefinedImportedSymbol : public LineNotRecognized {
public:
	CantDefinedImportedSymbol(std::string line, long number) : LineNotRecognized(line, number, "Ne mozete definisati simbol koji se uvozi!") {
	}
};
class EndDirectiveNotFound : public std::exception {
public:
	virtual const char* what() const noexcept {
		return "U ulaznom fajlu nije pronadjena .end direktiova\n";
	};

};

class SymbolNotDefined : public std::exception {
	std::string message;
public:
	SymbolNotDefined(std::string name) {
		this->message = "Simbol nije definisan ";
		this->message += name;
		this->message += "\n";
	}
	virtual const char* what() const noexcept {
		return this->message.c_str();
	};
};
class CantDefinedSimbolUsingYourself : public LineNotRecognized {
public:
	CantDefinedSimbolUsingYourself(std::string line, long number) : LineNotRecognized(line, number, "Ne mozete definisati simbol koristexi sebe") {
	}
};

class EquDefinitionError : public LineNotRecognized {
public:
	EquDefinitionError(std::string line, long number) : LineNotRecognized(line, number, "Neispravna definicija equ-a!") {
	}
};
class EquSymbolCantBeResolved : public LineNotRecognized {
public:
	EquSymbolCantBeResolved(std::string line, long number) : LineNotRecognized(line, number, "Equ se ne moze razresiti!") {
	}
};
class JumpAdressCanyBeLessThenZero : public LineNotRecognized {
public:
	JumpAdressCanyBeLessThenZero(std::string line, long number) : LineNotRecognized(line, number, "Adresa skoka ne moze biti manja od nule!") {
	}
};
#endif