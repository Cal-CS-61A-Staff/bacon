#include "stdafx.h"

#ifndef DICE_H
    #define DICE_H

using namespace std;

// dice stuff
class IDice {
public:
    virtual int operator() () =0;
};

class FairDice : public IDice {
public:
    FairDice(int sides) { numSides = sides; }

    int getNumSides() { return numSides; }
    int operator() () { return rand() % numSides + 1; }

private:
    int numSides;
};

class TestDice : public IDice {
public:
    vector<int> choices;
    int index = 0;
        
    TestDice(int n_args, ...) {
        va_list ap;
        va_start(ap, n_args);

        choices.reserve(n_args);
        for (int i = 0; i < n_args; ++i) {
            choices.push_back(va_arg(ap, int));
        }

        va_end(ap);
    }

    int next() {
        int val = choices[index];
        index = (index + 1) % (choices.size());
        return val;
    }
};

extern FairDice DEFAULT_DICE;
#endif