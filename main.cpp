#include "stdafx.h"
#include "hog.h"
#include "analysis.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <signal.h>

using namespace std;

// The program's version number
const char * VERSION = "1.3.2.43";

// The program name
const char * NAME = "Bacon Console";

// The console prompt
const char * PROMPT = "Bacon > ";

#ifdef _WIN32
// Root directory for storing files (Windows)
const char * APPDATA = getenv("APPDATA");
const string STORAGE_ROOT = string(APPDATA) + "\\Bacon\\";
#else
// Root directory for storing files (Unix)
const char * HOME = getenv("HOME");
const string STORAGE_ROOT = string(HOME) + "/.bacon/";
#endif

// Path to load extra strategies from
const string EXT_PATH = string(STORAGE_ROOT).append("extras.dat");

// Path to load the stored 'learn' strategy data from
const string LEARN_PATH = string(STORAGE_ROOT).append("learn.dat");

// Path to load options from
const string OPTIONS_PATH = string(STORAGE_ROOT).append("options.dat");

// A map containing all strategies
map<string, IStrategy *> strat;

// Map with only built-in strategies
map<string, IStrategy*> builtin_strats;

// Extra strategies added by user
set<string> extra_strats;

// Pointer to the default LearningStrategy instance
LearningStrategy * learning_strat;

// virtual buffer 
stringstream buf;

// list of ouput path specified after -f switch
vector<char *> output_paths;

// SIGINT handling
static volatile int interrupt = 0;

// indicates if running in console mode
bool console_mode = 0;

void int_handler(int dummy) {
    interrupt = 1;
}

// utility functions
// check if vurtual buffer OR cin buffer has content
inline bool has_buf() {
    return (!buf.str().empty()) || cin.rdbuf()->in_avail() > 1;
}

template <typename T>
// read in a name from the virtual buffer or, if it is not availble, the cin buffer
inline void read_token(T & read_to) {
    if (buf.str().empty())
        cin >> read_to;
    else
        buf >> read_to;
}

// print a horizontal line
inline void print_hline() {
    cout << "\n-----------" << endl;
}

// print a list of stratgies
inline void list_strats(bool no_human = false, bool extras_only = false) {
    if (has_buf()) return;

    for (auto i : strat){
        if (extras_only && extra_strats.find(i.first) == extra_strats.end()) continue;
        if (no_human && i.first == "human") continue;
        cout << i.first << "  ";
    }
}

// ask for a path
inline void ask_for_path(char * c, int len = 256) {
    if (output_paths.size()) {
        // use path specified with -f switch
        strcpy(c, output_paths[output_paths.size()-1]);
        output_paths.pop_back();
    }
    else {
#ifdef _WIN32
        if (console_mode) 
#endif
			cin.ignore();
			
        cin.getline(c, 256);
    }
}

// ask the user for a strategy name
inline IStrategy & ask_for_strategy(string msg, bool no_human = false) {
    string name;

    do{
        if (!has_buf()) {
            cout << msg;
            print_hline();
            cout << "Available strategies:\n";
            list_strats(no_human);

            if (!no_human) cout << "\n\nNote: 'human' means YOU play!";
            print_hline();
        }

        read_token(name);

    } while ((no_human && name == "human") || strat.find(name) == strat.end());

    return *strat[name];
}

// delete and erase an "extra" strategy, making sure to release memory
inline void delete_erase_extra_strat(string name) {
    if (extra_strats.find(name) != extra_strats.end()) {
        delete strat[name];
        extra_strats.erase(name);
    }
}

// load options from file
void load_options(void) {
    ifstream ifs(OPTIONS_PATH);
    
    if (ifs) {
        ifs >> enable_swine_swap >> enable_time_trot;
    }

    ifs.close();
}

void write_options(void) {
    ofstream ofs(OPTIONS_PATH);

    ofs << enable_swine_swap << " " << enable_time_trot << "\n";

    ofs.flush();
    ofs.close();
}

// load extra strategies
void load_exts(string path) {
    ifstream ext_ifs(EXT_PATH);

    string header = "", name = "";

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
}

