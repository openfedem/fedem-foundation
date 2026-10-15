// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaMsg.H
  \brief Interface for various user feedback.
*/

#ifndef FFA_MSG_H
#define FFA_MSG_H

#include <string>
#include <vector>

// The symbol ERROR might have already been defined if, e.g., windows.h
// is included before this header file. To avoid compiler problems due to name
// conflict with the FFaDialogType enum value below, it must be undefined here.
// TODO: Rename FFaMsg::ERROR to something else?
#ifdef ERROR
#undef ERROR
#endif

//! \brief Output stream for feedback messages to the user.
//! \details This symbol works like an output stream.
//! Use this instead of std::cout and std::cerr when the message is meaningful
//! to the user too (and not only for the developers). A set of output stream
//! operators are defined for various variable types (not documented) which
//! format the value to a std::string which then is passed to FFaMsg::listVt().
#define ListUI FFaMsg::getMessager()

class FaVec3;


/*!
  \brief Interface class for various user feedback.
*/

class FFaMsg
{
public:
  //! \brief Default constructor.
  FFaMsg() {}
  //! \brief Empty destructor.
  virtual ~FFaMsg() {}

  //! \brief Sets the real messager object to be used.
  static void setMessager(FFaMsg* messager = NULL);
  //! \brief Returns the current messager object.
  static FFaMsg& getMessager();
  //! \brief Returns \e true if we are using the default messager.
  static bool usingDefault();

  //! \brief Enums defining various dialog box types.
  enum FFaDialogType
  {
    DISMISS_WARNING, //!< Non-modal warning dialog
    DISMISS_ERROR,   //!< Non-modal error dialog
    DISMISS_INFO,    //!< Non-modal information dialog
    WARNING,         //!< Modal warning dialog
    ERROR,           //!< Modal error dialog
    OK,              //!< Modal info dialog
    OK_CANCEL,       //!< Modal info dialog with cancel option
    YES_NO,          //!< Modal info dialog with yes/no options
    YES_NO_CANCEL,   //!< Modal info dialog with yes/no and cancel options
    _ALL_,           //!< Separator for the "to all" dialog types
    OK_ALL_CANCEL,   //!< Ok to all similar queries, or cancel the operation
    YES_ALL_NO,      //!< Yes to all similar queries, or no for this only
    YES_ALL_NO_ALL,  //!< Yes or no to all similar queries
    YES_ALL_NO_CANCEL, //!< Yes to all similar queries, or no for this or cancel
    YES_ALL_NO_ALL_CANCEL, //!< Yes or no to all similar queries, or cancel
    YES_NO_ALL,      //!< Yes to this or no to all similar queries
    YES_NO_ALL_CANCEL, //!< Yes to this or no to all similar queries, or cancel
    GENERIC          //!< Generic modal dialog with user-provided button texts
  };

  //! \brief Clears the multiple query answer given dialog type or all.
  static void resetToAllAnswer(const FFaDialogType type = _ALL_);

  //////////////////////////////////////////////////////////////////////////////
  // User feedback methods.
  // These will be routed to the messager provided, or to the default one.

  //! \brief Shows a dialog according to the dialog type.
  static int dialog(const std::string& message,
		    const FFaDialogType type = DISMISS_WARNING,
		    const char** genericButtons = (const char**)NULL);

  //! \brief Overloaded method with additional selection from a list.
  static int dialog(int& returnedSelectionIdx, const std::string& message,
		    const FFaDialogType type,
		    const std::vector<std::string>& buttonTexts,
		    const std::vector<std::string>& selectionList);

  //! \brief Prints a message to the Output List GUI (or console).
  static void list(const std::string& str, bool onScreen = false);
  //! \brief Updates the tip message in the GUI (or prints it to console).
  static void tip (const std::string& str);

  //! \brief Sets the status message, but only the the status stack is empty.
  static void setStatus   (const std::string& statusText);
  //! \brief Replaces the top message of the status stack.
  static void changeStatus(const std::string& statusText);
  //! \brief Pushes a new message to the status stack.
  static void pushStatus  (const std::string& statusText);
  //! \brief Pops the status stack such that the previous message appears.
  static void popStatus   ();

