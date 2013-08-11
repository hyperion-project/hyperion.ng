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
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <algorithm>

using namespace std;
using namespace vlofgren;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*
 *
 * This is one way of adding new parameter types,
 * inheriting existing types and adding new validation.
 *
 * In this case, StringParameter (which is a typedef of PODParameter<string> gets
 * a new validator that only accepts strings with only letters.
 *
 */


class AlphabeticParameter : public StringParameter {
public:
	AlphabeticParameter(char shortName, const char* longName, const char* description) :
		StringParameter(shortName, longName, description) {}
	virtual ~AlphabeticParameter() {}

	void receiveSwitch() throw(Parameter::ParameterRejected) {
		throw Parameter::ParameterRejected();
	}


	/* isalpha may be a macro */
	static bool isNotAlpha(char c) { return !isalpha(c); }

	virtual string validate(const string& arg) throw(Parameter::ParameterRejected) {
		int nonalpha = count_if(arg.begin(), arg.end(), isNotAlpha);


		if(nonalpha) throw Parameter::ParameterRejected("I only want numbers");
		else return arg;
	}

};

/*
 *
 * The other way is to specialize the PODParameter class
 *
 */

enum RockPaperScissor { ROCK, PAPER, SCISSOR } ;

namespace vlofgren {
	// needs to live in the vlofgren namespace for whatever reason
	template<> enum RockPaperScissor
	PODParameter<enum RockPaperScissor>::validate(const string &s) throw(Parameter::ParameterRejected)
	{
		if(s == "rock")
			return ROCK;
		else if(s == "paper")
			return PAPER;
		else if(s == "scissor")
			return SCISSOR;
		else {
			throw ParameterRejected("Invalid argument");
		}

	}
}
typedef PODParameter<enum RockPaperScissor> RockPaperScissorParameter;


/*
 *
 *  Dummy program
 *
 */


int main(int argc, const char* argv[]) {

	// Create a parser

	OptionsParser optp("An example program (that also runs some tests)");
	ParameterSet& ps = optp.getParameters();

	/* An alternative option is to simply extend the options parser and set all this up
	 * in the constructor.
	 */

	ps.add<SwitchParameter>('f', "foo", "Enable the foo system (no argument)");
	ps.add<StringParameter>('b', "bar", "Enable the bar system (string argument)");
	ps.add<PODParameter<double> >('z', "baz", "Enable the baz system (floating point argument");

	PODParameter<int>& i = ps.add<PODParameter<int> >('i', "foobar", "Enable the foobar system (integer argument");
	i.setDefault(15);

	ps.add<AlphabeticParameter>('a', "alpha", "Custom parameter that requires a string of letters");
	ps.add<RockPaperScissorParameter>('r', "rps", "Takes the values rock, paper or scissor");
	ps.add<SwitchParameter>('h', "help", "Display help screen");


	// Register the parameters with the parser

	try {
		// Parse argv
		optp.parse(argc, argv);

		// Test for the help flag
		if(ps['h'].isSet()) {
			optp.usage();
			return EXIT_SUCCESS;
		}

		// Print out what values the parameters were given

		cout << "The following parameters were set:" << endl;

		cout << "foo: " << (ps['f'].isSet() ? "true" : "false") << endl;
		cout << "bar: \"" << ps['b'].get<string>() << "\""<< endl;
		cout << "baz: ";

		if(ps['z'].isSet()) {
			cout << ps['z'].get<double>() << endl;
		} else {
			cout << "not set" << endl;
		}

		/* You can also save the return value from ParserSet::add() if
		 * you feel the operator[].get<T>() stuff is a bit much */
		cout << "foobar: ";
		if(i.isSet()) {
			cout << i.get<int>() << endl;
		} else {
			cout << "not set" << endl;
		}
		cout << "alpha: ";
		if(ps["alpha"].isSet()) {
			cout << ps["alpha"].get<string>() << endl;
		} else {
			cout << "not set" << endl;
		}

		cout << "rps: ";
		if(ps["rps"].isSet()) {
			cout << ps["rps"].get<enum RockPaperScissor>() << endl;
		} else {
			cout << "not set" << endl;
		}

	} catch(Parameter::ParameterRejected &p){
		// This will happen if the user has fed some malformed parameter to the program
		cerr << p.what() << endl;
		optp.usage();
		return EXIT_FAILURE;
	} catch(runtime_error &e) {
		// This will happen if you try to access a parameter that hasn't been set
		cerr << e.what() << endl;

		return EXIT_FAILURE;
	}


	// List what non-parameter options were given (typically files)
	cout << "The following file arguments were given:" << endl;

	vector<string> files = optp.getFiles();
	for(vector<string>::iterator i = files.begin(); i != files.end(); i++) {
		cout << "\t" << *i << endl;
	}


	return EXIT_SUCCESS;
}

#endif