// write out extra strategies to the path specified
void write_exts(string path) {
    ofstream ext_ofs(EXT_PATH);

    for (auto name : extra_strats) {
        if (typeid(*strat[name]) == typeid(MatrixStrategy)) {
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
}

/* set up a strategy with the name for use inside the console 
   (delete old strat with same name, add to 'strat', insert to 'extra_strats', save all strats to file, etc.) */
inline void insert_strat_ptr(IStrategy * cs, string name, bool print_strats = false) {
    delete_erase_extra_strat(name);
    strat[name] = cs;

    extra_strats.insert(name);
    write_exts(EXT_PATH);

    if (print_strats) {
        print_hline();
        cout << "List of strategies:\n";
        list_strats();
        print_hline();
        cout << endl;
    }
}

// List all available commands (hardcoded)
inline void show_available_commands(void) {
    cout << "\nAvailable commands:\n\n\
play (-p) \t\t tournament (-t [-f]) \t train (-l) \t\t learnfrom (-lf) \n\
winrate[0|1] (-r[0|1])\t avgwinrate[0|1]\t mkfinal \t\t mkrandom\n\
get (-s) \t\t diff (-d) \t\t graph (-g) \t\t graphdiff (-gd) \n\
list (-l) \t\t import (-i [-f]) \t export[py] (-e [-f]) \t clone (-c)\n\
remove (-rm) \t\t help (-h) \t\t version (-v) \t\t option (-o) \t\t\n\
\t\t\t\t\t\t time \t\t\t exit" << endl << endl;
}

void announcer(int games_played, int games_remaining, int high, string high_strat) {
    cout << games_played << " games played, " << games_remaining <<
        " remaining. '" << high_strat << "' is leading with " << high << " wins.\n";
}

void exec(string cmd){
    // cancel any interrupts
    interrupt = 0;

    // the game of Hog
    if (cmd == "-p" || cmd == "play") {
        AnnouncerStrategy as1 = AnnouncerStrategy(ask_for_strategy("\nPlayer 0 strategy name:"), 0);
        AnnouncerStrategy as2 = AnnouncerStrategy(ask_for_strategy("\nPlayer 1 strategy name:"), 1);
        IStrategy & s0 = as1;
        IStrategy & s1 = as2;

        cout << endl;

        auto scores = play(s0, s1);

        cout << "Current score: " << scores.first << "-" << scores.second << endl;
        if (scores.first > scores.second)
            cout << "\nPlayer 0 wins!\n" << endl;
        else
            cout << "\nPlayer 1 wins!\n" << endl;
    }

    else if (cmd == "-t" || cmd == "tournament") {
        cout << "\n--Round-robin Tournament Tool--" << endl;

        vector<pair<string, IStrategy *> > contestants;

        for (auto name : extra_strats) {
            if (name != "final")
                contestants.push_back(make_pair(name, strat[name]));
        }

        int total_strats = contestants.size();
        int total_games = total_strats * (total_strats - 1) / 2;


        cout << "Number of strategies: " << total_strats << "\n";
        cout << "Total games to play: " << total_games << "\n";

		if (output_paths.size() == 0) cout << "\nNumber of threads:" << endl;
		
		int thds = 4;
		read_token(thds);
		
		if (thds <= 0) thds = 4;
		
        if (output_paths.size() == 0) cout << "\nFile to save results to when done:" << endl;

        char path[256];
        ask_for_path(path);

        cout << endl;

        ofstream save_ofs(path);
        if (!save_ofs) {
            cout << "Could not access the specified path. Please check if the path is correct and if "
                "the file is being used.\n";
            return;
        }

        cout << "Running tournament..." << endl;

        vector<pair<int, string> *> results = 
            round_robin(contestants, announcer, 50, 0.500001, thds, &interrupt);

        if (interrupt) 
            cout << "\nTournament interrupted by user. Incomplete results:\n\n";
        else 
            cout << "\nAll games have finished. Final results:\n\n";


        int rank = 1, ties = 0;

        for (unsigned i = 0; i < results.size(); ++i) {
            if (i && results[i]->first < results[i - 1]->first) {
                rank += ties;
                ties = 1;
            }
            else {
                ++ties;
            }

            cout << rank << ". " << results[i]->second << " with " << results[i]->first << " wins \n";

            if (!interrupt) 
                save_ofs << rank <<  ". " << results[i]->second << " with " << results[i]->first << " wins \n";
        }

        for (unsigned i = 0; i < results.size(); ++i) {
            delete results[i];
        }

        if (interrupt) 
            cout << "\n(Results are incomplete and so haven't been written to file)\n" << endl;
        else
            cout << "\nResults have been written to '" << path << "'.\n" << endl;
    }

    // learning
    else if (cmd == "-l" || cmd == "train") {
        int number;
        if (!has_buf()) cout << "Number of training rounds: ";

        read_token(number);

        IStrategy & s0 = ask_for_strategy("\nTraining opponent strategy name:", true);

        cout << endl;

        pair<int, int> focus(rand() % GOAL, rand() % GOAL);
        learning_strat->learn(s0, number, focus, &interrupt);

        cout << endl;

        learning_strat->write_to_file(LEARN_PATH);
    }

    else if (cmd == "-lf" || cmd == "learnfrom") {
        IStrategy & s0 =
            ask_for_strategy("\nName of strategy to use for training (will be copied to 'learn'):");

        delete learning_strat;
        learning_strat = new LearningStrategy(s0, LEARN_PATH);
        cout << "Strategy 'learn' overwritten. Run 'train' to begin training.\n" << endl;
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

        cout << "Win rate: " << rate << "\n" << endl;
    }

    else if (cmd == "avgwinrate" || cmd == "avgwinrate0" || cmd == "avgwinrate1") {

        bool argmode = has_buf();

        IStrategy & s0 = ask_for_strategy("\nPlayer 0 strategy name:", true);
        IStrategy & s1 = ask_for_strategy("\nPlayer 1 strategy name:", true);

        int samples = 200000;
        if (!has_buf() && !argmode)
            cout << "\nSamples: ";

        if (has_buf() || !argmode)
            read_token(samples);

        double rate = average_win_rate_by_sampling(s0, s1,
            (cmd == "avgwinrate" ? -1 : (cmd == "avgwinrate1" ? 1 : 0)),
            0, 0, 0, samples);

        cout << "Win rate @ " << samples << " samples: " << rate << "\n" << endl;
    }

    else if (cmd == "-s" || cmd == "get") {
        IStrategy & s0 = ask_for_strategy("\nName of strategy:");

        int score0;
        if (!has_buf()) cout << "Player 0 score: ";
        read_token(score0);

        int score1;
        if (!has_buf()) cout << "Player 1 score: ";
        read_token(score1);

        cout << "\nRolls for (" << score0 << "," << score1 << "): " << s0(score0, score1) << "\n" << endl;
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
                        cout << "at " << (i < 10 ? "0" : "") << i << ", " << (j < 10 ? "0" : "") << j << ":\t" << r1 << " \t " << r2 << "\t(difference of " << r2 - r1 << " )\n";
                    ++count;
                }
            }
        }

        if (count > MAX_COUNT) {
            cout << "Too many differences between the two strategies to show individually. " <<
                "Try using graphdiff to compare.\n\n";
        }

        if (count == 0) cout << "The two strategies are identical. No differences found.\n" << endl;
        else cout << "Found a total of " << count << " differences.\n\n";
    }

    else if (cmd == "-g" || cmd == "graph") {
        IStrategy & s0 = ask_for_strategy("\nPlayer 0 strategy name:", true);
        cout << endl;
        draw_strategy_diagram(s0);
        cout << endl;
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

        cout << endl;
        draw_strategy_diagram(*cs);
        cout << endl;

        delete cs;
    }

    else if (cmd == "mkfinal") {
        string name;

        while (name.length() == 0) {
            if (!has_buf())
                cout << "\nName to use for this strategy\n(Warning: using the name of an " <<
                "existing strategy will override that strategy!):\n\n";
            read_token(name);
        }

        MatrixStrategy * opti_strat = create_final_strat();

        insert_strat_ptr(opti_strat, name, true);
    }

    else if (cmd == "mkrandom") {
        string name;
        while (name.length() == 0) {
            if (!has_buf())
                cout << "\nName to use for this strategy\n(Warning: using the name of an " <<
                "existing strategy will override that strategy!):\n\n";
            read_token(name);
        }

        RandomStrategy rs = RandomStrategy();
        MatrixStrategy * rand_strat = new MatrixStrategy(rs);

        cout << "Randomized strategy saved to " << name << "." << endl;
        insert_strat_ptr(rand_strat, name, true);
    }

    // strategy manager
    else if (cmd == "-ls" || cmd == "list") {
        print_hline();
        cout << "Available strategies:\n";
        list_strats(false, false);
        print_hline();
        cout << endl;
    }

    else if (cmd == "-e" || cmd == "export" || cmd == "exportpy") {
        string name;
        while (true) {
            cout << "\n--Strategy Export Tool--\n";
            if (cmd == "exportpy") cout << "(Python Format)\n";

            if (!has_buf()) {
                cout << "\nPlease enter the name of the strategy to export (enter cancel to exit):\n";
                print_hline();

                cout << "Available strategies:\n";
                list_strats();
                print_hline();
            }

            read_token(name);

            if (name == "cancel") break;
            auto it = strat.find(name);
            if (it != strat.end()) {
                // found!
                MatrixStrategy tmpcs = MatrixStrategy(*it->second);

                if (output_paths.size() == 0) cout << "\nExport file path:" << endl;

                char path[256];
                ask_for_path(path);

                cout << endl;

                try {
                    if (cmd == "-e" || cmd == "export")
                        tmpcs.write_to_file(path);
                    else
                        tmpcs.write_to_file(path, true);

                    cout << "Strategy exported to: '" << path << "'\n" << endl;
                }
                catch (exception ex) {
                    cout << "Export failed!\n" << endl;
                }
                break;
            }
            else {
                cout << "\nStrategy not found. (Enter cancel to exit)" << endl;;
            }

        }
    }

    else if (cmd == "-i" || cmd == "import") {
        cout << "\n--Strategy Import Tool--\n";

        bool single_mode = (output_paths.size() == 0);
        if (single_mode) cout << "Please enter the strategy file path:" << endl;

        int ct = 0;

        do {
            char path[256];
            ask_for_path(path);

            string name;
            if (!single_mode) {
                name = string(path);
                int pos = name.find_last_of("\\/");
                if (pos != name.npos) name = name.substr(pos + 1);

                int pos2 = name.find_last_of('.');

                if (pos2 != name.npos) name = name.substr(0, pos2);
            }

            MatrixStrategy * cs = new MatrixStrategy();

            if (cs->load_from_file(path)) {
                while (name.length() == 0) {
                    cout << "\nName to use for this strategy\n(Warning: using the name of an existing strategy will override that strategy!):\n\n";
                    read_token(name);
                    if (name == "learn") {
                        cout << "The name 'learn' is reserved.\n";
                        name = "";
                    }
                }

                ++ct;
                insert_strat_ptr(cs, name, single_mode);
            }
            else {
                cout << "Import failed. Please check if the file path is correct.\n\n";
            }

        } while (output_paths.size() > 0);

        if (ct) {
            if (single_mode || ct == 1)
                cout << ct << " strategy imported.\n" << endl;
            else
                cout << ct << " strategies imported.\n" << endl;
        }
    }

    else if (cmd == "-rm" || cmd == "remove") {

        string name;
        while (true) {
            if (extra_strats.empty()) {
                cout << "Can't remove because no strategies have been imported.\n\n";

                break;
            }

            if (!has_buf()) {
                cout << "\nName of imported strategy to remove (enter cancel to exit):\n";

                print_hline();

                cout << "Available imported strategies:\n";
                list_strats(false, true);
                print_hline();
            }

            read_token(name);

            if (name == "cancel") break;
			
            auto it = extra_strats.find(name);
			bool erase_all = (it == extra_strats.end()) && (name == "all");
			
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
					cout << "All imported strategies removed.\n";
				else
					cout << "Strategy removed.\n";

                if (builtin_strats.find(name) != builtin_strats.end()) {
                    strat[name] = builtin_strats[name]; // reset internal strategy w/ the name
                    cout << "Internal strategy '" << name << "' restored.\n\n";
                }

                print_hline();
                cout << "List of strategies:\n";
                list_strats();
                print_hline();
                break;
            }
            else {
                cout << "\nStrategy does not exist or is not an imported strategy. (Enter cancel to exit)" << endl;;
            }

        }
    }

    else if (cmd == "-c" || cmd == "clone") {
        IStrategy & s0 = ask_for_strategy("\nName of strategy to clone:");

        string name;
        while (name.length() == 0) {
            if (!has_buf())
                cout << "\nName to use for the cloned strategy\n" <<
                "(Warning: using the name of an existing strategy will override that strategy!):\n\n";

            read_token(name);

            if (name == "learn") {
                cout << "The name 'learn' is reserved.\n";
                name = "";
            }
        }

        MatrixStrategy * cs = new MatrixStrategy(s0);

        cout << "Cached copy of strategy saved to: " << name << "." << endl;

        insert_strat_ptr(cs, name, true);
    }

    // logistics
    else if (cmd == "-v" || cmd == "version") {
        cout << "Bacon console version " << VERSION <<
            "\n(c) Alex Yu 2017\n\nSwine Swap: " <<
            (enable_swine_swap ? "Enabled" : "Disabled") <<
            "\nTime Trot: " <<
            (enable_time_trot ? "Enabled" : "Disabled") << "\n" << endl;
    }

    else if (cmd == "-o" || cmd == "option") {
        string name;
        vector<string> opt = { "swap", "trot" };

        while (name.length() == 0) {
            if (!has_buf()) {
                cout << "\nName of option to adjust (choices:";
                for (unsigned i = 0; i < opt.size(); ++i) cout << " " << opt[i];
                cout << "): ";
            }

            read_token(name);

            if (find(opt.begin(), opt.end(), name) == opt.end()) {
                cout << "\nInvalid option. (choices: ";
                for (unsigned i = 0; i < opt.size(); ++i) cout << " " << opt[i];
                cout << ")\n";

                name = "";
            }
        }

        string value;
        while (value.length() == 0) {
            if (!has_buf()) {
                cout << "\nValue for option '" << name << "': (on/off): ";
            }

            read_token(value);

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
                cout << "Invalid value. Please enter on/off (lower case):";
                value = "";
            }
        }

        write_options();

        cout << "\nOption '" << name << "' set to " << value << "\n" << endl;
    }

    else if (cmd == "time") {
        string command;
        if (!has_buf()) cout << "Command to measure:";
        read_token(command);

        clock_t begin = clock();

        exec(command);

        clock_t end = clock();

        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

        if (time_spent < 0.7) {
            cout << "time: task took " << time_spent * 1000 << " ms\n";
        }
        else {
            cout << "time: task took " << time_spent << " seconds\n";
        }

        cout << endl;
    }

    else {
        show_available_commands();

        if (cmd == "-h" || cmd == "help") {
            cout << "--The Game of Hog--\n\
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
            cout << endl;
        }
    }

}

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
        stringstream st;
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
}

int main(int argc, char * argv[]) {
    signal(SIGINT, int_handler);

    // initialize
    init_console();


    if (argc <= 1) { // interactive mode

        string cmd;

#ifdef _WIN32
        // Windows only

        // change title of console, if possible
        system((string("Title ") + NAME).c_str());

        CreateDirectoryA(STORAGE_ROOT.c_str(), NULL);
#else
		// speed up cin
		cin.sync_with_stdio(0);
	
        // make the storage directory, if possible
        system((string("mkdir -p ") + STORAGE_ROOT).c_str());
#endif    

        // Print greeting message
        cout << "Welcome to Bacon Console v." << VERSION << "!\n(c) Alex Yu 2017" << endl;

        // Enter console mode
        console_mode = true;
		
        // Main loop
        do {
            exec(cmd);
            cout << PROMPT;
        } while (cin >> cmd && cmd != "exit");
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
        catch (exception ex) {
        }
    }

    return 0;
}
