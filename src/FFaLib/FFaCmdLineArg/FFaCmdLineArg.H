// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaCmdLineArg.H
  \brief General command-line option handler.
*/

#ifndef FFA_CMD_LINE_ARG_H
#define FFA_CMD_LINE_ARG_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cctype>


/*!
  \brief Abstract interface for command-line options.
*/

class FFaCmdLineEntryBase
{
protected:
  //! \brief The constructor initializes the class attributes.
  FFaCmdLineEntryBase(const std::string& text, bool showToAll)
    : myHelpText(text), iAmSetOnCmdLine(false), isPublic(showToAll) {}

public:
  //! \brief Empty destructor.
  virtual ~FFaCmdLineEntryBase() {}
  //! \brief Checks if this option has been assigned a non-empty value.
  virtual bool hasValue() const = 0;
  //! \brief Checks if this option has been assigned a non-empty default value.
  virtual bool hasDefault() const = 0;
  //! \brief Converts the command-line option value from the given string.
  virtual int convertOption(const std::string&) = 0;
  //! \brief Composes the default section of the help text for this option.
  virtual std::string getDefaultString() const = 0;
  //! \brief Returns a string representation of the command-line option value.
  virtual std::string getValueString(bool = false) const = 0;
  //! \brief Resets the option to its default value.
  virtual void reset() = 0;

protected:
  std::string myHelpText; //!< Text description of this command-line option
  bool iAmSetOnCmdLine;   //!< \e true if this option has been specified
  bool isPublic;          //!< \e true unless this option is hidden

  friend class FFaCmdLineArg;
};


/*!
  \brief Template class for command-line options of arbitrary value type.
*/

template<class T> class FFaCmdLineEntry : public FFaCmdLineEntryBase
{
public:
  //! \brief The constructor initializes the class attributes and option values.
  FFaCmdLineEntry(const std::string& text, const T& value, bool showToAll)
    : FFaCmdLineEntryBase(text,showToAll), myValue(value), myDefault(value) {}
  //! \brief Empty destructor.
  virtual ~FFaCmdLineEntry() {}

  //! \brief Checks if this option has been assigned a non-empty value.
  virtual bool hasValue() const { return true; }
  //! \brief Checks if this option has been assigned a non-empty default value.
  virtual bool hasDefault() const { return true; }

  //! \brief Converts the command-line option value from the given string.
  virtual int convertOption(const std::string&) { return -1; }

  //! \brief Composes the default section of the help text for this option.
  virtual std::string getDefaultString() const
  {
    return this->toString(myDefault);
  }

  //! \brief Returns a string representation of the command-line option value.
  virtual std::string getValueString(bool noDefaults) const
  {
    if (noDefaults && !iAmSetOnCmdLine)
      return std::string();

    return this->toString(myValue);
  }

  //! \brief Returns the actual command-line option value.
  const T& getValue() const { return myValue; }

  //! \brief Resets the option to its default value.
  virtual void reset() { myValue = myDefault; iAmSetOnCmdLine = false; }

protected:
  //! \brief Returns the string representation of the specified value.
  virtual std::string toString(const T& value) const
  {
    std::stringstream sval;
    sval << value;
    return sval.str();
  }

private:
  T myValue;   //!< The assigned value of this command-line option
  T myDefault; //!< The default value of this command-line option
};


/*!
  \brief Singleton class for management of command-line options.
*/

class FFaCmdLineArg
{
  //! Data type for the command-line options container
  typedef std::map<std::string,FFaCmdLineEntryBase*> OptionMap;

private:
  //! \brief The default constructor is private to avoid other instances.
  FFaCmdLineArg();
  //! \brief The destructor deletes the allocated FFaCmdLineEntry objects.
  ~FFaCmdLineArg();

public:
  //! \brief Initializes the command-line options singleton object.
  static void init(int argc, char** argv) { instance()->initArgs(argc,argv); }
  //! \brief Returns \e true if no command-line options have been defined.
  static bool empty();
  //! \brief Returns the one and only instance of this class.
  static FFaCmdLineArg* instance();
  //! \brief Deletes the dynamically allocated instance.
  static void removeInstance();

  //! \brief Defines a new command-line option of arbitrary type.
  //! \param[in] identifier The actual option is "-<identifier>"
  //! \param[in] value Default value of this option
  //! \param[in] helpText Textual decription printed via the option "-help"
  //! \param[in] showToAll \e true if this option is public
  template<class T> void addOption(const std::string& identifier,
                                   const T&           value,
                                   const std::string& helpText,
                                   bool showToAll = true)
  {
    // In case the same option is defined twice, avoid memory leak
    OptionMap::iterator it = myOptions.find(identifier);
    if (it != myOptions.end()) delete it->second;

    myOptions[identifier] = new FFaCmdLineEntry<T>(helpText,value,showToAll);
  }

  //! \brief Defines a new command-line option of string type.
  //! \param[in] identifier The actual option is "-<identifier>"
  //! \param[in] value Default value of this option
  //! \param[in] helpText Textual decription printed via the option "-help"
  //! \param[in] showToAll \e true if this option is public
  void addOption(const std::string& identifier,
                 const char*        value,
                 const std::string& helpText,
                 bool showToAll = true)
  {
    this->addOption(identifier,std::string(value),helpText,showToAll);
  }

