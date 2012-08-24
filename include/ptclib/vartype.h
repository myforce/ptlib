/*
 * vartype.h
 *
 * Interface library for Variable Type class wrapper
 *
 * Portable Tools Library
 *
 * Copyright (C) 2012 by Vox Lucida Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): Robert Jongbloed
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_VARTYPE_H
#define PTLIB_VARTYPE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <ptclib/guid.h>


//////////////////////////////////////////////////////////////

/**A wrapper around a Variable Type class.
   This allows run time variable type objects to be passed to various
   sub-systems.
 */
class PVarType : public PObject
{
    PCLASSINFO(PVarType, PObject)
  public:
    /// Type of the parameter in Paramater structure
    enum BasicType {
      VarNULL,
      VarBoolean,
      VarChar,
      VarInt8,
      VarInt16,
      VarInt32,
      VarInt64,
      VarUInt8,
      VarUInt16,
      VarUInt32,
      VarUInt64,
      VarFloatSingle,
      VarFloatDouble,
      VarFloatExtended, // aka "long double"
      VarGUID,
      VarTime,
      VarStaticString,
      VarFixedString,
      VarDynamicString,
      VarStaticBinary,
      VarDynamicBinary
    };

  /**@name Construction */
  //@{
    /**Create a NULL type object.
     */
    PVarType() : m_type(VarNULL) { }

    /**Create a boolean type object.
     */
    PVarType(bool value) : m_type(VarBoolean) { m_.boolean = value; }

    /**Create a 8 bit integer type object.
     */
    PVarType(char value) : m_type(VarChar) { m_.character = value; }

    /**Create a 16 bit integer type object.
     */
    PVarType(int16_t value) : m_type(VarInt16) { m_.int16 = value; }

    /**Create a 32 bit integer type object.
     */
    PVarType(int32_t value) : m_type(VarInt32) { m_.int32 = value; }

    /**Create a 64 bit integertype object.
     */
    PVarType(int64_t value) : m_type(VarInt64) { m_.int64 = value; }

    /**Create a 8 bit unsigned integer type object.
     */
    PVarType(uint8_t value) : m_type(VarUInt8) { m_.uint8 = value; }

    /**Create a 16 bit unsigned integer type object.
     */
    PVarType(uint16_t value) : m_type(VarUInt16) { m_.uint16 = value; }

    /**Create a 32 bit unsigned integer type object.
     */
    PVarType(uint32_t value) : m_type(VarUInt32) { m_.uint32 = value; }

    /**Create a 64 bit unsigned integer type object.
     */
    PVarType(uint64_t value) : m_type(VarUInt64) { m_.uint64 = value; }

    /**Create a single precision floating point type object.
     */
    PVarType(float value) : m_type(VarFloatSingle) { m_.floatSingle = value; }

    /**Create a double precision floating point type object.
     */
    PVarType(double value) : m_type(VarFloatDouble) { m_.floatDouble = value; }

    /**Create a extended (long double) precision floating point type object.
     */
    PVarType(long double value) : m_type(VarFloatExtended) { m_.floatExtended = value; }

    /**Create a time of day type object.
     */
    PVarType(const PGloballyUniqueID & value) : m_type(VarGUID) { memcpy(m_.guid, value, value.GetSize()); }

    /**Create a time of day type object.
     */
    PVarType(const PTime & value) : m_type(VarTime) { m_.time.seconds = value.GetTimeInSeconds(); }

    /**Create a string type object.
     */
    PVarType(const char * value, bool dynamic = false) : m_type(VarNULL) { SetString(value, dynamic); }

    /**Create a string type object.
     */
    PVarType(const PString & value, bool dynamic = true) : m_type(VarNULL) { SetString(value, dynamic); }

    /**Create a data (memory block) type object.
     */
    PVarType(const void * value, PINDEX len, bool dynamic = false) : m_type(VarNULL) { SetBinary(value, len, dynamic); }

    /**Create a data (memory block) type object.
     */
    PVarType(const PBYTEArray & value, bool dynamic = true) : m_type(VarNULL) { SetBinary(value, dynamic); }

    /**Copy constructor.
       Not for "dynamic" types this is not very efficient as it will make a
       duplicate of the memory used.
      */
    PVarType(const PVarType & other) : PObject(other), m_type(VarNULL) { InternalCopy(other); }

    /**Assign a boolean type object.
     */
    PVarType & operator=(bool value) { if (SetType(VarBoolean)) m_.boolean = value; return *this; }

    /**Assign a 8 bit integer type object.
     */
    PVarType & operator=(char value) { if (SetType(VarChar)) m_.character = value; return *this; }

    /**Assign a 16 bit integer type object.
     */
    PVarType & operator=(int16_t value) { if (SetType(VarInt16)) m_.int16 = value; return *this; }

    /**Assign a 32 bit integer type object.
     */
    PVarType & operator=(int32_t value) { if (SetType(VarInt32)) m_.int32 = value; return *this; }

    /**Assign a 64 bit integertype object.
     */
    PVarType & operator=(int64_t value) { if (SetType(VarInt64)) m_.int64 = value; return *this; }

    /**Assign a 8 bit unsigned integer type object.
     */
    PVarType & operator=(uint8_t value) { if (SetType(VarUInt8)) m_.uint8 = value; return *this; }

