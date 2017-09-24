#pragma once

#include "stdafx.h"
#include "dice.h"
#include "strategy.h"

#ifndef HOG_H
    #define HOG_H 

    // Options to enable/disable rules
    extern int enable_time_trot, enable_swine_swap;

    // Reference to the default strategy
	extern IStrategy & DEFAULT_STRATEGY;

    int roll_dice(int num_rolls, IDice& dice = DEFAULT_DICE);
    int free_bacon(int score);
    int take_turn(int num_rolls, int score1, IDice& dice = DEFAULT_DICE);
	
    int is_swap(int score0, int score1);
    int is_time_trot(int turn_num, int rolls);

    pair<int, int> play(IStrategy & strategy0, IStrategy & strategy1, int score0 = 0, int score1 = 0,
                        IDice& dice = DEFAULT_DICE, int goal = GOAL, int starting_turn = 0);

#endif

