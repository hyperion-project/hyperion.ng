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

 /*
  * Modifications:
  *
  * - Removed using namespace std from header
  * - Changed Parameter container type from std::set to std::list to presume order
  * - Changed arguments of Parameters to be a seperated arguments on the command line
  * - Make the choice of receiving arguments or not in subclasses of CommonParameter
  */

#include <list>
#include <vector>
#include <stdexcept>
#include <string>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <iostream>

#ifndef GETOPTPP_H
#define GETOPTPP_H

namespace vlofgren {

class Parameter;
class ParserState;

class OptionsParser;

/** Container for a set of parameters */

class ParameterSet {
public:

	/** Find a parameter by short option form */
	Parameter& operator[](char c) const;

	/** Find a parameter by long option form. */
	Parameter& operator[](const std::string &s) const;

	/** Factory method that adds a new parameter of
	 * type T to the set.
	 *
	 * This is just for convenience. It allows ParameterSet
	 * to manage the pointers, as well as (usually) making the
	 * code slightly easier to read.
	 *
	 * Do not try to add non-Parameter types lest you will invoke
	 * the wrath of gcc's template error messages.
	 *
	 * @returns The created parameter. The reference is valid
	 * 			as long as ParameterSet exists.
	 */
	template<typename T>
	T &add(char shortName, const char* longName, const char* description);

	ParameterSet() {}
	~ParameterSet();
protected:
	friend class OptionsParser;
	std::list<Parameter*> parameters;

private:
	ParameterSet(const ParameterSet& ps);
};

/** getopt()-style parser for command line arguments
 *
 * Matches each element in argv against given
 * parameters, and collects non-parameter arguments
 * (typically files) in a vector.
 *
 */

class OptionsParser {
public:
	OptionsParser(const char *programDesc);
	virtual ~OptionsParser();

	ParameterSet& getParameters();

	/** Parse command line arguments */
	void parse(int argc, const char* argv[]) throw(std::runtime_error);

	/** Generate a usage screen */
	void usage() const;

	/** Return the name of the program, as
	 * given by argv[0]
	 */
	const std::string& programName() const;

	/** Return a vector of each non-parameter */
	const std::vector<std::string>& getFiles() const;
protected:
	std::string argv0;
	std::string fprogramDesc;

	ParameterSet parameters;
	std::vector<std::string> files;

	friend class ParserState;
};

/**
 * Corresponds to the state of the parsing, basically just a wrapper
 * for a const_iterator that handles nicer.
 */

class ParserState {
public:
	const std::string peek() const;
	const std::string get() const;
	void advance();
	bool end() const;
protected:
	ParserState(/*OptionsParser &opts,*/ std::vector<std::string>& args);
private:
	friend class OptionsParser;

//	OptionsParser &opts;
	const std::vector<std::string> &arguments;
	std::vector<std::string>::const_iterator iterator;
};

/**
 *
 * Abstract base class of all parameters
 *
 */

class Parameter {
public:

	/** Generic exception thrown when a parameter is malformed
	 */
	class ParameterRejected : public std::runtime_error {
	public:
		ParameterRejected(const std::string& s) : std::runtime_error(s) {}
		ParameterRejected() : runtime_error("") {}
	};

	/** Exception thrown when a parameter did not expect an argument */
	class UnexpectedArgument : public ParameterRejected {
	public:
		UnexpectedArgument(const std::string &s) : ParameterRejected(s) {}
		UnexpectedArgument() {}
	};

	/** Exception thrown when a parameter expected an argument */
	class ExpectedArgument : public ParameterRejected {
	public:
		ExpectedArgument(const std::string &s) : ParameterRejected(s) {}
		ExpectedArgument() {}
	};

	Parameter(char shortOption, const std::string & longOption, const std::string & description);

	virtual ~Parameter();

	/** Test whether the parameter has been set */
	virtual bool isSet() const = 0;

	/** This parameter's line in OptionsParser::usage() */
	virtual std::string usageLine() const = 0;

	/** Description of the parameter (rightmost field in OptionsParser::usage()) */
	const std::string& description() const;

	/** The long name of this  parameter (e.g. "--option"), without the dash. */
	const std::string& longOption() const;

	/** Check if this parameters has a short option */
	bool hasShortOption() const;

	/** The short name of this parameter (e.g. "-o"), without the dash. */
	char shortOption() const;

protected:

	/** Receive a potential parameter from the parser (and determien if it's ours)
	 *
	 * The parser will pass each potential parameter through it's registered parameters'
	 * receive function.
	 *
	 * @throw ParameterRejected if the parameter belongs to us, but is malformed somehow.
	 *
	 * @param state Allows access to the current argument.  This is a fairly powerful
	 * 				   iterator that technically allows for more complex grammar than what is
	 * 				   presently used.
	 */
	virtual int receive(ParserState& state) throw(ParameterRejected) = 0;

