#pragma once

#include <string>

namespace Process {
	
void restartHyperion(bool asNewProcess=false); 
std::string command_exec(const char* cmd);

};