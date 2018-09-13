#pragma once

#ifndef PARAMS_H
    #define PARAMS_H
    // ** Parameters for the Hog game **

    // Number of sides on a default die
    const int DICE_SIDES = 6;

    // Number of times to roll
    const int MAX_ROLLS = 10;

    // The goal score
    const int GOAL = 100;

    // Maximum theoretical number of turns
    const int MAX_TURNS = GOAL * 2;

    // Modulo for Time Trot
    const int MOD_TROT = 5; 

#endif // !PARAMS_H
