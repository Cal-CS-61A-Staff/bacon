#include "stdafx.h"
#include "strategy.h"
#include "hog.h"

// ** Implementation of basic strategies (LearningStrategy is implemented in analysis.cpp) **

int RandomStrategy::operator()(int score0, int score1) { 
    return rand() % (MAX_ROLLS+1); 
}

// AnnouncerStrategy

AnnouncerStrategy::AnnouncerStrategy(IStrategy & strat, int player_order) : inner_strategy(strat) {
    if (player_order > 1 || player_order < 0) player_order = 0;

    inner_order = player_order;
}

int AnnouncerStrategy::operator()(int score0, int score1) {
    if (inner_order == 0)
        std::cout << "Current score: " << score0 << "-" << score1 << std::endl << std::endl;
    else
        std::cout << "Current score: " << score1 << "-" << score0 << std::endl << std::endl;

    int inner_result = inner_strategy(score0, score1);
    std::cout << "Player " << inner_order << " Rolled: " << inner_result << std::endl;

    return inner_result;
}

// HumanStrategy

int HumanStrategy::operator() (int _, int __) {
    int roll = 0;

    do {
        if (roll != 0) std::cout << "Please enter a number between 0-10: ";
        else std::cout << "Please enter the number of die you wish to roll, human (0-10): ";

        std::cin >> roll;
    } while (roll < 0 || roll > MAX_ROLLS);

    return roll;
}

// MatrixStrategy

MatrixStrategy::MatrixStrategy(IStrategy & strat) {
    for (int i = 0; i < GOAL; ++i) {
        for (int j = 0; j < GOAL; ++j) {
            rolls[i][j] = strat(i, j);
        }
    }
}

MatrixStrategy::MatrixStrategy(int ** mat) {
    for (int i = 0; i < GOAL; ++i) {
        for (int j = 0; j < GOAL; ++j) {
            rolls[i][j] = mat[i][j];
        }
    }
}

void MatrixStrategy::set_roll_num(int score0, int score1, int roll) {
    rolls[score0][score1] = roll;
}

int MatrixStrategy::get_roll_num(int score0, int score1) {
    return rolls[score0][score1];
}

bool MatrixStrategy::load_from_file(std::string path) {
    std::ifstream ifs(path);
    if (!ifs) return false;

    for (int i = 0; i < GOAL; ++i) {
        for (int j = 0; j < GOAL; ++j) {
            int tmp; ifs >> tmp;
            set_roll_num(i, j, tmp);
        }
    }

    ifs.close();
    return true;
}

void MatrixStrategy::write_to_file(std::string path, bool pyformat) {
    std::ofstream ofs(path);

    if (pyformat) ofs << "def strategy(score0, score1):\n  return [[";

    for (int i = 0; i < GOAL; ++i) {
        if (pyformat && i) ofs << "],[";
        for (int j = 0; j < GOAL; ++j) {
            if (pyformat && j) ofs << ",";
            ofs << get_roll_num(i, j);
            if (!pyformat) ofs << " ";
        }
        if (!pyformat) ofs << "\n";
    }

    if (pyformat) ofs << "]][score0][score1]";

    ofs.flush();
    ofs.close();
}

// SwapStrategy

SwapStrategy::SwapStrategy(int margin, int num_rolls) {
    this->margin = margin;
    this->num_rolls = num_rolls;
}

int SwapStrategy::operator() (int score, int score1) {
    int new_score = score + free_bacon(score1);

    if (is_swap(new_score, score1)) {
        if (score1 - score > 0) {
            return 0;
        }
        else {
            return num_rolls;
        }
    }
    else if (new_score - score >= margin) {
        return 0;
    }
    else {
        return num_rolls;
    }
}
