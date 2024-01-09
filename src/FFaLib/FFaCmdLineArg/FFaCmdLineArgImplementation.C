// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaCmdLineArgImplementation.C
  \brief General command-line option handler.
*/

#include <fstream>

#include "FFaLib/FFaDefinitions/FFaMsg.H"
#include "FFaLib/FFaString/FFaStringExt.H"
#include "FFaLib/FFaCmdLineArg/FFaCmdLineArg.H"

#if defined(win32) || defined(win64)
//! \brief Valid characters for switch word prefix on Windows.
#define SWITCH_PREFIX "-/"
#else
//! \brief Valid characters for switch word prefix on UNIX.
#define SWITCH_PREFIX "-"
#endif


/*!
  \class FFaCmdLineArg

  All options are in the standard form &lt;switch word&gt;[ ]&lt;option&gt;,
  and each switch word must be prefixed with either '-' or '/'.
  (only '-' is valid on UNIX)

  The command argument parser can be used this way:

  \code
  int main(int argc, char** argv)
  {
    FFaCmdLineArg::init(argc,argv);

    FFaCmdLineArg::instance()->addOption("f", "untitled.dat", "Input filename");
    FFaCmdLineArg::instance()->addOption("debug", false, "Debug mode");
    FFaCmdLineArg::instance()->addOption("n32", false, "n32 mode");
    FFaCmdLineArg::instance()->addOption("rate", 2.0, "sample rate");

    double rate;
    FFaCmdLineArg::instance()->getValue("rate", rate);
    std::cout << rate << std::endl;
    ...
  }
  \endcode

  \author Jens Lien
*/


std::string FFaCmdLineArg::additionalHelpText;

bool FFaCmdLineArg::mute = false;

FFaCmdLineArg* FFaCmdLineArg::myInstance = NULL;


FFaCmdLineArg::FFaCmdLineArg()
{
#ifdef FFA_DEBUG
  std::cout <<"Creating the FFaCmdLineArg instance."<< std::endl;
#endif
}


FFaCmdLineArg::~FFaCmdLineArg()
{
#ifdef FFA_DEBUG
  std::cout <<"Destroying the FFaCmdLineArg instance."<< std::endl;
#endif
  for (OptionMap::value_type& opt : myOptions)
    delete opt.second;
}


bool FFaCmdLineArg::empty()
{
  return myInstance ? myInstance->myOptions.empty() : true;
}


FFaCmdLineArg* FFaCmdLineArg::instance()
{
  if (!myInstance)
    myInstance = new FFaCmdLineArg();

  return myInstance;
}


void FFaCmdLineArg::removeInstance()
{
  delete myInstance;
  myInstance = NULL;
}


void FFaCmdLineArg::initArgs(int argc, char** argv)
{
  for (int i = 1; i < argc; i++)
    myArgs.push_back(argv[i]);

  for (OptionMap::value_type& opt : myOptions)
    opt.second->reset();
}


bool FFaCmdLineArg::setValue(const std::string& identifier,
                             const std::string& value)
{
  OptionMap::iterator opIt = myOptions.find(identifier);
  if (opIt == myOptions.end())
  {
    if (!mute)
      std::cerr <<"  ** Command-line option "<< identifier <<" not defined."
                << std::endl;
    return false;
  }

  opIt->second->iAmSetOnCmdLine = false;
  myArgs.push_back("-"+identifier);
  myArgs.push_back(value);
  return true;
}


void FFaCmdLineArg::evaluate()
{
  for (size_t i = 0; i < myArgs.size(); i++)
  {
    // Is this argument value an option string?
    bool found = false;
    if (myArgs[i].find_first_of(SWITCH_PREFIX) == 0)
    {
      // Yes, check the option map for this option (ignoring case differences)
      FFaLowerCaseString anOption(myArgs[i].substr(1));
      for (OptionMap::value_type& opt : myOptions)
        if (anOption.find(FFaLowerCaseString(opt.first)) == 0)
        {
          // The option was found, now extract its value (if any)
          std::string argument;
          bool stopSearch = found = true;
          // Is the option value in the same string?
          size_t n = opt.first.size();
          if (anOption.size() > n)
          {
            if (anOption[n] == '=') // assumed syntax "-<option>=<value>"
              argument = myArgs[i].substr(n+2);
            else
            {
              // Assumed syntax "-<option><value>"
              argument = myArgs[i].substr(n+1);
              stopSearch = false; // continue the search for a better match
            }
          }
          // In the case of a bool option, do not use the next argument
          else if (dynamic_cast<FFaCmdLineEntry<bool>*>(opt.second))
          {
            argument = "+"; // assumed syntax "-<option>" (= "-<option>+")
          }
          else if (i+1 < myArgs.size())
          {
            // Assumed syntax "-<option> <value> [<value2> <value3> ...]"
            // Concatenate argument strings until the next SWITCH_PREFIX
            // Ignore strings like "-33", which could be a number value
            while (i+1 < myArgs.size() &&
                   (myArgs[i+1].size() <= 1 ||
                    myArgs[i+1].find_first_of(SWITCH_PREFIX) > 0 ||
                    isdigit(myArgs[i+1][1])))
              if (argument.empty())
                argument = myArgs[++i];
              else
                argument += " " + myArgs[++i];
          }
#if FFA_DEBUG > 1
          std::cout << opt.first <<" ["<< argument <<"]"<< std::endl;
#endif
          if (opt.second->iAmSetOnCmdLine)
            continue; // repeated option, use only the first instance

          int invalid = opt.second->convertOption(argument);
          if (invalid > 0)
            std::cerr <<"  ** Invalid option value for -"<< opt.first
                      <<": \""<< argument <<"\" (ignored).\n"
                      << std::string(33+opt.first.size()+invalid,' ')
                      <<"^"<< std::endl;
          if (stopSearch)
            break;
        }
    }

    if (!found && !mute)
      std::cerr <<"  ** Unknown command-line argument \""<< myArgs[i]
                <<"\" (ignored)."<< std::endl;
  }

  myArgs.clear();
}


