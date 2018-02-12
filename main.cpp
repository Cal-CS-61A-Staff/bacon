#include "stdafx.h"
#include "hog.h"
#include "analysis.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <signal.h>

// hide from global linkage
namespace {
    // The program's version number
    const char * VERSION = "1.3.4.46";

    // The program name
    const char * APP_NAME = "Bacon";

    // The console prompt
    const char * PROMPT = "Bacon > ";

    #ifdef _WIN32
    // Root directory for storing files (Windows)
    const char * APPDATA = std::getenv("APPDATA");
    const std::string STORAGE_ROOT = std::string(APPDATA) + "\\Bacon\\";
    #else
    // Root directory for storing files (Unix)
    const char * HOME = std::getenv("HOME");
    const std::string STORAGE_ROOT = std::string(HOME) + "/.bacon/";
    #endif

    // Path to load extra strategies from
    const std::string EXT_PATH = std::string(STORAGE_ROOT).append("extras.dat");

    // Path to load the stored 'learn' strategy data from
    const std::string LEARN_PATH = std::string(STORAGE_ROOT).append("learn.dat");

    // Path to load options from
    const std::string OPTIONS_PATH = std::string(STORAGE_ROOT).append("options.dat");


    // A map containing all strategies
    std::map<std::string, IStrategy *> strat;

    // Map with only built-in strategies, used to restore built-in strategies when imported strategies are removed
    std::map<std::string, IStrategy *> builtin_strats;

    // Extra strategies added/imported by user
    std::set<std::string> extra_strats;


    // Pointer to the default LearningStrategy instance
    LearningStrategy * learning_strat;


    // indicates if running in interactive mode
    bool interactive_mode = 0;

    // virtual buffer 
    std::stringstream buf;

    // list of ouput path specified after -f switch
    std::vector<char *> output_paths;


    /* integer used for SIGINT handling. If set to a value other than 0, 
       all running learning & tournament threads will exit.*/
    static volatile int interrupt = 0;

    // SIGINT handler
    void int_handler(int dummy) {
        interrupt = 1;
    }

    // utility functions
    // check if vurtual buffer OR cin buffer has content
    inline bool has_buf() {
        return (!buf.str().empty()) || std::cin.rdbuf()->in_avail() > 1;
    }

    template <typename T>
    // read in a name from the virtual buffer or, if it is not availble, the cin buffer
    inline int read_token(T & read_to) {
        if (buf.str().empty()){
            std::cin >> read_to;
            return !std::cin.fail();
        }
        else{
            buf >> read_to;
            return true;
        }
    }

    // print a horizontal line
    inline void print_hline() {
        std::cout << "\n-----------" << std::endl;
    }

    // print a list of stratgies
    inline void list_strats(bool no_human = false, bool extras_only = false) {
        if (has_buf()) return;

        for (auto i : strat){
            if (extras_only && extra_strats.find(i.first) == extra_strats.end()) continue;
            if (no_human && i.first == "human") continue;
            std::cout << i.first << "  ";
        }
    } // list_strats

    // ask for a path
    inline int ask_for_path(char * c, int len = 256) {
        if (output_paths.size()) {
            // use path specified with -f switch
            strcpy(c, output_paths[output_paths.size()-1]);
            output_paths.pop_back();
            return true;
        }
        else {
    #ifdef _WIN32
            if (interactive_mode) // fix for Windows, do not need to ignore in interactive mode
    #endif
                std::cin.ignore(); // ignore newline
                
             std::cin.getline(c, 256);
             
             return !std::cin.fail();
        }
    } // ask_for_path

    // ask the user for a strategy name
    inline IStrategy & ask_for_strategy(const std::string & msg, bool no_human = false) {
        std::string name;
        int success = true;
        
        do{
            // interrupted by SIGINT or EOF
            if (!success || interrupt){
                interrupt = false;
                std::cout << "Interrupted, using default strategy...";
                return DEFAULT_STRATEGY;
            }
            
            if (!has_buf()) {
                std::cout << msg;
                print_hline();
                std::cout << "Available strategies:\n";
                list_strats(no_human);

                if (!no_human) std::cout << "\n\nNote: 'human' means YOU play!";
                print_hline();
            }
            
            success = read_token(name);
            
        } while ((no_human && name == "human") || strat.find(name) == strat.end());

        return *strat[name];
    } // ask_for_strategy

