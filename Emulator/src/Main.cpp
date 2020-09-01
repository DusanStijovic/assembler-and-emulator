#include "Emulator.h"
#include <iostream>


int main(int argc, char* argv[]) {

	try {
		Emulator::checkIfArgumentsRight(argc, argv);
		Emulator::getEmulator()->processInputFiles();
		Emulator::getEmulator()->linkFiles();
		Emulator::getEmulator()->writeFilesToMemmory();
		Emulator::getEmulator()->simulate();
	}
	catch (std::exception& ex) {
		std::cout << ex.what()<<std::endl;
	}
}