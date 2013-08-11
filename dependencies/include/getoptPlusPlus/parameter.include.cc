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


#ifdef GETOPTPP_H


/* This file contains template voodoo, and due to the unique way GCC handles
 * templates, it needs to be included as a header (and it -is-). Do not attempt to
 * compile this file directly!
 */

template<typename T>
T &ParameterSet::add(char shortName, const char* longName, const char* description) {
	T* p = new T(shortName, longName, description);
    parameters.push_back(p);
	return *p;
}

/*
 *
 * Class CommonParameter implementation
 *
 *
 */

template<typename SwitchingBehavior>
CommonParameter<SwitchingBehavior>::~CommonParameter() {}

template<typename SwitchingBehavior>
CommonParameter<SwitchingBehavior>::CommonParameter(char shortOption, const char *longOption,
			const char* description) : Parameter(shortOption, longOption, description) {}

template<typename SwitchingBehavior>
bool CommonParameter<SwitchingBehavior>::isSet() const{
	return SwitchingBehavior::isSet();
}

template<typename SwitchingBehavior>
std::string CommonParameter<SwitchingBehavior>::usageLine() const {
    std::stringstream strstr;

    strstr.width(10);
    if (hasShortOption())
    {
        strstr << std::left<< std::string("-") + shortOption();
    }
    else
    {
        strstr << " ";
    }
		strstr.width(20);
        strstr << std::left << "--" + longOption();

		return strstr.str();
}



template<typename SwitchingBehavior>
bool CommonParameter<SwitchingBehavior>::receive(ParserState& state) throw(Parameter::ParameterRejected) {

    const std::string arg = state.get();

	try {
		if(arg.at(0) != '-') return false;

		if(arg.at(1) == '-') { /* Long form parameter */
			try {
				unsigned int eq = arg.find_first_of("=");

                if(eq == std::string::npos) {
                    if(arg.substr(2) != longOption())
                        return false;

                    this->receiveSwitch();
                } else {
                    if(arg.substr(2, eq-2) != longOption())
                        return false;

                    this->receiveArgument(arg.substr(eq+1));
                }
				return true;
			} catch(Parameter::ExpectedArgument &ea) {
				throw ExpectedArgument("--" + longOption() + ": expected an argument");
			} catch(Parameter::UnexpectedArgument &ua) {
				throw UnexpectedArgument("--" + longOption() + ": did not expect an argument");
			} catch(Switchable::SwitchingError &e) {
				throw ParameterRejected("--" + longOption() + ": parameter already set");
			} catch(Parameter::ParameterRejected &pr) {

                std::string what = pr.what();
				if(what.length())
					throw Parameter::ParameterRejected("--" + longOption() + ": " + what);
				throw Parameter::ParameterRejected("--" + longOption() + " (unspecified error)");
			}
		}
        else if (hasShortOption())
        {
            try {
                if(arg.at(1) == shortOption()) {
                    /* Matched argument on the form -f or -fsomething */
                    if(arg.length() == 2) { /* -f */
                        this->receiveSwitch();

                        return true;
                    } else { /* -fsomething */
                        this->receiveArgument(arg.substr(2));

                        return true;
                    }
                }
            } catch(Parameter::ExpectedArgument &ea) {
                throw ExpectedArgument(std::string("-") + shortOption() + ": expected an argument");
            } catch(Parameter::UnexpectedArgument &ua) {
                throw UnexpectedArgument(std::string("-") + shortOption() + ": did not expect an argument");
            } catch(Switchable::SwitchingError &e) {
                throw ParameterRejected(std::string("-") + shortOption() + ": parameter already set");
            }
        }
    } catch(std::out_of_range& o) {
		return false;
	}

	return false;
}



/*
 * PODParameter stuff
 *
 */


template<typename T>
PODParameter<T>::PODParameter(char shortOption, const char *longOption,
		const char* description) : CommonParameter<PresettableUniquelySwitchable>(shortOption, longOption, description) {}

template<typename T>
PODParameter<T>::~PODParameter() {}

template<typename T>
PODParameter<T>::operator T() const { return getValue(); }

template<typename T>
void PODParameter<T>::setDefault(T value) {
	PresettableUniquelySwitchable::preset();
	this->value = value;
}

template<typename T>
T PODParameter<T>::getValue() const {
	if(!isSet()) {
		throw runtime_error(
                std::string("Attempting to retreive the argument of parameter") + longOption() + " but it hasn't been set!");
	}
	return value;

}


template<typename T>
std::string PODParameter<T>::usageLine() const {
    std::stringstream strstr;

    strstr.width(10);
    if (hasShortOption())
    {
        strstr << std::left << std::string("-") + shortOption() +"arg";
    }
    else
    {
        strstr << "";
    }
	strstr.width(20);
    strstr << std::left << "--" + longOption() + "=arg";

	return strstr.str();
}

template<typename T>
void PODParameter<T>::receiveSwitch() throw (Parameter::ParameterRejected) {
	throw Parameter::ExpectedArgument();
}

template<typename T>
void PODParameter<T>::receiveArgument(const std::string &argument) throw(Parameter::ParameterRejected) {
	set();
	value = this->validate(argument);
}


#endif
