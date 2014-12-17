/* (C) 2011 Viktor Lofgren
 *
 *  This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "getoptpp.h"
#include <stdexcept>
#include <cassert>
#include <cstdio>
#include <string>
#include <sys/ioctl.h>

using namespace std;

namespace vlofgren {

/*
*
* Class OptionsParser
*
*
*/


OptionsParser::OptionsParser(const char* programDesc) : fprogramDesc(programDesc) {}
OptionsParser::~OptionsParser() {}

ParameterSet& OptionsParser::getParameters() {
	return parameters;
}

void OptionsParser::parse(int argc, const char* argv[]) throw(runtime_error)
{
	argv0 = argv[0];

	if(argc == 1) return;

	vector<string> v(&argv[1], &argv[argc]);

	ParserState state(/* *this,*/ v);

	for(; !state.end(); state.advance()) {

		std::list<Parameter*>::iterator i;

		for(i = parameters.parameters.begin();
			i != parameters.parameters.end(); i++)
		{
			int n = 0;
			try
			{
				n = (*i)->receive(state);
			}
			catch(Parameter::ExpectedArgument &)
			{
				throw Parameter::ExpectedArgument(state.get() + ": expected an argument");
			}
			catch(Parameter::UnexpectedArgument &)
			{
				throw Parameter::UnexpectedArgument(state.get() + ": did not expect an argument");
			}
			catch(Switchable::SwitchingError &)
			{
				throw Parameter::ParameterRejected(state.get() + ": parameter already set");
			}
			catch(Parameter::ParameterRejected & pr) {
				std::string what = pr.what();
				if(what.length())
				{
					throw Parameter::ParameterRejected(state.get() + ": " + what);
				}
				throw Parameter::ParameterRejected(state.get() + " (unspecified error)");
			}

			for (int j = 1; j < n; ++j)
			{
				state.advance();
			}

			if(n != 0)
			{
				break;
			}
		}

		if(i == parameters.parameters.end()) {
			std::string file = state.get();
			if(file == "--") {
				state.advance();
				break;
			}
			else if(file.at(0) == '-')
				throw Parameter::ParameterRejected(string("Bad parameter: ") + file);
			else files.push_back(state.get());
		}
	}

	if(!state.end()) for(; !state.end(); state.advance()) {
		files.push_back(state.get());
	}

}

void OptionsParser::usage() const {
	cerr << fprogramDesc << endl;
	cerr << "Build time: " << __DATE__ << " " << __TIME__ << endl << endl;
	cerr << "Usage: " << programName() << " [OPTIONS]" << endl << endl;

	cerr << "Parameters: " << endl;

	int totalWidth = 80;
	int usageWidth = 33;

	// read total width from the terminal
	struct winsize w;
	if (ioctl(0, TIOCGWINSZ, &w) == 0)
	{
		if (w.ws_col > totalWidth)
			totalWidth = w.ws_col;
	}

	std::list<Parameter*>::const_iterator i;
	for(i = parameters.parameters.begin();
		i != parameters.parameters.end(); i++)
	{
		cerr.width(usageWidth);
		cerr << std::left << "    " + (*i)->usageLine();

		std::string description = (*i)->description();
		while (int(description.length()) > (totalWidth - usageWidth))
		{
			size_t pos = description.find_last_of(' ', totalWidth - usageWidth);
			cerr << description.substr(0, pos) << std::endl << std::string(usageWidth - 1, ' ');
			description = description.substr(pos);
		}
		cerr << description << endl;

	}
}

const vector<string>& OptionsParser::getFiles() const {
	return files;
}

const string& OptionsParser::programName() const {
	return argv0;
}

/*
* Parameter set
*
*
*/

ParameterSet::ParameterSet(const ParameterSet& ps) {
	throw new runtime_error("ParameterSet not copyable");
}

ParameterSet::~ParameterSet() {
	for(std::list<Parameter*>::iterator i = parameters.begin();
		i != parameters.end(); i++)
	{
		delete *i;
	}

}

/* The typical use case for command line arguments makes linear searching completely
* acceptable here.
*/

Parameter& ParameterSet::operator[](char c) const {
	for(std::list<Parameter*>::const_iterator i = parameters.begin(); i!= parameters.end(); i++) {
		if((*i)->shortOption() == c) return *(*i);
	}
	throw out_of_range("ParameterSet["+string(&c)+string("]"));
}