    /**Assign a 16 bit unsigned integer type object.
     */
    PVarType & operator=(uint16_t value) { if (SetType(VarUInt16)) m_.uint16 = value; return *this; }

    /**Assign a 32 bit unsigned integer type object.
     */
    PVarType & operator=(uint32_t value) { if (SetType(VarUInt32)) m_.uint32 = value; return *this; }

    /**Assign a 64 bit unsigned integer type object.
     */
    PVarType & operator=(uint64_t value) { if (SetType(VarUInt64)) m_.uint64 = value; return *this; }

    /**Assign a single precision floating point type object.
     */
    PVarType & operator=(float value) { if (SetType(VarFloatSingle)) m_.floatSingle = value; return *this; }

    /**Assign a double precision floating point type object.
     */
    PVarType & operator=(double value) { if (SetType(VarFloatDouble)) m_.floatDouble = value; return *this; }

    /**Assign a extended (long double) precision floating point type object.
     */
    PVarType & operator=(long double value) { if (SetType(VarFloatExtended)) m_.floatExtended = value; return *this; }

    /**Assign a time of day type object.
     */
    PVarType & operator=(const PGloballyUniqueID & value) { if (SetType(VarGUID)) memcpy(m_.guid, value, value.GetSize()); return *this; }

    /**Assign a time of day type object.
     */
    PVarType & operator=(const PTime & value) { if (SetType(VarTime)) m_.time.seconds = value.GetTimeInSeconds(); return *this; }

    /**Assignment operator for strings.
       Default to dynamic, not static as is this is much safer.
      */
    PVarType & operator=(const char * str) { return SetDynamicString(str); }

    /**Assignment operator for strings.
       Default to dynamic, not static as is this is much safer.
      */
    PVarType & operator=(const PString & str) { return SetDynamicString(str); }

    /**Assignment operator.
       Not for "dynamic" types this is not very efficient as it will make a
       duplicate of the memory used.
      */
    PVarType & operator=(const PVarType & other) { InternalCopy(other); return *this; }

    /// Destroy the variable type object.
    ~PVarType() { InternalDestroy(); }
  //@}

  /**@name Overrides from PObject */
  //@{
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);
    virtual PObject * Clone() const;
  //@}

  /**@name member variable access */
  //@{
    /// Get the basic type of this instance.
    BasicType GetType() const { return m_type; }

    /**Set the basic type.
       The \p option argument means different things depending on the type. If
       VarDynamicString or VarDynamicBinary then this indicates the size of
       the storage to allocate. If VarTime, then thius is the format to use
       for AsString().
      */
    virtual bool SetType(BasicType type, PINDEX option = 0);

    bool AsBoolean() const;
    int AsInteger() const;
    unsigned AsUnsigned() const;
    int64_t AsInteger64() const;
    uint64_t AsUnsigned64() const;
    double AsFloat() const;
    PGloballyUniqueID AsGUID() const;
    PTime AsTime() const;
    PString AsString() const;

    const void * GetPointer() const;
    PINDEX GetSize() const;

    /// Set the instances value without changing it's type.
    virtual PVarType & SetValue(const PString & value);

    virtual PVarType & SetString(const char * value, bool dynamic);
    PVarType & SetStaticString(const char * value) { return SetString(value, false); }
    PVarType & SetDynamicString(const char * value) { return SetString(value, true); }

    virtual PVarType & SetBinary(const void * data, PINDEX len, bool dynamic);
    PVarType & SetBinary(const PBYTEArray & value, bool dynamic) { return SetBinary(value, value.GetSize(), dynamic); }
    PVarType & SetStaticBinary(const void * data, PINDEX len) { return SetBinary(data, len, false); }
    PVarType & SetStaticBinary(const PBYTEArray & value) { return SetBinary(value, false); }
    PVarType & SetDynamicBinary(const void * data, PINDEX len) { return SetBinary(data, len, true); }
    PVarType & SetDynamicBinary(const PBYTEArray & value) { return SetBinary(value, true); }
  //@}

  protected:
    /// Called before all the AsXXX() functions execute.
    virtual void OnGetValue();

    /// Called after SetValue() has (possibly) changed the value
    virtual void OnValueChanged();

    // Internal functions
    virtual void InternalCopy(const PVarType & other);
    void InternalDestroy();

    BasicType m_type; ///< Type of parameter

    union Variant {
      Variant() { memset(this, 0, sizeof(this)); }
      bool        boolean;
      char        character;
      int8_t      int8;
      int16_t     int16;
      int32_t     int32;
      int64_t     int64;
      uint8_t     uint8;
      uint16_t    uint16;
      uint32_t    uint32;
      uint64_t    uint64;
      float       floatSingle;
      double      floatDouble;
      long double floatExtended;
      uint8_t     guid[PGloballyUniqueID::Size];

      struct {
        time_t            seconds;
        PTime::TimeFormat format;
      } time;

      const char * staticString;

      struct Dynamic {
        char * Alloc(size_t sz);
        char * Realloc(size_t sz);
        void Copy(const Dynamic & other);
        char * data;
        size_t size;
      } dynamic;

      struct {
        const char * data;
        size_t       size;
      } staticBinary;
    } m_;
};


#endif  // PTLIB_VARTYPE_H

