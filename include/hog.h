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

    // Roll 'num_rolls' of 'dice' and return the sum of rolls
    int roll_dice(int num_rolls, IDice& dice = DEFAULT_DICE);
    
    // Returns the amount of points earned if the free bacon rule is applied at a score
    int free_bacon(int score);
    
    /* Adds the points obtained by rolling 'num_rolls' of 'dice' to the player's score and sees if any rules apply. 
       Returns the player's score after this turn*/
    int take_turn(int num_rolls, int score1, IDice& dice = DEFAULT_DICE);
	
    // Returns true if reaching 'score0', 'score1' on a turn would result in a swap
    int is_swap(int score0, int score1);
    
    // Returns true if the time trot rule applies when a player rolls 'rolls' on turn 'turn_num'
    int is_time_trot(int turn_num, int rolls);

    /*Simulate a game between two strategies,
      starting at 'score0' and 'score1' at turn 'starting_turn' with goal score 'goal'.*/
    pair<int, int> play(IStrategy & strategy0, IStrategy & strategy1, int score0 = 0, int score1 = 0,
                        IDice& dice = DEFAULT_DICE, int goal = GOAL, int starting_turn = 0);

#endif

