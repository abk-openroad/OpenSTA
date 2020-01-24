// OpenSTA, Static Timing Analyzer
// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef STA_ERROR_H
#define STA_ERROR_H

#include <exception>
#include "DisallowCopyAssign.hh"
#include "Machine.hh"

namespace sta {

// Abstract base class for sta exceptions.
class StaException : public std::exception
{
public:
  StaException();
  virtual ~StaException() THROW_DCL {}
  virtual const char *what() const throw() = 0;
};

class StaExceptionLine : public StaException
{
public:
  StaExceptionLine(const char *filename,
		   int line);

protected:
  const char *filename_;
  int line_;
};

class InternalError : public StaExceptionLine
{
public:
  InternalError(const char *filename,
		int line,
		const char *msg);
  virtual const char *what() const throw();

protected:
  const char *msg_;
};

// Report an error condition that should not be possible.
// The default handler prints msg to stderr and exits.
// The msg should NOT include a period or return, as these
// are added by InternalError::asString().
#define internalError(msg) \
  throw sta::InternalError(__FILE__, __LINE__, msg)

#define internalErrorNoThrow(msg) \
  printf("Internal Error: %s:%d %s\n", __FILE__, __LINE__, msg)

// Failure opening filename for reading.
class FileNotReadable : public StaException
{
public:
  explicit FileNotReadable(const char *filename);
  virtual const char *what() const throw();

protected:
  const char *filename_;
};

// Failure opening filename for writing.
class FileNotWritable : public StaException
{
public:
  explicit FileNotWritable(const char *filename);
  virtual const char *what() const throw();

protected:
  const char *filename_;
};

} // namespace
#endif