  //! \brief Returns the actual value of the specified command-line option.
  //! \param[in] identifier The option to return the value for
  //! \param[out] aVal The actual value of the command-line option
  //! \return \e true if the specified option was defined, otherwise \a false
  template<class T> bool getValue(const std::string& identifier, T& aVal)
  {
    this->evaluate();

    OptionMap::const_iterator opIt = myOptions.find(identifier);
    if (opIt != myOptions.end())
    {
      FFaCmdLineEntry<T>* obj = dynamic_cast<FFaCmdLineEntry<T>*>(opIt->second);
      if (obj)
      {
        aVal = obj->getValue();
        return true;
      }
    }

    if (!mute)
      std::cerr <<" *** Could not evaluate command-line option \""
                << identifier <<"\""<< std::endl;
    return false;
  }

  //! \brief Assigns a new value to the specified command-line option.
  //! \param[in] identifier The option to assign new value for
  //! \param[in] value New value of this option
  //! \return \e true if the specified option was defined, otherwise \a false
  bool setValue(const std::string& identifier, const std::string& value);

  //! \brief Creates a multi-line string with a list of all defined options.
  //! \param[out] report A report with option descriptions and default values
  //! \param[in] all If \e true, include also private (normally hidden) options
  void composeHelpText(std::string& report, bool all = false) const;
  //! \brief Same as composeHelpText(), but with each option as a single line.
  void composeSingleLineHelpText(std::string& report, bool all = false) const;
  //! \brief Prints out all specified command-line options.
  //! \param[in] noDefaults If \e true, print out all defined options
  void listOptions(bool noDefaults = false) const;
  //! \brief Checks if an option \a identifier is specified on the command-line.
  bool isOptionSetOnCmdLine(const std::string& identifier);
  //! \brief Reads command-line option values from the file \a fileName.
  bool readOptionsFile(const std::string& fileName);

  //! This text is printed after the option list itself when using "-help"
  static std::string additionalHelpText;

  static bool mute; //!< If \e true, error messages are muted

private:
  //! \brief Copies all command-line arguments into a temporary container.
  void initArgs(int argc, char** argv);
  //! \brief Processes the temporary command-line option list \a myArgs.
  void evaluate();

  std::vector<std::string> myArgs;    //!< Temporary command-line option storage
  OptionMap                myOptions; //!< Command-line options container

  static FFaCmdLineArg* myInstance; //!< The actual instance of this class
};


////////////////////////////////////////////////////////////////////////////////
// Template specializations
////////////////////////////////////////////////////////////////////////////////

typedef std::vector<int>    IntVec;    //!< Convenience type definition
typedef std::vector<double> DoubleVec; //!< Convenience type definition


/*!
  \brief Checks if a string option has been assigned a non-empty value.
*/

template<> inline
bool FFaCmdLineEntry<std::string>::hasValue() const
{
  return !myValue.empty();
}

/*!
  \brief Checks if a string option has been assigned a non-empty default value.
*/

template<> inline
bool FFaCmdLineEntry<std::string>::hasDefault() const
{
  return !myDefault.empty();
}


/*!
  \brief Checks if an int vector option has been assigned a non-empty
  default value.
*/

template<> inline
bool FFaCmdLineEntry<IntVec>::hasDefault() const
{
  return !myDefault.empty();
}


/*!
  \brief Checks if a double vector option has been assigned a non-empty
  default value.
*/

template<> inline
bool FFaCmdLineEntry<DoubleVec>::hasDefault() const
{
  return !myDefault.empty();
}


/*!
  \brief Composes the default section of the help text for a string option.
*/

template<> inline
std::string FFaCmdLineEntry<std::string>::getDefaultString() const
{
  return myDefault;
}

/*!
  \brief Returns a string representation of a string option value.
*/

template<> inline
std::string FFaCmdLineEntry<std::string>::getValueString(bool noDefaults) const
{
  if (noDefaults && myValue == myDefault)
    return std::string();

  return myValue;
}


/*!
  \brief Composes the default section of the help text for a string option.
*/

template<> inline
std::string FFaCmdLineEntry<bool>::getDefaultString() const
{
  return std::string(myDefault ? "+ (true)" : "- (false)");
}


/*!
  \brief Returns a string representation of a bool option value.
*/

template<> inline
std::string FFaCmdLineEntry<bool>::getValueString(bool noDefaults) const
{
  if (noDefaults && myValue == myDefault)
    return std::string();

  return std::string(myValue ? "+ (true)" : "- (false)");
}


/*!
  \brief Converts the command-line option to an int value.
*/

template<> inline
int FFaCmdLineEntry<int>::convertOption(const std::string& value)
{
  if (value.empty())
    return 0;

  char* endPtr;
  long int aval = strtol(value.c_str(),&endPtr,10);
  if (*endPtr) return 1 + (endPtr - value.c_str());

  myValue = (int)aval;
  iAmSetOnCmdLine = true;
  return 0;
}