    // delete and erase an "extra" strategy, making sure to release memory
    inline void delete_erase_extra_strat(std::string name) {
        if (extra_strats.find(name) != extra_strats.end()) {
            delete strat[name];
            extra_strats.erase(name);
        }
    }

    // load options from file
    void load_options(void) {
        std::ifstream ifs(OPTIONS_PATH);
        
        if (ifs) {
            ifs >> enable_swine_swap >> enable_time_trot;
        }

        ifs.close();
    }

    void write_options(void) {
        std::ofstream ofs(OPTIONS_PATH);

        ofs << enable_swine_swap << " " << enable_time_trot << "\n";

        ofs.flush();
        ofs.close();
    }

    // load extra strategies
    void load_exts(const std::string & path) {
        std::ifstream ext_ifs(EXT_PATH);

        std::string header = "", name = "";

        ext_ifs >> header;

        while (header == "strategy") {
            ext_ifs >> name;
            MatrixStrategy * es = new MatrixStrategy();
            for (int i = 0; i < GOAL; ++i) {
                for (int j = 0; j < GOAL; ++j) {
                    int tmp; ext_ifs >> tmp;
                    es->set_roll_num(i, j, tmp);
                }
            }

            delete_erase_extra_strat(name);

            strat[name] = es;
            extra_strats.insert(name);

            header = "";
            ext_ifs >> header;
        }

        ext_ifs.close();
    } // load_exts

    // write out extra strategies to the path specified
    void write_exts(const std::string & path) {
        std::ofstream ext_ofs(EXT_PATH);

        for (auto name : extra_strats) {
            IStrategy * strategy = strat[name];
            if (typeid(*strategy) == typeid(MatrixStrategy)) {
                ext_ofs << "strategy";
                ext_ofs << " " << name;
                MatrixStrategy es = *(MatrixStrategy *)strat[name];

                for (int i = 0; i < GOAL; ++i) {
                    ext_ofs << "\n";
                    for (int j = 0; j < GOAL; ++j) {
                        if (j) ext_ofs << " ";
                        ext_ofs << es.get_roll_num(i, j);
                    }
                }
            }
            ext_ofs << "\n";
        }

        ext_ofs.flush();
        ext_ofs.close();
    } // write_exts

    /* set up a strategy with the name for use inside the console 
       (delete old strat with same name, add to 'strat', insert to 'extra_strats', save all strats to file, etc.) */
    inline void insert_strat_ptr(IStrategy * cs, std::string name, bool print_strats = false) {
        delete_erase_extra_strat(name);
        strat[name] = cs;

        extra_strats.insert(name);
        write_exts(EXT_PATH);

        if (print_strats) {
            print_hline();
            std::cout << "List of strategies:\n";
            list_strats();
            print_hline();
            std::cout << std::endl;
        }
    } // insert_strat_ptr

    // List all available commands (hardcoded)
    inline void show_available_commands(void) {
        std::cout << "\nAvailable commands:\n\n\
    play (-p) \t\t tournament (-t [-f]) \t train (-l) \t\t learnfrom (-lf) \n\
    winrate[0|1] (-r)\t avgwinrate[0|1]\t mkfinal \t\t mkrandom\n\
    get (-s) \t\t diff (-d) \t\t graph (-g) \t\t graphdiff (-gd) \n\
    list (-ls) \t\t import (-i [-f]) \t export[py] (-e [-f]) \t clone (-c)\n\
    remove (-rm) \t help (-h) \t\t version (-v) \t\t option (-o) \t\t\n\
    \t\t\t\t\t\t time \t\t\t exit" << std::endl << std::endl;
    } // show_available_commands

