#pragma once

#include "stdafx.h"
#include "params.h"

#ifndef STRATEGY_H
    #define STRATEGY_H

    using namespace std;

    class IStrategy {
    public:
        virtual int operator()(int score0, int score1) =0;
    };

    // A strategy that always rolls the same number of die
    class AlwaysRollStrategy : public IStrategy {
    public:

        // A strategy that always rolls num dice
        AlwaysRollStrategy(int num) { num_to_roll = num; }

        int operator()(int score0, int score1) { return num_to_roll; }

    private:
        int num_to_roll;
    };

    // A strategy that rolls a random number of dice between 0-10
    class RandomStrategy : public IStrategy {
    public:
        int operator()(int score0, int score1);
    };

    /* A strategy that behaves just the same as the existing strategy but prints out
      extra information (i.e. roll number, score) each time it is called*/
    class AnnouncerStrategy : public IStrategy {
    public:

        /* Creates a new strategy that behaves just the same as the existing strategy but
           prints out extra information (i.e. roll number, score) each time it is called */
        AnnouncerStrategy(IStrategy & strat, int player_order);

        int operator()(int score0, int score1);

    private:
        IStrategy & inner_strategy;
        int inner_order;
    };

    class HumanStrategy : public IStrategy {
    public:
        int operator()(int _, int __);
    };

    class MatrixStrategy : public IStrategy {
    public:
        // Create an empty MatrixStrategy that rolls 0 for all scores
        MatrixStrategy() { memset(rolls, 0, sizeof rolls); }

        // Create a new MatrixStrategy that caches the rolls of another strategy at each score
        MatrixStrategy(IStrategy & strat);

        // Create a new MatrixStrategy from a jagged array
        MatrixStrategy(int ** mat);

        // Set the number the strategy rolls at a set of scores
        void set_roll_num(int score0, int score1, int roll); 

        // Get the number the strategy rolls at a set of scores
        int get_roll_num(int score0, int score1); 

        // Load the strategy matrix from a file
        bool load_from_file(string path);

        // Write the strategy matrix to a file
        void write_to_file(string path, bool pyformat = false);

        int operator() (int score0, int score1) {
            return rolls[score0][score1];
        }

    private:
        int rolls[GOAL][GOAL];
    };

    /* The swap_strategy from the Hog assignment. Swaps where beneficial, 
       also rolls 0 when gives points > margin. Else rolls 4.*/
    class SwapStrategy : public IStrategy {
    public:
        // Create a new SwapStrategy
        SwapStrategy(int margin = 8, int num_rolls = 4);

        int operator() (int score, int score1);

    private:
        int num_rolls, margin;
    };

#endif