/*!
  \brief Converts the command-line option to a float value.
*/

template<> inline
int FFaCmdLineEntry<float>::convertOption(const std::string& value)
{
  if (value.empty())
    return 0;

  char* endPtr;
  float aval = strtof(value.c_str(),&endPtr);
  if (*endPtr) return 1 + (endPtr - value.c_str());

  myValue = aval;
  iAmSetOnCmdLine = true;
  return 0;
}


/*!
  \brief Converts the command-line option to a double value.
*/

template<> inline
int FFaCmdLineEntry<double>::convertOption(const std::string& value)
{
  if (value.empty())
    return 0;

  char* endPtr;
  double aval = strtod(value.c_str(),&endPtr);
  if (*endPtr) return 1 + (endPtr - value.c_str());

  myValue = aval;
  iAmSetOnCmdLine = true;
  return 0;
}


/*!
  \brief Converts the command-line option to a vector of integers.
*/

template<> inline
int FFaCmdLineEntry<IntVec>::convertOption(const std::string& value)
{
  myValue.clear();
  if (value.empty())
    return 0;

  char* endPtr;
  long int aval = strtol(value.c_str(),&endPtr,10);
  while (*endPtr)
  {
    char* next = endPtr;
    if (!isdigit(next[0]) && !isspace(next[0]) &&
        next[0] != '-' && next[0] != '+')
      return 1 + (endPtr - value.c_str());

    myValue.push_back((int)aval);
    aval = strtol(next,&endPtr,10);
  }

  myValue.push_back(aval);
  iAmSetOnCmdLine = true;
  return 0;
}


/*!
  \brief Converts the command-line option to a vector of doubles.
*/

template<> inline
int FFaCmdLineEntry<DoubleVec>::convertOption(const std::string& value)
{
  myValue.clear();
  if (value.empty())
    return 0;

  char* endPtr;
  double aval = strtod(value.c_str(),&endPtr);
  while (*endPtr)
  {
    char* next = endPtr;
    if (!isdigit(next[0]) && !isspace(next[0]) &&
        next[0] != '-' && next[0] != '+' && next[0] != '.')
      return 1 + (endPtr - value.c_str());

    myValue.push_back(aval);
    aval = strtod(next,&endPtr);
  }

  myValue.push_back(aval);
  iAmSetOnCmdLine = true;
  return 0;
}


/*!
  \brief Helper function converting a vector of something to a string.
*/

template<class T> inline
std::string vec2string(const std::vector<T>& value)
{
  if (value.empty())
    return "";

  std::stringstream sval;
  sval << value.front();
  for (size_t i = 1; i < value.size(); i++)
    sval <<" "<< value[i];

  return sval.str();
}

/*!
  \brief Returns a string representation of a vector of integers.
*/

template<> inline
std::string FFaCmdLineEntry<IntVec>::toString(const IntVec& value) const
{
  return vec2string(value);
}

/*!
  \brief Returns a string representation of a vector of doubles.
*/

template<> inline
std::string FFaCmdLineEntry<DoubleVec>::toString(const DoubleVec& value) const
{
  return vec2string(value);
}


/*!
  \brief Converts the command-line option to a string value.
*/

template<> inline
int FFaCmdLineEntry<std::string>::convertOption(const std::string& value)
{
  if (value.empty())
    return 0;

  myValue = value;
  if (myValue.size() > 1)
  {
    // remove fnutts if they are first and last in the string:
    if (*myValue.begin()  == '"') myValue.erase(myValue.begin());
    if (*myValue.rbegin() == '"') myValue.erase(myValue.end()-1);
  }
  iAmSetOnCmdLine = true;
  return 0;
}


/*!
  \brief Converts the command-line option to a bool value.
*/

template<> inline
int FFaCmdLineEntry<bool>::convertOption(const std::string& value)
{
  if (value.empty())
    myValue = true;
  else if (value.size() != 1)
    return false;
  else if (value == "+")
    myValue = true;
  else if (value == "-")
    myValue = false;
  else
    return 1;

  iAmSetOnCmdLine = true;
  return 0;
}


/*!
  \brief Helper function completing a vector-valued option value from defaults.
*/

template<class T> inline
const std::vector<T>& complDefault(const std::vector<T>& value,
                                   const std::vector<T>& defval)
{
  if (value.empty())
    return defval;
  else if (value.size() >= defval.size())
    return value;

  static std::vector<T> tmp;
  tmp.assign(value.begin(),value.end());
  tmp.insert(tmp.end(),defval.begin()+value.size(),defval.end());
  return tmp;
}

/*!
  \brief Returns a vector of doubles command-line option value.
*/

template<> inline
const DoubleVec& FFaCmdLineEntry<DoubleVec>::getValue() const
{
  return complDefault(myValue,myDefault);
}

/*!
  \brief Returns a vector of integers command-line option value.
*/

template<> inline
const IntVec& FFaCmdLineEntry<IntVec>::getValue() const
{
  return complDefault(myValue,myDefault);
}

#endif
