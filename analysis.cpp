#include "stdafx.h"
#include "analysis.h"

// hide from linkage
namespace {
    // trp[i]: total permutations of rolling i dice: 1, 6, 36, ...
    int total_roll_perms[MAX_ROLLS + 1];

    // trps[i][j]: # ways to get sum j by rolling i dice
    int total_roll_perms_for_sum[MAX_ROLLS + 1][DICE_SIDES * MAX_ROLLS + 1];

    // prs[i][j]: probability of getting sum j by rolling i dice; = trps[i][j] / trp[i]
    double prob_roll_sum[MAX_ROLLS + 1][DICE_SIDES * MAX_ROLLS + 1];

    // pars[i]: probability of getting sum i by rolling any number (1-10) of dice; = sum(prs[0][i] ... prs[10][i]) / 10.0
    double prob_any_roll_sum[DICE_SIDES * MAX_ROLLS + 1];

    // ptns[i][j][k]: probability of being on (turn_num % 8) = i at player scores (j,k); 
    //                used to approximate likelihood of getting a time trot turn
    double prob_turn_num_at_score[MOD_TROT][GOAL][GOAL];

    // wrs[i][j][t]: gives (x,y), where 
    // x is the first player's win rate at score (i,j), where it is the first player's turn. 
    // y is the best strategy to use
    // if t is 1, Time Trot is enabled at the current turn. else it is disabled.
    std::pair<double, int> win_rate_at_score[GOAL][GOAL][2];

    bool perms_computed = false, turn_num_computed = false;

    // helper function for adding & swapping scores
    inline void add_swap_scores(int & score0, int & score1, int add0 = 0) {
        score0 += add0;
        if (!enable_swine_swap) return;
        if (is_swap(score0, score1)) std::swap(score0, score1);
    }
}

// *** Implementation of LearningStrategy class ***

void LearningStrategy::learn(IStrategy & oppo_strat, int number,
    std::pair<int, int>focus, volatile int * interrupt, bool quiet, int announce_interval, int wr_interval) {
        MatrixStrategy testStrat = MatrixStrategy(*this);

        if (!quiet) {
            std::cout << "Learning procedure started.\n" << "Initial win rate: ";
            std::cout << average_win_rate(testStrat, oppo_strat, -1, 0, 0, 0) << "\n\n";
        }

        for (int i = 0; i < number; ++i) {
            if (interrupt && *interrupt) break;

            double best_awr = average_win_rate(testStrat, oppo_strat, -1, 0, 0, 0);

            int rn = get_roll_num(focus.first, focus.second);
            int best_rolls = rn;

            for (int j = 0; j <= MAX_ROLLS; ++j) {
                if (j == rn) continue;

                testStrat.set_roll_num(focus.first, focus.second, j);

                double awr = average_win_rate(testStrat, oppo_strat, -1, 0, 0, 0);

                if (awr > best_awr) {
                    best_awr = awr;
                    best_rolls = j;
                }
            }

            testStrat.set_roll_num(focus.first, focus.second, best_rolls);

            set_roll_num(focus.first, focus.second, best_rolls);

            focus = next_focus(focus); // advance to next focus point

            if (!quiet) {
                if (i % announce_interval == announce_interval - 1) {
                    std::cout << i + 1 << " round" << (i ? "s" : "") << " of learning completed, " << number - i - 1 << " remaining. \n";
                }

                if (i % wr_interval == wr_interval - 1) {
                    std::cout << "Updated Win rate @ " << i + 1 << ": " << best_awr << "\n\n";
                }
            }

            if (i % wr_interval == wr_interval - 1 && file_path != "") write_to_file(file_path);
        }

        if (!quiet) {
            if (file_path != "") write_to_file(file_path);

            if (interrupt && *interrupt) {
                std::cout << "\nLearning interrupted by user (results saved).\n";
            }
            else {
                std::cout << "\nLearning complete. Computing final win rate...\n";
                std::cout << "Final win rate: " << average_win_rate(testStrat, oppo_strat, -1, 0, 0, 0) << std::endl;
            }
        }
}

std::pair<int, int> LearningStrategy::next_focus(std::pair<int, int> focus) {
    // advance to the next value of focus. will cycle through all possible values of focus

    if (focus.first < focus.second) {
        std::swap(focus.first, focus.second);
    }

    else {
        if (focus.first > focus.second) std::swap(focus.first, focus.second);

        --focus.first;
        --focus.second;

        if (focus.first < 0) {
            // cycle back to 99-diff-1, 99
            int diff = focus.second - focus.first;
            focus.first = GOAL - diff - 2;
            focus.second = GOAL - 1;

            if (focus.first < 0) focus.first = GOAL - 1; // cycle back to 99, 99
        }
    }

    return focus;
}

