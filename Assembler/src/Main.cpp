#include <iostream>
#include "Asembler.h"


int main(int argc, char* argv[]) {

	try {
		Asembler::checkProgramCall(argc, argv);
		Asembler::getAsembler()->processInputFile();
		Asembler::getAsembler()->writeToOutputFile();
		Asembler::deleteAsembler();
	}
	catch (std::exception& e) {
		std::cout << e.what();
	}
}