Parameter& ParameterSet::operator[](const string& param) const {
	for(std::list<Parameter*>::const_iterator i = parameters.begin(); i!= parameters.end(); i++) {
		if((*i)->longOption() == param) return *(*i);
	}
	throw out_of_range("ParameterSet["+param+"]");
}



/*
*
* Class ParserState
*
*
*/


ParserState::ParserState(/*OptionsParser &opts, */vector<string>& args) :
	/*opts(opts),*/ arguments(args), iterator(args.begin())
{

}

const string ParserState::peek() const {
	vector<string>::const_iterator next = iterator+1;
	if(next != arguments.end()) return *next;
	else return "";

}

const string ParserState::get() const {
	if(!end()) return *iterator;
	else return "";
}

void ParserState::advance() {
	iterator++;
}

bool ParserState::end() const {
	return iterator == arguments.end();
}


/*
*
* Class Parameter
*
*
*/



Parameter::Parameter(char shortOption, const std::string & longOption, const std::string & description) :
	fshortOption(shortOption), flongOption(longOption), fdescription(description)
{

}

Parameter::~Parameter() {}

const string& Parameter::description() const { return fdescription; }
const string& Parameter::longOption() const { return flongOption; }
bool Parameter::hasShortOption() const { return fshortOption != 0x0; }
char Parameter::shortOption() const { assert(hasShortOption()); return fshortOption; }

/*
*
* Class Switchable
*
*
*/

bool Switchable::isSet() const { return fset; }
Switchable::~Switchable() {};
Switchable::Switchable() : fset(false) {}

void MultiSwitchable::set() throw (Switchable::SwitchingError) { fset = true; }
MultiSwitchable::~MultiSwitchable() {}


void UniquelySwitchable::set() throw (Switchable::SwitchingError) {
	if(UniquelySwitchable::isSet()) throw Switchable::SwitchingError();
	fset = true;
}
UniquelySwitchable::~UniquelySwitchable() {}


PresettableUniquelySwitchable::~PresettableUniquelySwitchable() {}
bool PresettableUniquelySwitchable::isSet() const {
	return UniquelySwitchable::isSet() || fpreset.isSet();
}
void PresettableUniquelySwitchable::set() throw (Switchable::SwitchingError)
{
	UniquelySwitchable::set();
}
void PresettableUniquelySwitchable::preset() {
	fpreset.set();
}

/*
*
* PODParameter specializations
*
*
*
*/


template<>
PODParameter<string>::PODParameter(char shortOption, const char *longOption,
								   const char* description) : CommonParameter<PresettableUniquelySwitchable>(shortOption, longOption, description) {
}


template<>
int PODParameter<int>::validate(const string &s) throw(Parameter::ParameterRejected)
{
	// This is sadly necessary for strto*-functions to operate on
	// const char*. The function doesn't write to the memory, though,
	// so it's quite safe.

	char* cstr = const_cast<char*>(s.c_str());
	if(*cstr == '\0') throw ParameterRejected("No argument given");

	long l = strtol(cstr, &cstr, 10);
	if(*cstr != '\0') throw ParameterRejected("Expected int");

	if(l > INT_MAX || l < INT_MIN) {
		throw ParameterRejected("Expected int");
	}

	return l;
}

template<>
long PODParameter<long>::validate(const string &s) throw(Parameter::ParameterRejected)
{
	char* cstr = const_cast<char*>(s.c_str());
	if(*cstr == '\0') throw ParameterRejected("No argument given");

	long l = strtol(cstr, &cstr, 10);
	if(*cstr != '\0') throw ParameterRejected("Expected long");

	return l;
}

template<>
double PODParameter<double>::validate(const string &s) throw(Parameter::ParameterRejected)
{
	char* cstr = const_cast<char*>(s.c_str());
	if(*cstr == '\0') throw ParameterRejected("No argument given");

	double d = strtod(cstr, &cstr);
	if(*cstr != '\0') throw ParameterRejected("Expected double");

	return d;
}

template<>
string PODParameter<string>::validate(const string &s) throw(Parameter::ParameterRejected)
{
	return s;
}


} //namespace