/* Part of the final strategy implementation.
computes the number of ways to get each number by rolling each number of dice. complexity n^2 * m*/
void compute_perms() {
    memset(total_roll_perms_for_sum, 0, sizeof total_roll_perms_for_sum);
    memset(prob_any_roll_sum, 0, sizeof prob_any_roll_sum);
    memset(prob_roll_sum, 0, sizeof prob_roll_sum);

    total_roll_perms[0] = 1;
    total_roll_perms_for_sum[0][0] = 1; // base case

    for (int i = 1; i <= MAX_ROLLS; ++i) {

        total_roll_perms[i] = total_roll_perms[i - 1] * DICE_SIDES;

        // the lowest possible sum for the previous number of rolls
        int prev_low = 2 * (i - 1);

        // used to add up all the permutations so we can see if the sum matches the expected total.
        int check_sum = 0;

        // add # ways of getting a score of one due to Pig Out
        int num_ones = 0, last_pow = 1, last_choose = 1;

        for (int j = 1; j <= i; ++j) {
            num_ones += last_choose * last_pow;
            last_pow *= 5;
            last_choose = last_choose * (i - j + 1) / j;
        }

        total_roll_perms_for_sum[i][1] += num_ones;
        prob_roll_sum[i][1] += (double)num_ones / total_roll_perms[i];

        prob_any_roll_sum[1] += prob_roll_sum[i][1] / 8.0;

        check_sum += num_ones;

        // sum up permutations for getting all other scores

        // rolling sum to reduce complexity by m (DICE_SIDES)
        int rolling = 0;

        for (int j = DICE_SIDES * i - DICE_SIDES + 1; j < DICE_SIDES * i; ++j) {
            rolling += total_roll_perms_for_sum[i - 1][j];
        }

        for (int j = DICE_SIDES * i; j >= 2 * i; --j) {

            // update rolling sum
            if (j - 1 >= prev_low) rolling -= total_roll_perms_for_sum[i - 1][j - 1];
            if (j - DICE_SIDES >= prev_low) rolling += total_roll_perms_for_sum[i - 1][j - DICE_SIDES];

            total_roll_perms_for_sum[i][j] = rolling;
            check_sum += rolling;

            prob_roll_sum[i][j] = (double)rolling / total_roll_perms[i];
            prob_any_roll_sum[j] += prob_roll_sum[i][j] / 10.0;
        }

        // totals don't match, maybe we have a bug!
        if (check_sum != total_roll_perms[i])
            throw "Permutations mismatch in compute_sums";
    }

    perms_computed = true;
}

double prob_full_turn_num_at_score[MAX_TURNS + 1][GOAL][GOAL];

/*
Part of the final strategy implementation
Computes the probability of being on a certain time trot turn number at each score. O(n^4) */
void compute_prob_turn_num_at_score() {
    // keep track of total probability so we can divide by this to find real probability

    memset(prob_turn_num_at_score, 0, sizeof prob_turn_num_at_score);
    memset(prob_full_turn_num_at_score, 0, sizeof prob_full_turn_num_at_score);

    // base case

    prob_full_turn_num_at_score[0][0][0] = 100.0;

    for (int i = 0; i < MAX_TURNS; ++i) {

        for (int j = 0; j < GOAL; ++j) {
            for (int k = 0; k < GOAL; ++k) {

                if ((j < 2 || k < 2) && i > 3) prob_full_turn_num_at_score[i][j][k] = 0.0;
                else if (j + k < i) prob_full_turn_num_at_score[i][j][k] = 0.0;

                if (prob_full_turn_num_at_score[i][j][k] == 0.0) continue;

                // consider free bacon rule
                int s = free_bacon(j);
                int new_score = j + s, new_oppo_score = k;

                add_swap_scores(new_score, new_oppo_score);

                if (new_oppo_score < GOAL && new_score < GOAL) {
                    double delta = 0.1 * prob_full_turn_num_at_score[i][j][k];

                    prob_full_turn_num_at_score[i + 1][new_oppo_score][new_score] += delta;
                }

                for (int s = 1; s < std::min(GOAL - j, DICE_SIDES * MAX_ROLLS + 1); ++s) {
                    if (j + s >= GOAL) break;

                    new_score = j + s; new_oppo_score = k;

                    add_swap_scores(new_score, new_oppo_score);

                    if (new_oppo_score < GOAL && new_score < GOAL) {
                        double delta = prob_any_roll_sum[s] * prob_full_turn_num_at_score[i][j][k];

                        prob_full_turn_num_at_score[i + 1][new_oppo_score][new_score] += delta;
                    }
                }
            }
        }
    }

    for (int j = 0; j < GOAL; ++j) {
        for (int k = 0; k < GOAL; ++k) {
            double sum = 0.0;

            for (int i = 0; i <= MAX_TURNS; ++i) {
                double delta = prob_full_turn_num_at_score[i][j][k];
                prob_turn_num_at_score[i % MOD_TROT][j][k] += delta;
            }

            for (int i = 0; i < MOD_TROT; ++i) {
                sum += prob_turn_num_at_score[i][j][k];
            }

            for (int i = 0; i < MOD_TROT; ++i) {
                // avoid div by 0
                if (sum == 0.0)
                    prob_turn_num_at_score[i][j][k] = 1.0 / MOD_TROT;
                else
                    prob_turn_num_at_score[i][j][k] /= sum;
            }
        }
    }

    turn_num_computed = true;
}