  //! \brief Enables substep counting in the GUI, in the range 1 to \a steps.
  static void enableSubSteps (int steps);
  //! \brief Assigns the substep counter.
  static void setSubStep     (int step);
  //! \brief Clears the substep counter.
  static void disableSubSteps();

  //! \brief Displays the elapsed time of a solver process in the GUI.
  static void displayTime(int hour, int min, int sec);
  //! \brief Clears the elapsed time display.
  static void clearTime  ();

  //! \brief Sets the subtask message in the GUI.
  static void setSubTask(const std::string& taskText);

  //! \brief Enables a progress bar in the GUI, in range 1 to \a nSteps.
  static void enableProgress (int nSteps);
  //! \brief Updates the progress counter.
  static void setProgress    (int progressStep);
  //! \brief Clears the progress counter.
  static void disableProgress();

  //! \cond STREAM_OPERATORS
  // Global stream-like operators for easy printing of messages to output list.
  // Typically used with the ListUI macro (see above).
  friend FFaMsg& operator<< (FFaMsg&, const std::string&);
  friend FFaMsg& operator<< (FFaMsg&, const char*);
  friend FFaMsg& operator<< (FFaMsg&, const FaVec3&);
  friend FFaMsg& operator<< (FFaMsg&, double);
  friend FFaMsg& operator<< (FFaMsg&, char);
  friend FFaMsg& operator<< (FFaMsg&, int);
  friend FFaMsg& operator<< (FFaMsg&, long long int);
  friend FFaMsg& operator<< (FFaMsg&, size_t);
  //! \endcond

  //! \brief Opens a list file containing a copy of the output list window.
  virtual bool openListFile(const char* = NULL, bool = false) { return true; }
  //! \brief Closes the list file.
  virtual void closeListFile() {}

protected:

  // Virtual methods reimplemented to make real GUI meaning

  //! \brief Shows a dialog according to the dialog type.
  virtual int dialogVt(const std::string& message,
		       const FFaDialogType type,
		       const char** genericButtons);
  //! \brief Shows a dialog according to the dialog type.
  virtual int dialogVt(const std::string& message,
		       const FFaDialogType type,
		       const std::vector<std::string>& buttonTexts);
  //! \brief Overloaded method with additional selection from a list.
  virtual int dialogVt(int& returnedSelectionIdx,
		       const std::string& message,
		       const FFaDialogType type,
		       const std::vector<std::string>& buttonTexts,
		       const std::vector<std::string>& selectionList);

  //! \brief Prints a message to the console.
  virtual void listVt(const std::string& str, bool onScreen = false);
  //! \brief Prints tip message to the console.
  virtual void tipVt (const std::string& str);

  //! \brief GUI interface for setting the status message.
  virtual void setStatusVt   (const std::string&) {}
  //! \brief GUI interface for changing the status message.
  virtual void changeStatusVt(const std::string&) {}
  //! \brief GUI interface for pushing a new status message.
  virtual void pushStatusVt  (const std::string&) {}
  //! \brief GUI interface for popping the status message stack.
  virtual void popStatusVt   () {}

  //! \brief GUI interface for enabling substep counting.
  virtual void enableSubStepsVt (int) {}
  //! \brief GUI interface for assigning the substep counter.
  virtual void setSubStepVt     (int) {}
  //! \brief GUI interface for clearing the substep counter.
  virtual void disableSubStepsVt() {}

  //! \brief GUI interface for displaying the elapsed time of a solver process.
  virtual void displayTimeVt(int, int, int) {}
  //! \brief GUI interface for clearing the elapsed time display.
  virtual void clearTimeVt  () {}

  //! \brief GUI interface for setting the subtask message.
  virtual void setSubTaskVt(const std::string&) {}

  //! \brief GUI interface for enabling the progress bar.
  virtual void enableProgressVt (int) {}
  //! \brief GUI interface for updating the progress counter.
  virtual void setProgressVt    (int) {}
  //! \brief GUI interface for clearing the progress counter.
  virtual void disableProgressVt() {}
};

#endif
