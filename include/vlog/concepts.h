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

#ifndef CONCEPTS_H
#define CONCEPTS_H


#include <vlog/support.h>
#include <trident/kb/dictmgmt.h>
#include <trident/kb/kb.h>
#include <trident/model/model.h>

#include <string>
#include <inttypes.h>
#include <stdlib.h>
#include <vector>
#include <unordered_map>

/*** PREDICATES ***/
#define EDB 0
#define IDB 1
#define MAX_NPREDS 32768
#define OUT_OF_PREDICATES   32768

typedef uint16_t PredId_t;

class Predicate {
private:
    const PredId_t id;
    const uint8_t type;
    const uint8_t adornment;
    const uint8_t card;

public:
    Predicate(const Predicate &pred) : id(pred.id), type(pred.type), adornment(pred.adornment), card(pred.card) {
    }

    Predicate(const Predicate p, const uint8_t adornment) : id(p.id),
        type(p.type), adornment(adornment), card(p.card) {
    }

    Predicate(const PredId_t id, const uint8_t adornment, const uint8_t type,
              const uint8_t card) : id(id), type(type), adornment(adornment),
        card(card) {
    }

    PredId_t getId() const {
        return id;
    }

    uint8_t getType() const {
        return type & 1;
    }

    bool isMagic() const {
        return (type >> 1) != 0;
    }

    uint8_t getAdorment() const {
        return adornment;
    }

    uint8_t getCardinality() const {
        return card;
    }

    static bool isEDB(std::string pred) {
        return pred.at(pred.size() - 1) == 'E';
    }

    static uint8_t calculateAdornment(Tuple &t) {
        uint8_t adornment = 0;
        for (size_t i = 0; i < t.getSize(); ++i) {
            if (!t.get(i).isVariable()) {
                adornment += 1 << i;
            }
        }
        return adornment;
    }

    static uint8_t changeVarToConstInAdornment(const uint8_t adornment, const uint8_t pos) {
        uint8_t shiftedValue = (uint8_t) (1 << pos);
        return adornment | shiftedValue;
    }

    static uint8_t getNFields(uint8_t adornment) {
        uint8_t n = 0;
        for (int i = 0; i < 8; ++i) {
            if (adornment & 1)
                n++;
            adornment >>= 1;
        }
        return n;
    }
};

/*** SUBSTITUTIONS ***/
struct Substitution {
    uint8_t origin;
    Term destination;
    Substitution() {}
    Substitution(uint8_t origin, Term destination) : origin(origin), destination(destination) {}
};

/*** LITERALS ***/
class Program;
class Literal {
private:
    const Predicate pred;
    const Tuple tuple;
public:
    Literal(const Predicate pred, const Tuple tuple) : pred(pred), tuple(tuple) {}

    Predicate getPredicate() const {
        return pred;
    }

    bool isMagic() const {
        return pred.isMagic();
    }

    Term getTermAtPos(const int pos) const {
        return tuple.get(pos);
    }

    size_t getTupleSize() const {
        return tuple.getSize();
    }

    Tuple getTuple() const {
        return tuple;
    }

    size_t getNBoundVariables() const {
        return pred.getNFields(pred.getAdorment());
    }

    static int mgu(Substitution *substitutions, const Literal &l, const Literal &m);

    static int subsumes(Substitution *substitutions, const Literal &from, const Literal &to);

    static int getSubstitutionsA2B(
        Substitution *substitutions, const Literal &a, const Literal &b);

    Literal substitutes(Substitution *substitions, const int nsubs) const;

    bool sameVarSequenceAs(const Literal &l) const;

    uint8_t getNVars() const;

    uint8_t getNUniqueVars() const;

    std::vector<uint8_t> getPosVars() const;

    std::vector<std::pair<uint8_t, uint8_t>> getRepeatedVars() const;

    std::vector<uint8_t> getSharedVars(const std::vector<uint8_t> &vars) const;

    std::vector<uint8_t> getNewVars(std::vector<uint8_t> &vars) const;

    std::vector<uint8_t> getAllVars() const;

    std::string tostring(Program *program, DictMgmt *dict) const;

    std::string tostring() const;

    /*Literal operator=(const Literal &other) {
        return Literal(other.pred,other.tuple);
    }*/

    bool operator ==(const Literal &other) const;
};

class Rule {
private:
    const Literal head;
    const std::vector<Literal> body;
    const bool _isRecursive;

    static bool checkRecursion(const Literal &head, const std::vector<Literal> &body);

public:
    bool doesVarAppearsInFollowingPatterns(int startingPattern, uint8_t value);

    Rule(const Literal head, std::vector<Literal> body) : head(head),
        body(body),
        _isRecursive(checkRecursion(head, body)) {}

    Rule createAdornment(uint8_t headAdornment);

    bool isRecursive() const {
        return this->_isRecursive;
    }

    const Literal &getHead() const {
        return head;
    }

    const std::vector<Literal> &getBody() const {
        return body;
    }

    uint8_t getNIDBPredicates() const {
        uint8_t i = 0;
        for (std::vector<Literal>::const_iterator itr = body.begin(); itr != body.end();
                ++itr) {
            if (itr->getPredicate().getType() == IDB) {
                i++;
            }
        }
        return i;
    }

    uint8_t getNIDBNotMagicPredicates() const {
        uint8_t i = 0;
        for (std::vector<Literal>::const_iterator itr = body.begin(); itr != body.end();
                ++itr) {
            if (itr->getPredicate().getType() == IDB && !itr->getPredicate().isMagic()) {
                i++;
            }
        }
        return i;
    }

    uint8_t getNEDBPredicates() const {
        uint8_t i = 0;
        for (std::vector<Literal>::const_iterator itr = body.begin(); itr != body.end();
                ++itr) {
            if (itr->getPredicate().getType() == EDB) {
                i++;
            }
        }
        return i;
    }

    std::string tostring(Program *program, DictMgmt *dict) const;

    std::string tostring() const;

    ~Rule() {
    }

    Rule operator=(const Rule &other) {
        return Rule(head, body);
    }
};

class Program {
private:
    KB *kb;
    std::vector<Rule> rules[MAX_NPREDS];
    Dictionary dictVariables;

    Dictionary dictPredicates;
    std::unordered_map<PredId_t, uint8_t> cardPredicates;

    Dictionary additionalConstants;

    void parseRule(std::string rule);

    std::string rewriteRDFOWLConstants(std::string input);

public:
    Program(KB *kb) : kb(kb), additionalConstants(kb->getNTerms()) {
    }

    Literal parseLiteral(std::string literal);

    void readFromFile(std::string pathFile);

    Term_t getIDVar(std::string var);

    PredId_t getPredicateID(std::string &p, const uint8_t card);

    std::string getPredicateName(const PredId_t id);

    Predicate getPredicate(std::string &p);

    Predicate getPredicate(std::string &p, uint8_t adornment);

    Predicate getPredicate(const PredId_t id);

    std::vector<Rule> *getAllRulesByPredicate(PredId_t predid);

    std::string getFromAdditional(Term_t val) {
	return additionalConstants.getRawValue(val);
    }

    void sortRulesByIDBPredicates();

    std::vector<Rule> getAllRules();

    Program clone() const;

    std::shared_ptr<Program> cloneNew() const;

    void cleanAllRules();

    void addAllRules(std::vector<Rule> &rules);

    bool isPredicateIDB(const PredId_t id);

    std::string getAllPredicates();

    std::string tostring();

    static std::string compressRDFOWLConstants(std::string input);

    ~Program() {
    }
};
#endif