/* Part of the final strategy implementation
   Note: this is NOT the procedure for calculating the exact win rate. Look for average_win_rate.
   
   Due to special properties of the scores (i.e. the same score cannot occur twice in a game)
   this function may be written recursively.

   Time complexity: O(N^2 * M^2) where N is the goal score, and M is the max # rolls. 
   Constant factors: DICE_SIDES (6), trot (2) */
std::pair<double, int> compute_win_rates(int i, int j, int trot) {

    // perform computations if wrs(i, j) has not yet been computed; else return memoized result
    if (win_rate_at_score[i][j][trot].second == -1) {

        int best_strat = 0;
        double best_wr = 0.0;

        for (int r = 0; r <= MAX_ROLLS; ++r) {
            int total_times_score_counted = 0;
            double wr = 0.0;

            for (int k = 1; k <= DICE_SIDES * r || r == 0; ++k) {
                if (r == 0) {
                    // for zero rolls, set k (the change in score)
                    // to the value acquired from using the free bacon fule
                    k = free_bacon(j);
                }

                int new_score = i + k, new_oppo_score = j;
                add_swap_scores(new_score, new_oppo_score);

                double delta;
                if (new_score >= GOAL) {
                    // immediate win, yay
                    delta = 1.0;
                }

                else if (new_oppo_score >= GOAL) {
                    // immediate loss due to swapping! we need to avoid this
                    delta = 0.0;
                }

                else {
                    // no one wins, add win rate at next round
                    if (trot) {
                        // use Time Trot

                        double trot_prob = 0.0;

                        if (r < MOD_TROT) trot_prob = prob_turn_num_at_score[r][i][j];

                        delta =
                            (1.0 - compute_win_rates(new_oppo_score, new_score, 1).first) * (1 - trot_prob) +
                            compute_win_rates(new_score, new_oppo_score, 0).first * trot_prob;
                    }
                    else {
                        // no Time Trot allowed
                        delta = 1.0 - compute_win_rates(new_oppo_score, new_score, 1).first;
                    }
                }

                if (r == 0) { // special stuff for Free Bacon (0 rolls)

                    // the win rate for free bacon = delta
                    wr = delta;

                    // set to 1 to ignore total_times_score_counted
                    total_times_score_counted = 1;

                    break; // no need to continue
                }

                else { // every other roll #

                    wr += delta * total_roll_perms_for_sum[r][k];

                    // add to total so we can divide by this later.
                    total_times_score_counted += total_roll_perms_for_sum[r][k];
                }

                if (k == 1) k = 2 * r - 1; // skip unnecessary computations
            }

            wr /= total_times_score_counted;

            if (wr > best_wr) {
                best_wr = wr;
                best_strat = r;
            }
        }

        win_rate_at_score[i][j][trot] = std::pair<double, int>(best_wr, best_strat);
    }

    return win_rate_at_score[i][j][trot];
}