    // Announcer for round robin tournament
    void announcer(int games_played, int games_remaining, int high, std::string high_strat) {
        std::cout << games_played << " games played, " << games_remaining <<
            " remaining. '" << high_strat << "' is leading with " << high << " wins." << std::endl;
    }

    // Execute Bacon command cmd
    void exec(const std::string & cmd){
        // cancel any interrupts
        interrupt = 0;

        // the game of Hog
        if (cmd == "-p" || cmd == "play") {
            AnnouncerStrategy as1 = AnnouncerStrategy(ask_for_strategy("\nPlayer 0 strategy name:"), 0);
            AnnouncerStrategy as2 = AnnouncerStrategy(ask_for_strategy("\nPlayer 1 strategy name:"), 1);
            IStrategy & s0 = as1;
            IStrategy & s1 = as2;

            std::cout << std::endl;

            auto scores = play(s0, s1);

            std::cout << "Current score: " << scores.first << "-" << scores.second << std::endl;
            if (scores.first > scores.second)
                std::cout << "\nPlayer 0 wins!\n" << std::endl;
            else
                std::cout << "\nPlayer 1 wins!\n" << std::endl;
        }

        else if (cmd == "-t" || cmd == "tournament") {
            std::cout << "\n--Round-robin Tournament Tool--" << std::endl;

            std::vector<std::pair<std::string, IStrategy *> > contestants;

            for (auto name : extra_strats) {
                if (name != "final")
                    contestants.push_back(make_pair(name, strat[name]));
            }

            size_t total_strats = contestants.size();
            int total_games = (int)total_strats * ((int)total_strats - 1) / 2;

            std::cout << "Number of strategies: " << total_strats << "\n";
            std::cout << "Total games to play: " << total_games << "\n";

            if (output_paths.size() == 0) std::cout << "\nNumber of threads:" << std::endl;
            
            int thds = 4;
            int success = read_token(thds);
            if (!success) return;
            
            if (thds <= 0) thds = 4;
            
            if (output_paths.size() == 0) std::cout << "\nFile to save results to when done:" << std::endl;

            char path[256];
            ask_for_path(path);

            std::cout << std::endl;

            std::cout << "Running tournament..." << std::endl;

            std::vector<std::pair<int, std::string> *> results = 
                round_robin(contestants, announcer, 100, 0.500001, thds, &interrupt);

            if (interrupt) 
                std::cout << "\nTournament interrupted by user. Incomplete results:\n\n";
            else 
                std::cout << "\nAll games have finished. Final results:\n\n";


            int rank = 1, ties = 0;

            
            std::ofstream save_ofs;

            if (!interrupt) {
                save_ofs.open(path, std::ofstream::out | std::ofstream::trunc);
                if (save_ofs.fail() || !save_ofs.is_open()) {
                    std::cout << "Could not access the specified path. Please check if the path is correct and if "
                        "the file is being used. Results will be saved to results.txt in the current directory.\n";
                    save_ofs.clear();
                    save_ofs.open(path, std::ofstream::out | std::ofstream::trunc);
                }
            }

            for (unsigned i = 0; i < results.size(); ++i) {
                if (i && results[i]->first < results[i - 1]->first) {
                    rank += ties;
                    ties = 1;
                }
                else {
                    ++ties;
                }

                std::cout << rank << ". " << results[i]->second << " with " << results[i]->first << " wins \n";

                if (!interrupt) 
                    save_ofs << rank <<  ". " << results[i]->second << " with " << results[i]->first << " wins \n";
            }

            for (unsigned i = 0; i < results.size(); ++i) {
                delete results[i];
            }

            if (interrupt) 
                std::cout << "\n(Results are incomplete and so haven't been written to file)\n" << std::endl;
            else
                std::cout << "\nResults have been written to '" << path << "'.\n" << std::endl;
        }

        // learning
        else if (cmd == "-l" || cmd == "train") {
            int number;
            if (!has_buf()) std::cout << "Number of training rounds: ";

            int success = read_token(number);
            if (!success) return;

            IStrategy & s0 = ask_for_strategy("\nTraining opponent strategy name:", true);

            std::cout << std::endl;

            std::pair<int, int> focus(rand() % GOAL, rand() % GOAL);
            learning_strat->learn(s0, number, focus, &interrupt);

            std::cout << std::endl;

            learning_strat->write_to_file(LEARN_PATH);
        }

        else if (cmd == "-lf" || cmd == "learnfrom") {
            IStrategy & s0 =
                ask_for_strategy("\nName of strategy to use for training (will be copied to 'learn'):");

            delete learning_strat;
            learning_strat = new LearningStrategy(s0, LEARN_PATH);
            std::cout << "Strategy 'learn' overwritten. Run 'train' to begin training.\n" << std::endl;
        }

        // strategic analysis
        else if (cmd == "-r" || cmd == "-r0" || cmd == "-r1" ||
                 cmd == "winrate" || cmd == "winrate0" || cmd == "winrate1") {

            bool argmode = has_buf();

            IStrategy & s0 = ask_for_strategy("\nPlayer 0 strategy name:", true);
            IStrategy & s1 = ask_for_strategy("\nPlayer 1 strategy name:", true);

            double rate = average_win_rate(s0, s1,
                ((cmd == "-r" || cmd == "winrate") ? -1 : ((cmd == "-r1" || cmd == "winrate1") ? 1 : 0)),
                0, 0, 0);

            std::cout << "Win rate: " << rate << "\n" << std::endl;
        }

        else if (cmd == "avgwinrate" || cmd == "avgwinrate0" || cmd == "avgwinrate1") {

            bool argmode = has_buf();

            IStrategy & s0 = ask_for_strategy("\nPlayer 0 strategy name:", true);
            IStrategy & s1 = ask_for_strategy("\nPlayer 1 strategy name:", true);

            int samples = 200000;
            if (!has_buf() && !argmode)
                std::cout << "\nSamples: ";

            if (has_buf() || !argmode)
                read_token(samples);

            double rate = average_win_rate_by_sampling(s0, s1,
                (cmd == "avgwinrate" ? -1 : (cmd == "avgwinrate1" ? 1 : 0)),
                0, 0, 0, samples);

            std::cout << "Win rate @ " << samples << " samples: " << rate << "\n" << std::endl;
        }

        else if (cmd == "-s" || cmd == "get") {
            IStrategy & s0 = ask_for_strategy("\nName of strategy:");

            int score0;
            if (!has_buf()) std::cout << "Player 0 score: ";
            read_token(score0);

            int score1;
            if (!has_buf()) std::cout << "Player 1 score: ";
            read_token(score1);

            if (score0 >= GOAL || score1 >= GOAL || score0 < 0 || score1 < 0){
                std::cout << "\nInvalid scores! Please enter a score between 0-" << GOAL << std::endl;
            }
            else{
                std::cout << "\nRolls for (" << score0 << "," << score1 << "): " << s0(score0, score1) << "\n" << std::endl;
            }
        }

        else if (cmd == "-d" || cmd == "diff") {
            IStrategy & s0 = ask_for_strategy("\nName of strategy 1:", true);
            IStrategy & s1 = ask_for_strategy("\nName of strategy 2:", true);

            int count = 0;
            const int MAX_COUNT = 200;

            for (int i = 0; i < GOAL; ++i) {
                for (int j = 0; j < GOAL; ++j) {
                    int r1 = s0(i, j);
                    int r2 = s1(i, j);
                    if (r1 != r2) {
                        if (count <= MAX_COUNT)
                            std::cout << "at " << (i < 10 ? "0" : "") << i << ", " << (j < 10 ? "0" : "") << j << ":\t" << r1 << " \t " << r2 << "\t(difference of " << r2 - r1 << " )\n";
                        ++count;
                    }
                }
            }

            if (count > MAX_COUNT) {
                std::cout << "Too many differences between the two strategies to show individually. " <<
                    "Try using graphdiff to compare.\n\n";
            }

            if (count == 0) std::cout << "The two strategies are identical. No differences found.\n" << std::endl;
            else std::cout << "Found a total of " << count << " differences.\n\n";
        }

        else if (cmd == "-g" || cmd == "graph") {
            IStrategy & s0 = ask_for_strategy("\nPlayer 0 strategy name:", true);
            std::cout << std::endl;
            draw_strategy_diagram(s0);
            std::cout << std::endl;
        }

        else if (cmd == "-gd" || cmd == "graphdiff") {
            IStrategy & s0 = ask_for_strategy("\nName of strategy 1:", true);
            IStrategy & s1 = ask_for_strategy("\nName of strategy 2:", true);

            MatrixStrategy * cs = new MatrixStrategy();

            for (int i = 0; i < GOAL; ++i) {
                for (int j = 0; j < GOAL; ++j) {
                    cs->set_roll_num(i, j, abs(s1(i, j) - s0(i, j)));
                }
            }

            std::cout << std::endl;
            draw_strategy_diagram(*cs);
            std::cout << std::endl;

            delete cs;
        }

        else if (cmd == "mkfinal") {
            std::string name;

            do {
                if (!has_buf())
                    std::cout << "\nName to use for this strategy\n(Warning: using the name of an " <<
                    "existing strategy will override that strategy!):\n\n";
            } while (name.length() == 0 && read_token(name) && !interrupt);
            if (interrupt) {interrupt = false; return;}

            MatrixStrategy * opti_strat = create_final_strat();

            insert_strat_ptr(opti_strat, name, true);
        }

        else if (cmd == "mkrandom") {
            std::string name;
            do {
                if (!has_buf())
                    std::cout << "\nName to use for this strategy\n(Warning: using the name of an " <<
                    "existing strategy will override that strategy!):\n\n";
            } while (name.length() == 0 && read_token(name) && !interrupt);
            if (interrupt) {interrupt = false; return;}

            RandomStrategy rs = RandomStrategy();
            MatrixStrategy * rand_strat = new MatrixStrategy(rs);

            std::cout << "Randomized strategy saved to " << name << "." << std::endl;
            insert_strat_ptr(rand_strat, name, true);
        }

        // strategy manager
        else if (cmd == "-ls" || cmd == "list") {
            print_hline();
            std::cout << "Available strategies:\n";
            list_strats(false, false);
            print_hline();
            std::cout << std::endl;
        }

        else if (cmd == "-e" || cmd == "export" || cmd == "exportpy") {
            std::string name;
            while (true) {
                std::cout << "\n--Strategy Export Tool--\n";
                if (cmd == "exportpy") std::cout << "(Python Format)\n";

                if (!has_buf()) {
                    std::cout << "\nPlease enter the name of the strategy to export (enter cancel to exit):\n";
                    print_hline();

                    std::cout << "Available strategies:\n";
                    list_strats();
                    print_hline();
                }

                int success = read_token(name);
                if (!success || name == "cancel" || interrupt) break;
                
                auto it = strat.find(name);
                if (it != strat.end()) {
                    // found!
                    MatrixStrategy tmpcs = MatrixStrategy(*it->second);

                    if (output_paths.size() == 0) std::cout << "\nExport file path:" << std::endl;

                    char path[256];
                    ask_for_path(path);

                    std::cout << std::endl;

                    try {
                        if (cmd == "-e" || cmd == "export")
                            tmpcs.write_to_file(path);
                        else
                            tmpcs.write_to_file(path, true);

                        std::cout << "Strategy exported to: '" << path << "'\n" << std::endl;
                    }
                    catch (...) {
                        std::cout << "Export failed!\n" << std::endl;
                    }
                    break;
                }
                else {
                    std::cout << "\nStrategy not found. (Enter cancel to exit)" << std::endl;;
                }

            }
            if (interrupt) interrupt = false;
        }

        else if (cmd == "-i" || cmd == "import") {
            std::cout << "\n--Strategy Import Tool--\n";

            bool single_mode = (output_paths.size() == 0);
            if (single_mode) std::cout << "Please enter the strategy file path:" << std::endl;

            int ct = 0;

            do {
                char path[256];
                int success = ask_for_path(path);
                
                if (interrupt || !success) break;

                std::string name;
                if (!single_mode) {
                    name = std::string(path);

                    size_t pos = name.find_last_of("\\/");
                    if (pos != name.npos) name = name.substr(pos + 1);

                    size_t pos2 = name.find_last_of('.');
                    if (pos2 != name.npos) name = name.substr(0, pos2);
                }

                MatrixStrategy * cs = new MatrixStrategy();

                if (cs->load_from_file(path)) {
                    while (name.length() == 0) {
                        std::cout << "\nName to use for this strategy\n(Warning: using the name of an existing strategy will override that strategy!):\n\n";
                        read_token(name);
                        if (name == "learn") {
                            std::cout << "The name 'learn' is reserved.\n";
                            name = "";
                        }
                    }

                    ++ct;
                    insert_strat_ptr(cs, name, single_mode);
                }
                else {
                    std::cout << "Import failed. Please check if the file path is correct.\n\n";
                }

            } while (output_paths.size() > 0);
            
            if (interrupt) {interrupt = false; return;}

            if (ct) {
                if (single_mode || ct == 1)
                    std::cout << ct << " strategy imported.\n" << std::endl;
                else
                    std::cout << ct << " strategies imported.\n" << std::endl;
            }
        }

        else if (cmd == "-rm" || cmd == "remove") {

            std::string name;
            while (true) {
                if (extra_strats.empty()) {
                    std::cout << "Can't remove because no strategies have been imported.\n\n";

                    break;
                }

                if (!has_buf()) {
                    std::cout << "\nName of imported strategy to remove (enter cancel to exit):\n";

                    print_hline();

                    std::cout << "Available imported strategies:\n";
                    list_strats(false, true);
                    print_hline();
                }

                int success = read_token(name);
                if (name == "cancel" || interrupt || !success) break;
                
                auto it = extra_strats.find(name);
                bool erase_all = (it == extra_strats.end()) && (name == "all" || name == "*");
                
                if (it != extra_strats.end() || erase_all) {
                    if (erase_all){
                        // erase all
                        for (auto name : extra_strats){
                            delete strat[name];
                            strat.erase(name);
                        }
                        
                        extra_strats.clear();
                    }
                    else{	
                        // found single match
                        extra_strats.erase(it);
                        delete strat[name];
                        strat.erase(name);
                    }

                    write_exts(EXT_PATH);
                    
                    if (erase_all)
                        std::cout << "All imported strategies removed.\n";
                    else
                        std::cout << "Strategy removed.\n";

                    if (builtin_strats.find(name) != builtin_strats.end()) {
                        strat[name] = builtin_strats[name]; // reset internal strategy w/ the name
                        std::cout << "Internal strategy '" << name << "' restored.\n\n";
                    }

                    print_hline();
                    std::cout << "List of strategies:\n";
                    list_strats();
                    print_hline();
                    break;
                }
                else {
                    std::cout << "\nStrategy does not exist or is not an imported strategy. (Enter cancel to exit)" << std::endl;;
                }

            }
            
            if (interrupt) {interrupt = false; return;}
        }

        else if (cmd == "-c" || cmd == "clone") {
            IStrategy & s0 = ask_for_strategy("\nName of strategy to clone:");

            std::string name;
            while (name.length() == 0) {
                if (!has_buf())
                    std::cout << "\nName to use for the cloned strategy\n" <<
                    "(Warning: using the name of an existing strategy will override that strategy!):\n\n";

                int success = read_token(name);
                if (!success || interrupt) {interrupt =  false; return;}

                if (name == "learn") {
                    std::cout << "The name 'learn' is reserved.\n";
                    name = "";
                }
            }

            MatrixStrategy * cs = new MatrixStrategy(s0);

            std::cout << "Cached copy of strategy saved to: " << name << "." << std::endl;

            insert_strat_ptr(cs, name, true);
        }

        // logistics
        else if (cmd == "-v" || cmd == "version") {
            std::cout << APP_NAME << " version " << VERSION <<
                "\n(c) Alex Yu 2017\n\nSwine Swap: " <<
                (enable_swine_swap ? "Enabled" : "Disabled") <<
                "\nTime Trot: " <<
                (enable_time_trot ? "Enabled" : "Disabled") << "\n" << std::endl;
        }

        else if (cmd == "-o" || cmd == "option") {
            std::string name;
            std::vector<std::string> opt = { "swap", "trot" };

            while (name.length() == 0) {
                if (!has_buf()) {
                    std::cout << "\nName of option to adjust (choices:";
                    for (unsigned i = 0; i < opt.size(); ++i) std::cout << " " << opt[i];
                    std::cout << "): ";
                }

                int success = read_token(name);
                if (!success || interrupt) { interrupt = false; return; }

                if (std::find(opt.begin(), opt.end(), name) == opt.end()) {
                    std::cout << "\nInvalid option. (choices: ";
                    for (unsigned i = 0; i < opt.size(); ++i) std::cout << " " << opt[i];
                    std::cout << ")\n";

                    name = "";
                }
            }

            std::string value;
            while (value.length() == 0) {
                if (!has_buf()) {
                    std::cout << "\nValue for option '" << name << "': (on/off): ";
                }

                int success = read_token(value);
                if (!success || interrupt) { interrupt = false; return; }

                if (value == "on") {
                    if (name == "swap")
                        enable_swine_swap = 1;
                    else
                        enable_time_trot = 1;
                }

                else if (value == "off") {
                    if (name == "swap")
                        enable_swine_swap = 0;
                    else
                        enable_time_trot = 0;
                }

                else {
                    std::cout << "Invalid value. Please enter on/off (lower case):";
                    value = "";
                }
            }

            write_options();

            std::cout << "\nOption '" << name << "' set to " << value << "\n" << std::endl;
        }

        else if (cmd == "time") {
            std::string command;
            if (!has_buf()) std::cout << "Command to measure:";
            read_token(command);

            clock_t begin = clock();

            exec(command);

            clock_t end = clock();

            double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

            if (time_spent < 0.7) {
                std::cout << "time: task took " << time_spent * 1000 << " ms\n";
            }
            else {
                std::cout << "time: task took " << time_spent << " seconds\n";
            }

            std::cout << std::endl;
        }

        else {
            show_available_commands();

            if (cmd == "-h" || cmd == "help") {
                std::cout << "--The Game of Hog--\n\
    play (-p): simulate a game of Hog between two strategies (or play against one of them).\n\
    tournament (-t): run a tournament with all the imported strategies. Use the -f switch to specify output file path: bacon -t -f output.txt\n\n\
    \
    --Learning--\n\
    train (-l): start training against a specified strategy (improves the 'learn' strategy).\n\
    learnfrom(-lf): sets the 'learn' strategy to a copy of the specified strategy. The 'train' command will now train this new strategy.\n\n\
    \
    --Strategic Analysis--\n\
    winrate (-r): get the theoretical win rate of a strategy against another one.\n\
    avgwinrate: get the average win rate of a strategy against another one using sampling.\n\
    winrate0 (-r0), winrate1 (-r1), avgwinrate0, avgwinrate1: force the first strategy to play as player #.\n\n\
    mkfinal: re-compute the 'final' strategy; saves the result to the specified strategy name.\n\
    mkrandom: creates a randomized strategy and saves the result to the specified strategy name.\n\n\
    get (-s): see what a given strategy would roll at a given set of scores.\n\
    diff (-d): get the differences in between two strategies.\n\
    graph (-g): get a graphic representation of a strategy.\n\
    graphdiff (-gd): get a graphic representation of the differences between two strategies.\n\n\
    \
    --Strategy Manager--\n\
    list (-ls): show a list of available strategies. \n\
    import (-i): add a new strategy from a file. Use the -f switch to specify import file path(s): bacon -i mystrategy -f a.strat\n\
    export (-e): export a strategy to a file. Use the -f switch parameter to specify output file path: bacon -o final -f final.strat \n\
    exportpy: export a strategy to a Python script that defines a function called 'strategy'.\n\
    clone (-c): clones an existing strategy and saves a cached copy of it to a new name.\n\
    remove (-rm): remove an imported strategy and restore an internal strategy, if available. Enter 'remove all' or '-r all' to clear all imported strategies.\n\n\
    \
    --Logistics--\n\
    help (-h): display this help.\n\
    version (-v): display the version number.\n\
    option (-o): adjust options (turn on/off Swine Swap, Time Trot).\n\
    time: measure the runtime of any bacon command.\n\
    exit: get out of here!\n";
                std::cout << std::endl;
            }
        }

    } // exec