	friend class OptionsParser;

	char fshortOption;
	const std::string flongOption;
	const std::string fdescription;
private:

};

/*
 *
 * Abstract base class of all parameters
 *
 */

class Switchable;

/** Base class for most parameter implementations.
 *
 * It parses the argument in receive() and if it matches,
 * calls receiveSwitch() or receiveArgument() which are implemented
 * in child classes.
 *
 * The SwitchingBehavior mixin determines what happens if the argument
 * is set multiple times.
 */

template<typename SwitchingBehavior=Switchable>
class CommonParameter : public Parameter, protected SwitchingBehavior {
public:

	/** Test whether the parameter has been set */
	virtual bool isSet() const;

	CommonParameter(char shortOption, const char *longOption,
			const char* description);
	virtual ~CommonParameter();

	virtual std::string usageLine() const;

protected:
	/** Parse the argument given by state, and dispatch either
	 * receiveSwitch() or receiveArgument() accordingly.
	 *
	 * @param state The current argument being parsed.
	 * @return The number of parameters taken from the input
	 */
	virtual int receive(ParserState& state) throw(ParameterRejected) = 0;
};

/** This class (used as a mixin) defines how a parameter
 * behaves when switched on, specifically when switched on multiple times.
 *
 */
class Switchable {
public:
	class SwitchingError : public Parameter::ParameterRejected {};

	/** Test whether the parameter has been set */
	virtual bool isSet() const;

	/** Set the parameter
	 *
	 */
	virtual void set() throw (SwitchingError) = 0;

	virtual ~Switchable();
	Switchable();
protected:
	bool fset;
};

/** Switching behavior that does not complain when set multiple times. */
class MultiSwitchable : public Switchable {
public:
	virtual ~MultiSwitchable();
	virtual void set() throw(SwitchingError);

};

/** Switching behavior that allows switching only once.
 *
 * This is typically what you want if your parameter has an argument.
 *
 */
class UniquelySwitchable : public Switchable {
public:

	virtual ~UniquelySwitchable();

	/** Set the parameter
	 *
	 * @throw SwitchingError Thrown if the parameter is already set.
	 */
	virtual void set() throw (SwitchingError);
};

/** Switching behavior that makes possible allows presettable parameters,
 * that is, it can either be set by the program, or by a command line argument,
 * and the command-line part is UniquelySwitchable, but the program part
 * is MultiSwitchable (and is set by preset())
 *
 *
 */
class PresettableUniquelySwitchable : public UniquelySwitchable {
public:

	/** Test whether the parameter has been set OR preset */
	virtual bool isSet() const;

	/** Call if the parameter has been set.
	 *
	 * @throw SwitchingError thrown if the parameter is already set
	 * (doesn't care if it's been pre-set)
	 */
	virtual void set() throw (Switchable::SwitchingError);

	/** Call if the parameter has been preset */
	virtual void preset();

	virtual ~PresettableUniquelySwitchable();
private:
	MultiSwitchable fpreset;
};

/* Parameter that does not take an argument, and throws an exception
 * if an argument is given */

template<typename SwitchingBehavior=MultiSwitchable>
class SwitchParameter : public CommonParameter<SwitchingBehavior> {
public:
	SwitchParameter(char shortOption, const char *longOption,
			const char* description);
	virtual ~SwitchParameter();

protected:
	virtual int receive(ParserState& state) throw(Parameter::ParameterRejected);
};

/** Plain-Old-Data parameter. Performs input validation.
 *
 * Currently only supports int, long and double, but extending
 * it to other types (even non-POD) is as easy as partial template specialization.
 *
 * Specifically, you need to specialize validate().
 */

template<typename T, typename SwitchingBehavior=PresettableUniquelySwitchable>
class PODParameter : public CommonParameter<SwitchingBehavior> {
public:
	PODParameter(char shortOption, const char *longOption,
			const char* description);
	virtual ~PODParameter();

	/* Retreive the value of the argument. Throws an exception if
	 * the value hasn't been set (test with isSet())
	 */
	T getValue() const;

	/** Type-casting operator, for convenience. */
	operator T() const;

	/** Set a default value for this parameter */
	virtual void setDefault(T value);

	std::string usageLine() const;
protected:
	virtual int receive(ParserState& state) throw(Parameter::ParameterRejected);

	/** Validation function for the data type.
	 *
	 * @throw ParameterRejected if the argument does not conform to this data type.
	 * @return the value corresponding to the argument.
	 */
	virtual T validate(const std::string& s) throw (Parameter::ParameterRejected);

	T value;
};


typedef PODParameter<int> IntParameter;
typedef PODParameter<long> LongParameter;
typedef PODParameter<double> DoubleParameter;
typedef PODParameter<std::string> StringParameter;

#include "parameter.include.cc"

} //namespace

#endif