// Compute the "final strategy" and return a MatrixStrategy containing the roll number for each score
MatrixStrategy * create_final_strat(bool quiet) {
    std::pair<double, int> default_val = std::pair<double, int>(-1.0, -1);
    std::fill(win_rate_at_score[0][0], win_rate_at_score[0][0] + GOAL * GOAL * 2, default_val);

    // precompute stuff
    if (!quiet) {
        std::cout << "\n";

        std::cout << "Preparing 0/2, please wait ..." << std::endl;
    }

    // precompute dice permulations used in the other functions

    if (!perms_computed) compute_perms();

    if (!quiet) std::cout << "Preparing 1/2, please wait ..." << std::endl;

    // precompute the probability of being on a specific turn number at each score; used for Time Trot rule

    if (!turn_num_computed) compute_prob_turn_num_at_score();

    if (!quiet)
        std::cout << "Preparing 2/2...\n\nComputing strategy..." << std::endl;

    // make a CachedStrategy in the heap to store our strategy matrix. We will return this at the end.
    MatrixStrategy * opt_strat = new MatrixStrategy();

    for (int i = 0; i < GOAL; ++i) {
        for (int j = 0; j < GOAL; ++j) {
            if (j % 4 == 0 || i % 4 == 0){
                opt_strat->set_roll_num(i, j, 4);
            }
            else{
                opt_strat->set_roll_num(i, j, compute_win_rates(i, j, 1).second);
            }
        }
    }

    if (!quiet)
        std::cout << "Strategy saved\n" << std::endl;

    // return the complete strategy
    return opt_strat;
}


// *** Win rate computations ***

// hide from linkage
namespace {
    // storage class for win rate computation DP	
    class WinRateStorage{
        
    public:	
        const static int SIZE = GOAL * GOAL * 2 * MOD_TROT * 2;
        
        // get the value stored for a specified state
        inline double get(int score, int oppo_score, int who, int turn, int trot){
            return val[index(score, oppo_score, who, turn, trot)];
        }

        // set the value stored for a specified state to 'value'
        inline void set(int score, int oppo_score, int who, int turn, int trot, double value){
            val[index(score, oppo_score, who, turn, trot)] = value;
        }

        // set all values to -1 in preparation for a new win rate calculation
        inline void clear(void) {
            std::fill(val, val + SIZE, -1.0);
        }

        // Default constructor, allocates space for state array
        WinRateStorage(void){
            val = new double [ SIZE ];
        }

        /* Copy constructor does NOT actually copy the old array.
           Definined like this to make vector.resize work. */
        WinRateStorage(const WinRateStorage &obj){
            val = new double [ SIZE ];
        }
        
        // Frees space taken by state array
        ~WinRateStorage(void){
            delete[] val;
        }

    private:

        double * val; 

        static inline int index(int score, int oppo_score, int who, int turn, int trot) {
            return score * GOAL * 2 * MOD_TROT * 2 + oppo_score * 2 * MOD_TROT * 2 +
                who * MOD_TROT * 2 + turn * 2 + trot;
        }

    };

    /* DP storage vector. A new WinRateStorage is allocated for each thread spawned to prevent access conflict.
       Initialized with a single WinRateStorage and resized as required. */
    std::vector<WinRateStorage> dp(1);

    // coroutine for average win rate calculator
    double average_win_rate_coroutine(IStrategy & strat, IStrategy & oppo_strat, int score, int oppo_scoe,
                int who, int turn, int trot, int t_id) {

        if (dp[t_id].get(score, oppo_scoe, who, turn, trot) == -1.0) {

            int r = strat(score, oppo_scoe);
            
            int total_times_score_counted = 0;
            double wr = 0.0;

            for (int k = 1; k <= DICE_SIDES * r || r == 0; ++k) {
                if (r == 0) {
                    // for zero rolls, set k (the change in score)
                    // to the value acquired from using the free bacon fule
                    k = free_bacon(oppo_scoe);
                }

                int new_score = score + k, new_oppo_score = oppo_scoe;
                add_swap_scores(new_score, new_oppo_score);

                double delta;
                if (new_score >= GOAL) {
                    // immediate win, yay
                    delta = 1.0;
                }

                else if (new_oppo_score >= GOAL) {
                    // immediate loss due to swapping! we need to avoid this
                    delta = 0.0;
                }

                else {
                    // no one wins, add win rate at next round

                    if (enable_time_trot && trot && turn == r) {
                        // apply Time Trot
                        delta =
                            average_win_rate_coroutine(strat, oppo_strat, 
                                new_score, new_oppo_score, who, (turn + 1) % MOD_TROT, 0, t_id);
                    }
                    else {
                        // no Time Trot, go to opponent's round
                        delta = 1.0 - average_win_rate_coroutine(oppo_strat, strat,
                            new_oppo_score, new_score, 1-who, 
                            (enable_time_trot * (turn + 1)) % MOD_TROT, enable_time_trot, t_id); 
                    }
                }

                if (r == 0) { // special stuff for Free Bacon (0 rolls)

                    // the win rate for free bacon = delta
                    wr = delta;

                    // set to 1 to ignore total_times_score_counted
                    total_times_score_counted = 1;

                    break; // no need to continue
                }

                else { // every other roll #

                    wr += delta * total_roll_perms_for_sum[r][k];

                    // add to total so we can divide by this later.
                    total_times_score_counted += total_roll_perms_for_sum[r][k];
                }

                if (k == 1) k = 2 * r - 1; // skip unnecessary computations
            }

            wr /= total_times_score_counted;

            dp[t_id].set(score, oppo_scoe, who, turn, trot, wr);
        }

        return dp[t_id].get(score, oppo_scoe, who, turn, trot);
    }
}
    
