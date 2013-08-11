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

    if (hasShortOption())
    {
        strstr << "-" << shortOption() << ", ";
    }
    else
    {
        strstr << "    ";
    }
    strstr.width(20);
    strstr << std::left << "--" + longOption();

    return strstr.str();
}

/*
 *
 * Class SwitchParameter
 *
 *
 */

template<typename SwitchingBehavior>
SwitchParameter<SwitchingBehavior>::SwitchParameter(char shortOption, const char *longOption,
            const char* description) : CommonParameter<SwitchingBehavior>(shortOption, longOption, description) {}
template<typename SwitchingBehavior>
SwitchParameter<SwitchingBehavior>::~SwitchParameter() {}

template<typename SwitchingBehavior>
int SwitchParameter<SwitchingBehavior>::receive(ParserState& state) throw(Parameter::ParameterRejected) {

    const std::string arg = state.get();

    if(arg.at(0) != '-') return false;

    if ((arg.at(1) == '-' && arg.substr(2) == this->longOption()) ||
            (this->hasShortOption() && arg.at(1) == this->shortOption() && arg.length() == 2))
    {
        this->set();
        return 1;
    }

    return 0;
}

/*
 * PODParameter stuff
 *
 */


template<typename T, typename SwitchingBehavior>
PODParameter<T, SwitchingBehavior>::PODParameter(char shortOption, const char *longOption,
		const char* description) : CommonParameter<PresettableUniquelySwitchable>(shortOption, longOption, description) {}

template<typename T, typename SwitchingBehavior>
PODParameter<T, SwitchingBehavior>::~PODParameter() {}

template<typename T, typename SwitchingBehavior>
PODParameter<T, SwitchingBehavior>::operator T() const { return getValue(); }

template<typename T, typename SwitchingBehavior>
void PODParameter<T, SwitchingBehavior>::setDefault(T value) {
	PresettableUniquelySwitchable::preset();
	this->value = value;
}

template<typename T, typename SwitchingBehavior>
T PODParameter<T, SwitchingBehavior>::getValue() const {
    if(!this->isSet()) {
        throw std::runtime_error(
                std::string("Attempting to retreive the argument of parameter") + this->longOption() + " but it hasn't been set!");
	}
	return value;

}

template<typename T, typename SwitchingBehavior>
std::string PODParameter<T, SwitchingBehavior>::usageLine() const {
    std::stringstream strstr;

    if (this->hasShortOption())
    {
        strstr << "-" << this->shortOption() << ", ";
    }
    else
    {
        strstr << "    ";
    }
	strstr.width(20);
    strstr << std::left << "--" + this->longOption() + " <arg>";

	return strstr.str();
}

template<typename T, typename SwitchingBehavior>
int PODParameter<T, SwitchingBehavior>::receive(ParserState& state) throw(Parameter::ParameterRejected) {

    const std::string arg = state.get();

    if(arg.at(0) != '-') return false;

    if((arg.at(1) == '-' && arg.substr(2) == this->longOption()) ||
            (this->hasShortOption() && arg.at(1) == this->shortOption() && arg.length() == 2))
    {
        // retrieve the argument
        std::string arg1 = state.peek();
        if (arg1.length() == 0)
        {
            throw Parameter::ExpectedArgument(arg + ": expected an argument");
            return 1;
        }

        this->set();
        value = this->validate(arg1);

        return 2;
    }

    return 0;
}

#endif
