#include "stdafx.h"

#ifndef DICE_H
    #define DICE_H

using namespace std;

// dice stuff

// Interface for al dice types
class IDice {
public:
    // Returns number obtained by rolling this die
    virtual int operator() () =0;
};

// Fair die that returns each with equal likelihood
class FairDice : public IDice {
public:
    // Create a fair die with 'side' sides
    FairDice(int sides) { numSides = sides; }

    // Number of sides on this die
    int getNumSides() { return numSides; }
    
    int operator() () { return rand() % numSides + 1; }

private:
    int numSides;
};

// A die that deterministically cycles through a list of possible options
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

// Default die used in Hog; usually a six-sided fair die (defined in hog.cpp)
extern FairDice DEFAULT_DICE;
#endif