/* Recursively computes win rate of one strategy against another at a set of scores (memoized)

   Params:
   strat, oppo_strat: keep track of the players' strategies (NOT part of the state)
   thread_id: thread id. Index of DP vector to use for DP storage.

   The state:
   score, oppo_score: scores of the players
   who: player number of the current player
   turn: current turn number, mod 8 (needed because of time trot)
   trot: whether time trot is enabled (will be set to 0 after time trot is applied
         to ensure time trot is not used twice in a row)

    Time complexity: (N^2 * M^2) where N = goal score, M = max rolls
    Constant factors to consider: who (2), trot(2), DICE_SIDES(6)
*/
double average_win_rate(IStrategy & strategy0, IStrategy & strategy1,
            int strategy0_plays_as, int score0, int score1, int starting_turn, int thread_id) {

    // precompute permutations, which this depends on, if it has not been computed yet
    if (!perms_computed) compute_perms();

    // init dp array
    dp[thread_id].clear();

    double total = 0.0, samp = 0.0;

    if (strategy0_plays_as != 1) { // average of playing as each player
        total += average_win_rate_coroutine(strategy0, strategy1, score0, score1, 0,
            starting_turn, enable_time_trot, thread_id);

        // init dp array
        dp[thread_id].clear();
        ++ samp;
    }

    if (strategy0_plays_as != 0) {
        total += 1 - average_win_rate_coroutine(strategy1, strategy0, score1, score0, 0,
            starting_turn, enable_time_trot, thread_id);

        ++ samp;
    }

    return total / samp;
}

// compute the average win rate by sampling (i.e. playing lots of games)
double average_win_rate_by_sampling(IStrategy & strategy0, IStrategy & strategy1,
                     int strategy0_plays_as, int score0, int score1, int starting_turn, int samples) {

    int wins = 0;

    bool sub = false;
    if (strategy0_plays_as == -1) {
        sub = samples % 2;
        samples = (samples + 1) / 2;
    }

    if (strategy0_plays_as != 1) {
        for (int i = 0; i < samples; ++i) {
            auto scores = play(strategy0, strategy1, score0, score1, DEFAULT_DICE, GOAL, starting_turn);
            if (scores.first > scores.second) ++wins;
        }
    }

    if (sub) --samples;

    if (strategy0_plays_as != 0) {
        for (int i = 0; i < samples; ++i) {
            auto scores = play(strategy1, strategy0, score0, score1, DEFAULT_DICE, GOAL, starting_turn);
            if (scores.second > scores.first) ++wins;
        }
    }


    if (strategy0_plays_as == -1) {
        samples *= 2;
        if (sub) ++samples;
    }

    return (double)wins / samples;
}

// hide from linkage
namespace {
    // sorts by wins in descending order, then sorts by names alphabetically
    bool wins_comparer(std::pair<int, std::string> * a, std::pair<int, std::string> * b) {
        if (a->first > b->first) 
            return true;

        else if (a->first < b->first) 
            return false;

        else {
            return a->second < b->second;
        }
    }

    // multithreading stuff
    std::mutex mtx;
    std::condition_variable announcer_cv;
    bool announcer_lock = false;