void FFaCmdLineArg::composeHelpText(std::string& helpText, bool all) const
{
  // Find longest option string
  size_t longest = 0;
  for (const OptionMap::value_type& opt : myOptions)
    if (all || opt.second->isPublic)
      if (longest < opt.first.size())
        longest = opt.first.size();

  longest += 2; // add two spaces after the longest identifier

  const int indent = 8;
  const int newLineIndent = longest + indent;

  for (const OptionMap::value_type& opt : myOptions)
    if (all || opt.second->isPublic)
    {
      helpText += std::string(indent-1,' ');
      helpText += '-';
      helpText += opt.first;
      helpText += std::string(longest-opt.first.size(),' ');

      // Replace all newlines in myHelpText with the correct indentation
      for (char c : opt.second->myHelpText)
      {
        helpText += c;
        if (c == '\n')
          helpText += std::string(newLineIndent,' ');
      }
      helpText += '\n';

      // Here goes the default values
      if (opt.second->myHelpText.find("Default:") == std::string::npos)
        if (opt.second->hasDefault())
          helpText += std::string(newLineIndent,' ')
            + "Default: " + opt.second->getDefaultString() + "\n";
    }

  if (!additionalHelpText.empty())
    helpText += additionalHelpText;
}


void FFaCmdLineArg::composeSingleLineHelpText(std::string& helpText,
                                              bool all) const
{
  for (const OptionMap::value_type& opt : myOptions)
    if (all || opt.second->isPublic)
    {
      helpText += '-';
      helpText += opt.first;
      helpText += '\t';

      // Replace all newlines in helpText with the correct indentation
      bool addDefaultText = true;
      const std::string& myHelpText = opt.second->myHelpText;
      for (size_t i = 0; i < myHelpText.size(); i++)
        if (myHelpText[i] == '\n')
          helpText += ' ';
        else if (myHelpText.substr(i,9) == "Default: ")
        {
          // The help text contains the default value, ommit the defaultString
          addDefaultText = false;
          helpText += '\t';
          i += 8;
        }
        else
          helpText += myHelpText[i];

      // Here goes the default values
      if (addDefaultText && opt.second->hasDefault())
        helpText += "\t" + opt.second->getDefaultString();
      helpText += '\n';
    }

  if (!additionalHelpText.empty())
    helpText += additionalHelpText;
}


void FFaCmdLineArg::listOptions(bool noDefaults) const
{
  const_cast<FFaCmdLineArg*>(this)->evaluate();

  size_t longest = 0;
  std::map<std::string,std::string> givenOptions;
  for (const OptionMap::value_type& opt : myOptions)
  {
    std::string value = opt.second->getValueString(noDefaults);
    if (value.empty()) continue;

    if (longest < opt.first.size())
      longest = opt.first.size();

    givenOptions[opt.first] = value;
  }

  ListUI <<"\n  ** "<< (noDefaults ? "Specified" : "All")
         <<" command-line options **";
  for (const std::pair<const std::string,std::string>& opt : givenOptions)
    ListUI <<"\n     -"<< opt.first
           << std::string(longest+2-opt.first.size(),' ') << opt.second;
  ListUI <<"\n\n";
}


/*!
  Returns \e true if the option is specified, and \e false if it is not set.
  If the option is undefined, return \e false and print a warning to std::cerr.
*/

bool FFaCmdLineArg::isOptionSetOnCmdLine(const std::string& identifier)
{
  this->evaluate();

  OptionMap::const_iterator opIt = myOptions.find(identifier);
  if (opIt != myOptions.end())
    return opIt->second->iAmSetOnCmdLine ? opIt->second->hasValue() : false;

  if (!mute)
    std::cerr <<"  ** Command-line option "<< identifier <<" not defined."
              << std::endl;
  return false;
}


bool FFaCmdLineArg::readOptionsFile(const std::string& fileName)
{
  if (fileName.empty())
    return false;

  std::ifstream fs(fileName.c_str());
  if (!fs)
  {
    std::cerr <<" *** Could not open option file "<< fileName << std::endl;
    return false;
  }

  // Parse and append to myArgs

  char c = ' ';
  std::string tmpOpt;
  bool readingString = false;
  while (!fs.eof() && isspace(c))
    fs.get(c);

  while (!fs.eof())
  {
    if (c == '#' && !readingString)
    {
      // Skip comments
      fs.ignore(BUFSIZ,'\n');
      if (tmpOpt.empty()) // comment line - continue with the next line
      {
        fs.get(c);
        continue;
      }
      else // trailing comment - must process what we have got so far
        c = ' ';
    }

    // Check for space in the string - use a fnutt check
    if (c == '"')
      readingString = !readingString;

    if (readingString || !isspace(c))
    {
      tmpOpt += c;
      fs.get(c);
    }
    else // we have an OK string
    {
      myArgs.push_back(tmpOpt);
      tmpOpt.resize(0);
      while (!fs.eof() && isspace(c)) fs.get(c);
    }
  }

  // In case the last line is not terminated with a new-line character...
  if (!tmpOpt.empty())
  {
    if (!readingString)
      myArgs.push_back(tmpOpt);
    else if (!mute)
      std::cerr <<"  ** Ignoring non-terminated option string value: "
                << tmpOpt << std::endl;
  }
  return true;
}
