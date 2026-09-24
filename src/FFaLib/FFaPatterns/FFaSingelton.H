// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaSingelton.H
  \brief Template for singleton classes.
*/

#ifndef FFA_SINGELTON_H
#define FFA_SINGELTON_H

/*!
  \brief Template for singleton classes.
  \details This class is used as a wrapper for classes for which there should
  only exist one single instance (or none). All referencing to the class
  members are then to be done via the instance() method. The instance is
  heap-allocated on the first call to instance(), and can be explicitly
  deleted by the removeInstance() method.
*/

template<class T, class Container = T> class FFaSingelton
{
protected:
  //! \brief The constructor is protected to allow objects of sub-classes only.
  FFaSingelton() {}
  //! \brief Disable default copy constructor.
  FFaSingelton(const FFaSingelton&) = delete;
  //! \brief Disable default assignment operator.
  FFaSingelton& operator=(const FFaSingelton&) = delete;

public:
  //! \brief Returns the actual instance of this class.
  //! \details The instance is allocated dynamically in the first invocation.
  static T* instance()
  {
    if (!ourInstance)
      ourInstance = new T();
    return ourInstance;
  }

  //! \brief Deletes the dynamically allocated instance.
  //! \details This method is used to clean up memory before terminating.
  static void removeInstance()
  {
    delete ourInstance;
    ourInstance = 0;
  }

  //! \brief Returns \e true, if the instance has been allocated.
  static bool allocated() { return ourInstance != 0; }

private:
  static T* ourInstance; //!< Points to the dynamically allocated instance
};


template<class T, class Container>
T* FFaSingelton<T,Container>::ourInstance = 0;

#endif