    // coroutine for the round-robin tournament procedure
    void round_robin_coroutine(std::vector<std::pair<std::string, IStrategy *> > strats,
        void announcer(int games_played, int games_remaining, int high, std::string high_strat),
        int announcer_interval, double margin, volatile int * interrupt, 
            int * high, int * high_strat, std::vector<std::pair<int,std::string> *> victories, int * games_played,
            int jbase, int jdelta) {

        unsigned N = (unsigned)strats.size();

        int total_games = N * (N - 1) / 2;

        for (unsigned i = 0; i < N; ++i){
            // interrupt not null & set
            if (interrupt && *interrupt) break;

            for (unsigned j = i + 1 + jbase; j < N; j += jdelta){
                if (interrupt && *interrupt) break;

                std::string name0 = strats[i].first, name1 = strats[j].first;
                IStrategy * strat0 = strats[i].second, * strat1 = strats[j].second;

                double avr = average_win_rate(*strat0, *strat1, -1, 0, 0, 0, jbase);
                int winner = -1;

                if (avr > margin)
                    winner = i;
                else if (avr < 1.0 - margin)
                    winner = j;

                if (winner != -1) {
                    ++victories[winner]->first;
                    if (victories[winner]->first > (*high)) {
                        (*high) = victories[winner]->first;
                        (*high_strat) = winner;
                    }
                }

                ++(*games_played);

                if ((*games_played) % announcer_interval == 0 && announcer != NULL) {
                    std::unique_lock<std::mutex> lck(mtx);
                    while (announcer_lock) announcer_cv.wait(lck);

                    announcer_lock = true;

                    announcer(*games_played, total_games - *games_played, *high, strats[* high_strat].first);

                    announcer_lock = false;
                    announcer_cv.notify_all();
                }
            }
        }
    }
}

// Run a round-robin tournament on [thread] threads
std::vector<std::pair<int, std::string> *> round_robin(std::vector<std::pair<std::string, IStrategy *> > strats,
    void announcer(int games_played, int games_remaining, int high, std::string high_strat),
    int announcer_interval, double margin, int threads, volatile int * interrupt){

    // Compute permutations beforehand to prevent conflict between threads
    if (!perms_computed) compute_perms();

    size_t N = strats.size();
    int high = 0, high_strat = 0;

    std::vector<std::pair<int, std::string> *> victories(N);

    for (unsigned i = 0; i < N; ++i) {
        victories[i] = new std::pair<int, std::string>(0, strats[i].first);
    }

    int total_games = (int)N * ((int)N - 1) / 2;
    int games_played = 0;

    std::vector<std::thread *> threadmgr;
    threadmgr.reserve(threads);

    dp.resize(threads);

    announcer_lock = false;

    for (int i = 0; i < threads; ++i) {
        std::thread * th = new std::thread(round_robin_coroutine,
            strats, announcer, announcer_interval,
            margin, interrupt,
            &high, &high_strat, victories, &games_played, i, threads);

        threadmgr.push_back(th);
    }

    for (int i = 0; i < threads; ++i) {
        std::thread * th = threadmgr[i];
        if (th -> joinable()) {
            th -> join();
            delete th;
        }
    }

    dp.resize(1);

    std::sort(victories.begin(), victories.end(), wins_comparer);

    return victories;
}

// *** Graphing ***

// Draws a diagram representing the strategy
void draw_strategy_diagram(IStrategy & strat) {
    std::cout << "Y-axis is player score, X-axis is opponent score. Bottom left is 0, 0.\n" << std::endl;
    for (int i = GOAL - 2; i >= 0; i -= 2) {
        std::cout << "[";
        for (int j = 0; j < GOAL; ++j) {
            int result = (strat(i, j) + strat(i + 1, j)) / 2;
            switch (result) {
            case 0:
                std::cout << " ";
                break;
            case 1:
            case 2:
                std::cout << ":";
                break;
            case 3:
            case 4:
                std::cout << "|";
                break;
            case 5:
            case 6:
                std::cout << "%";
                break;
            case 7:
            case 8:
                std::cout << "\u2593";
                break;
            case 9:
            case 10:
                std::cout << "\u2588";
                break;
            default:
                std::cout << "??";
            }
        }
        std::cout << "]" << std::endl;
    }

    std::cout << "\nLEGEND:  [  ] = 0  [::] = 1,2  [||] = 3,4  [%%] = 5,6  [\u2593\u2593] = 7,8  [\u2588\u2588] = 9,10." << std::endl;
}