    inline void init_console(void) {
        srand((unsigned int)time(NULL));

        // load options
        load_options();

        learning_strat = new LearningStrategy(DEFAULT_STRATEGY, LEARN_PATH, false);

        learning_strat->load_from_file(LEARN_PATH);

        // prepare and add internal strategies
        strat["swap"] = new SwapStrategy();
        strat["swap_visual"] = new SwapVisualiser();
        strat["human"] = new HumanStrategy();
        strat["random"] = new RandomStrategy();
        strat["learn"] = learning_strat;

        strat["default"] = &DEFAULT_STRATEGY;

        for (int i = 0; i <= MAX_ROLLS; ++i) {
            std::stringstream st;
            st << "always" << i;
            strat[st.str()] = new AlwaysRollStrategy(i);
        }

        for (auto pair : strat) {
            builtin_strats[pair.first] = pair.second;
        }

        // load extra strategies
        load_exts(EXT_PATH);

        // create final strategy, if it is not in the cache/has been overridden
        if (extra_strats.find("final") == extra_strats.end()) {
            MatrixStrategy * cached_fs = create_final_strat(true);
            strat["final"] = cached_fs;

            // allow removal of final strategy
            extra_strats.insert("final");
        }
    } // init_console
} // anonymous namespace

int main(int argc, char * argv[]) {
    signal(SIGINT, int_handler);

    // initialize
    init_console();

    // speed up cin
    std::cin.sync_with_stdio(0);
		
    #ifdef _WIN32
            // Windows only

            // change title of console, if possible
            system((std::string("Title ") + APP_NAME).c_str());

            CreateDirectoryA(STORAGE_ROOT.c_str(), NULL);
    #else
        
            // make the storage directory, if possible
            system((std::string("mkdir -p ") + STORAGE_ROOT).c_str());
    #endif    

    if (argc <= 1) { // interactive mode

        std::string cmd;

        // Print greeting message
        std::cout << "Welcome to " << APP_NAME << " v." << VERSION << "!\n(c) Alex Yu 2017" << std::endl;

        // We are in interactive mode
        interactive_mode = true;
		
        // Main loop
        do {
            exec(cmd);
            std::cout << PROMPT;
        } while (std::cin >> cmd && cmd != "exit");
    }

    else { // read args
        bool fpath_param = false;

        for (int i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "-f") == 0) {
                fpath_param = true; continue;
            }

            if (fpath_param) {
                // this occurs after the -f switch: add to list of paths
                output_paths.push_back(argv[i]);
            }

            else {
                // else add all the arguments to virtual buffer
                if (i != 2) buf << " ";
                buf << argv[i];
            }
        }

        exec(argv[1]);
    }

    learning_strat->write_to_file(LEARN_PATH); // save learning strategy
    write_exts(EXT_PATH); // write out extra strategies
    write_options();

    // clean up

    for (auto name : strat) {
        try {
            delete name.second;
        }
        catch (...){
            // ignore error
        }
    }

    return 0;
} // main
