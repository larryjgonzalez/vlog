/*
   Copyright (C) 2015 Jacopo Urbani.

   This file is part of Trident.

   Trident is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   Trident is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Trident.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _TRIPLEWRITERS_H
#define _TRIPLEWRITERS_H

#include "lz4io.h"

#include <vector>
#include <string>

class SimpleTripleWriter: public TripleWriter {
private:
    LZ4Writer *writer;
public:
    SimpleTripleWriter(string dir, string prefixFile, bool quad);

    void write(const long t1, const long t2, const long t3);

    void write(const long t1, const long t2, const long t3, const long count);

    ~SimpleTripleWriter() {
        delete writer;
    }
};

class SortedTripleWriter: public TripleWriter {
private:
    const int fileSize;
    vector<Triple> buffer;
    int buffersCurrentSize;
    int idLastWrittenFile;
    string dir;
    string prefixFile;

public:
    SortedTripleWriter(string dir, string prefixFile, int fileSize);

    void write(const long t1, const long t2, const long t3);

    void write(const long t1, const long t2, const long t3, const long count);

    ~SortedTripleWriter();
};

#endif
