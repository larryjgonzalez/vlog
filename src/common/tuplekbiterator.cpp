/*
   Copyright (C) 2015 Jacopo Urbani.

   This file is part of Vlog.

   Vlog is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   Vlog is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Vlog.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <trident/sparql/sparqloperators.h>
#include <trident/kb/querier.h>
#include <trident/iterators/pairitr.h>

#include <vlog/tuplekbitr.h>
#include <vlog/concepts.h>

TupleKBItr::TupleKBItr() {}

void TupleKBItr::init(Querier *querier, const Tuple *t,
                      const std::vector<uint8_t> *fieldsToSort, bool onlyVars) {
    this->onlyVars = onlyVars;
    this->querier = querier;
    long s, p, o;
    if (t->get(0).isVariable()) {
        s = -1;
    } else {
        s = (long)t->get(0).getValue();
    }
    if (t->get(1).isVariable()) {
        p = -1;
    } else {
        p = (long)t->get(1).getValue();
    }
    if (t->get(2).isVariable()) {
        o = -1;
    } else {
        o = (long)t->get(2).getValue();
    }

    if (fieldsToSort == NULL) {
        idx = querier->getIndex(s, p, o);
    } else {
        //Assign variables names according to the sorting order
        int varid = -2;
        for (std::vector<uint8_t>::const_iterator itr = fieldsToSort->begin();
                itr != fieldsToSort->end(); ++itr) {
            switch (*itr) {
            case 0:
                s = varid--;
                break;
            case 1:
                p = varid--;
                break;
            case 2:
                o = varid--;
                break;
            }
        }
        idx = querier->getIndex(s, p, o);

        if (s < 0)
            s = -1;
        if (p < 0)
            p = -1;
        if (o < 0)
            o = -1;
    }
    invPerm = querier->getInvOrder(idx);

    if (onlyVars) {
        int idx = 0;
        for (uint8_t j = 0; j < 3; ++j) {
            if (!t->get(j).isVariable()) {
                idx++;
            } else {
                varsPos[j - idx] = (uint8_t) invPerm[j];
            }
        }
        sizeTuple = (uint8_t) (3 - idx);
    } else {
        sizeTuple = 3;
    }

    nextProcessed = false;
    nextOutcome = false;
    processedValues = 0;

    //If some variables have the same name, then we must change it
    equalFields = t->getRepeatedVars();
    physIterator = querier->get(idx, s, p, o);
}

bool TupleKBItr::checkFields() {
    if (equalFields.size() > 0) {
        for (std::vector<std::pair<uint8_t, uint8_t>>::const_iterator itr =
                    equalFields.begin();
                itr != equalFields.end(); ++itr) {
            if (getElementAt(itr->first) != getElementAt(itr->second)) {
                return false;
            }
        }
    }
    return true;
}

bool TupleKBItr::hasNext() {
    if (!nextProcessed) {
        if (physIterator->has_next()) {
            physIterator->next_pair();
            nextOutcome = true;
            if (equalFields.size() > 0) {
                while (!checkFields()) {
                    if (!physIterator->has_next()) {
                        nextOutcome = false;
                        break;
                    } else {
                        physIterator->next_pair();
                    }
                }
            }
        } else {
            nextOutcome = false;
        }
        nextProcessed = true;
    }
    return nextOutcome;
}

void TupleKBItr::next() {
    if (nextProcessed) {
        nextProcessed = false;
    } else {
        physIterator->next_pair();
    }
}

size_t TupleKBItr::getTupleSize() {
    return sizeTuple;
}

uint64_t TupleKBItr::getElementAt(const int p) {
    const uint8_t pos = onlyVars ? varsPos[p] : (uint8_t) invPerm[p];

    switch (pos) {
    case 0:
        return physIterator->getKey();
    case 1:
        return physIterator->getValue1();
    case 2:
        return physIterator->getValue2();
    }
    BOOST_LOG_TRIVIAL(error) << "This should not happen";
    throw 10;
}

TupleKBItr::~TupleKBItr() {
    if (querier != NULL)
        querier->releaseItr(physIterator);
}

void TupleKBItr::clear() {
    querier->releaseItr(physIterator);
    physIterator = NULL;
    querier = NULL;
}
