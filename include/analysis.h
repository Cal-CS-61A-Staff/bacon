#pragma once

#ifndef ANALYSIS_H
    #define ANALYSIS_H 

#include "strategy.h"    
#include "hog.h"    

    // The default number of samples for sampling the win rate (not used for absolute win rate calculation)
    const int DEFAULT_WR_SAMPLES = 10000;

    // ** Learning **

    /* A strategy that improves by using simple machine learning.
       May be trained against other strategies using the learn() function. Guarenteed to get better
       win rates against the opponent strategy until it reaches the optimal win rate.
       (If the win rate does not improve in 10000 rounds, then it must have reached optimality.)
        
       May be associated with a file path to enable auto-saving during training. */
    class LearningStrategy : public MatrixStrategy {
    public:
        /* Creates a LearningStrategy from an existing strategy. 
           If overwrite is true, immediately writes the strategy to the specified path.*/
        LearningStrategy(IStrategy & strat, std::string path = "", bool overwrite=true) : MatrixStrategy(strat) {
            file_path = path;
            if (overwrite && path != "") write_to_file(path);
        }

        /* Creates a LearningStrategy from a matrix. 
           If overwrite is true, immediately writes the strategy to the specified path.*/
        LearningStrategy(int ** mat, std::string path = "", bool overwrite=true) : MatrixStrategy(mat) {
            file_path = path;
            if (overwrite && path != "") write_to_file(path);
        }

        // Creates a LearningStrategy by loading a strategy from a path
        LearningStrategy(std::string path = "") : MatrixStrategy() {
            file_path = path;
            load_from_file(path);
        }

        /* Begin training against the specified opponent strategy, for the specified number of rounds */
        void learn(IStrategy & oppo_strat, int number = GOAL * GOAL,
            std::pair<int, int>focus = std::pair<int, int>(GOAL-1, GOAL-1), volatile int * interrupt = NULL,
            bool quiet = false, int announce_interval = 10, int wr_interval = 100);

        /* Takes a pair representing the current "focus" point of training and returns the next "focus" point
           in the sequence. Will automatically go over all focus points */
        static std::pair<int, int> next_focus(std::pair<int, int> focus);

    private:
        // used for auto-saving during training
        std::string file_path;

    }; // class LearningStrategy

    // ** Strategic analysis **

    // particularly horrendous strategy, used for visualizing where swaps occur w/ draw_strategy_diagram
    class SwapVisualiser : public IStrategy{
        int operator() (int score0, int score1) {
            return 1 - is_swap(score0, score1);
        }
    };

    // Compute the absolute theoretical win rate of a strategy against another
    double average_win_rate(IStrategy & strategy0, IStrategy & strategy1 = DEFAULT_STRATEGY,
                            int strategy0_plays_as = -1, int score0 = 0, int score1 = 0,
                            int starting_turn = 0, int thread_id = 0);

    // Compute the win rate of a strategy against another using sampling
    double average_win_rate_by_sampling(IStrategy & strategy0, IStrategy & strategy1 = DEFAULT_STRATEGY,
                            int strategy0_plays_as = -1, int score0 = 0, int score1 = 0,
                            int starting_turn = 0, int samples = DEFAULT_WR_SAMPLES);

    /* Run a round-robin tournament. Returns a vector of pairs where
       the first element of each item is the number of wins,
       and the second is the name of the player */
    std::vector<std::pair<int, std::string> *> round_robin(std::vector<std::pair<std::string, IStrategy *>> strats,
        void announcer(int games_played, int games_remaining, int high, std::string high_strat) = NULL,
        int announcer_interval = 100,
        double margin = 0.500001, int threads = 4, volatile int * interrupt = NULL);

    // Compute the "final" strategy using DP
    MatrixStrategy * create_final_strat(bool quiet=false);

    // Draw the diagram for a specific strategy
    void draw_strategy_diagram(IStrategy & strat);

#endif
