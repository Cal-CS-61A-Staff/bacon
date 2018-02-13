## Overview


### What does Bacon do?

Bacon is an an analysis program for Hog, a dice game from the 
CS61A class project [Hog](https://cs61a.org/proj/hog/). Bacon was created for the [Hog Contest](https://cs61a.org/proj/hog_contest/), which is a Hog strategy contest students are encouraged to participate in.

Bacon may be used by students to construct and test strategies, or by instructors to run contests. The system is designed to be highly efficient and a vast improvement over the old system used to run the Hog Contest.

The core portion of Bacon is written entirely in C++ and is highly optimized. 
On average, the computation of the exact theoretical winrate between two strategies takes approximately 100 milliseconds.

Moreover, the tournament procedure is multithreaded and may be split up to run on an arbitrary number of threads. 
A test tournament with 100 random strategies (4950 games) finished in less than **1.5 minutes** when ran on 12 threads on OCF, a lot faster than the old contest system, which took almost a week to compute the results of a tournament with the same number of strategies. Another test tournament with 132 random strategies (8646 games) finished in less than **11.5 minutes** running on 2 threads.

### Components
This project has two main components: `bacon` and `hogconv[.py]`.

`bacon` is the main binary, used for computing exact win rates, analyzing strategies, running tournaments etc.

`hogconv.py` is a Python script that converts Hog strategies written in Python (containing final_strategy and TEAM_NAME as specified in the [Hog Contest](https://cs61a.org/proj/hog_contest/)) to `.strat` files which `bacon` can understand.

`contest.py` is a Python script for simulating the Hog Contest in one command.

## Installation

### Linux

First, clone this repository into a folder of your choice:
```sh
git clone https://github.com/sxyu/bacon
```

Enter the directory `bacon`:
```sh
cd bacon
```

Compile the source:
```sh
make
```

Then install:
```sh
make install
```

OPTIONAL: If you are using a computer where you have no root/sudo access, use the following instead to install to `$HOME/bin`:
```sh
make install_user
```

### Mac OS X

As above, `cd` into the `bacon` directory and enter `make`.
However, `make install` won't work for Mac.
So simply use the output binary in the bin directory: `bin/bacon`.

### Windows

#### Method 1
Download and install [Visual Studio Community](https://www.visualstudio.com/vs/community/) from Microsoft.

Clone the repository and enter the bacon direcotry:
```sh
git clone https://github.com/sxyu/bacon
```
```sh
cd bacon
```

Then open the the `bacon.sln` file from the repo with Visual Studio and change the build mode to *Release* and platform to *Win32* or *Win64* on the top toolbar as appropriate. 
Build the project by navigating to `Build > Build Solution`. The output file should be located in `bin/`.

#### Method 2
Alternatively, download and install [MinGW](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/mingw-w64-install.exe/download) and [Make](http://gnuwin32.sourceforge.net/packages/make.htm) for Windows.

Just like with Linux, `cd` into the `bacon` directory and enter `make`. 
Do not use `make install` however. Instead, simply copy the `bacon.exe` file inside the `bin/` directory and `hogconv.py` inside the root directory to somewhere convenient and run them.


## Running Simulated Contests

### Simulating Using contest.py

`cd` into the project root directory and simply run:
`python3 contest.py SUBMISSION_DIR`

Where SUBMISSION_DIR is the path to the base directory containing the student submissions.
The script will recurse to each subdirectory of SUBMISSION_DIR and look for hog_contest.py, each of which is converted.
The contest result will be available in `results.txt`.

You may optionally use `-t N` to specify the number of threads (default 4),
`-n NAME` to specify the names of the submission files (default hog_contest.py), or
`-o PATH` to specify the path to the output file (default results.txt)

### Simulating Manually:

Replace `hogconv.py` and `bin/bacon` below with the path to the script/binary on your system as appropriate.

1. Convert the students' submissions (*.py) to *.strat: 
```sh
python3 hogconv.py -o strat [student_strategies/*.py hog_contest.py foo.py]
```
 list the `hog_contest.py` files in the [] part (don't actually include the []!) according to how the student strategies are laid out.

 
2. Clear existing strategies in Bacon:
```sh
bin/bacon -rm all
```

3. Import the newly converted strategies:
```sh
bin/bacon -i -f strat/*
```

4. Simulate tournament (you can replace '4' below with any number of threads desired):
```sh
bin/bacon -t 4 -f results.txt
```
 
5. Look at output in `results.txt`

Output example:
```
1. doriath with 6 wins
2. experimental with 5 wins
3. alphahog with 4 wins
4. antidefault with 2 wins
4. antidefault_copy with 2 wins
6. swap with 1 wins
7. terrible with 0 wins
```

## bacon: Detailed Usage Guide


### Usage from System Shell

Bacon may be used from the system shell by directly passing arguments to the `bacon` binary.

For example, to compute win rate, you would type:
```sh
bacon -r strategy0 strategy1
```

Where `strategy0` and `strategy1` are the names of the strategies, for instance, try:
```sh
bacon -r final swap
```

You can also simulate a game of hog between two strategies:
```sh
bacon -p final swap
```

Or play against one of the strategies yourself using the `human` built-in strategy:
```sh
bacon -p human final
```

To obtain a list of all the strategies, use:
```sh
bacon -ls
```

#### Other cool things you can do:

Draw a diagram of a strategy (your console must use an appropriate monospaced font for this to work):
```sh
bacon -g final
```

Compare two strategies:
```sh
bacon -d final swap
```

Compare two strategies graphically:
```sh
bacon -gd final swap
```

Export a strategy:
```sh
bacon -e final -f mystrategy.strat
```

Import strategies (generated with `hogconv.py` or exported with `-e`):
```sh
bacon -i -f strategies/*.strat
```

Run a tournament between all imported strategies:
```sh
bacon -t threads -f output_file
```
Where `threads` is the number of threads to use, and `output_file` is a file to write out the final rankings to.
To stop the tournament before it finishes, simply press `ctrl + C`.

You can also measure the runtime of any command using `time`:
```sh
bacon time -t threads -f output_file
```

### Bacon Interactive Shell
Another way to use Bacon is through the interactive shell. You may start the interactive shell by simply typing `bacon`, without any arguments:
```sh
bacon
```

You may enter any bacon command here and receive an immediate response.

For example, to calculate win rate, use the command `winrate`:
```sh
winrate final always4
```

Note that the command line flags `-p`, `-t`, etc. are actually implemented as shorthands for the longer Bacon commands.
For example, the command to calculate win rate, `winrate`, has the shorthand `-r`. In the system shell, you may also use the longer form if you wish:
```sh
bacon winrate final always4
```

Further, you do not really need to enter all the arguments for a command into the console at once. You may for example simply enter `winrate`, and Bacon will prompt you for the other required arguments automatically:

```
winrate

Player 0 strategy name:
Player 1 stratey name:
```

### List of Commands

You can get a list of commands just like the one below by entering `bacon -h` in the shell or typing `help` into the Bacon console. The parts in brackets (`-p`, `-t`, etc.) are the shorthands for the commands. Some commands have no shorthands.

#### Hog

|  Command	 |  Description    |
|  -------------  |  -------------  |
| play (-p) |  simulate a game of Hog between two strategies (or play against one of them). |
| tournament (-t) |  run a tournament with all the imported strategies. Use the -f switch to specify output file path |  bacon -t -f output.txt |

#### Strategy Training

|  Command	 |  Description    |
|  -------------  |  -------------  |
| train (-l) |  start training against a specified strategy (improves the 'learn' strategy). |
| learnfrom(-lf) |  sets the 'learn' strategy to a copy of the specified strategy. The 'train' command will now train this new strategy. |

#### Strategic Analysis

|  Command	 |  Description    |
|  -------------  |  -------------  |
| winrate (-r) |  get the theoretical win rate of a strategy against another one. |
| avgwinrate |  get the average win rate of a strategy against another one using sampling. |
| winrate0 (-r0), winrate1 (-r1), avgwinrate0, avgwinrate1 |  force the first strategy to play as player #. |
| mkfinal |  re-compute the 'final' strategy; saves the result to the specified strategy name. |
| mkrandom |  creates a randomized strategy and saves the result to the specified strategy name. |
| get (-s) |  see what a given strategy would roll at a given set of scores. |
| diff (-d) |  get the differences in between two strategies. |
| graph (-g) |  get a graphic representation of a strategy. |
| graphdiff (-gd) |  get a graphic representation of the differences between two strategies. |

#### Strategy Manager

|  Command	 |  Description    |
|  -------------  |  -------------  |
| list (-ls) |  show a list of available strategies.  |
| import (-i) |  add a new strategy from a file. Use the -f switch to specify import file path(s) |  bacon -i mystrategy -f a.strat |
| export (-e) |  export a strategy to a file. Use the -f switch parameter to specify output file path |  bacon -o final -f final.strat  |
| exportpy |  export a strategy to a Python script that defines a function called 'strategy'. |
| clone (-c) |  clones an existing strategy and saves a cached copy of it to a new name. |
| remove (-rm) |  remove an imported strategy and restore an internal strategy, if available. Enter 'remove all' or '-rm all' to clear all imported strategies. |

#### Logistics

|  Command	 |  Description    |
|  -------------  |  -------------  |
| help (-h) |  display this help. |
| version (-v) |  display the version number. |
| option (-o) |  adjust options (turn on/off Swine Swap, Time Trot). |
| time |  measure the runtime of any bacon command. |
| exit |  exit the program

### List of built-in strategies

`human`: Asks you what to roll at each round. Use this to play against your strategies for fun.

`default`: Always rolls 4. The baseline strategy.

`random`: Rolls a random number of dice at each round (not legal in contest, but useful for testing). You may create a random, but fixed, strategy for analysis using `mkrandom`.

`swap`: The 'swap' strategy. Students implemented this as part of the Hog project. Applies Free Bacon and Swine Swap where beneficial.

`final`: The 'final' strategy calculated using DP. This was my initial submission in the Hog Contest (NOTE: just to be safe, I have sabotaged this a bit so it's not that good; also, it's different from my winning submission.) Useful as a benchmark. Note that this is not technically a built-in strategy and may be removed. To recompute it, use `mkfinal`.

`always0` ... `always10`: Always rolls n dice.

`swap_visual`: Not a real strategy, but may be used with `-g`: `bacon -g swap_visual` to visualize the density of scores qualifying for Swine Swap across the universe of all scores.

`learn`: A special strategy which will learn and improve through training. Run the `train` command to train this strategy against another strategy. You may also run `learnfrom` to override `learn` with another strategy from which training will start.

## hogconv: Detailed Usage Guide

Hogconv is a simple Python script to help instructors to convert Python-based strategies into space-separated matrices that Bacon can understand.

You may use Python to run the script (note that the script is also compatible with Python 2.7, but students' submissions may not be):
```sh
python3 hogconv.py
```
If you installed Bacon using  `make install`, you may simply type:
```sh
hogconv
```

I will be using `hogconv` to represent both options in the example below.

To convert, simply pass a list of Python strategies to `hogconv`:
```sh
hogconv student_strategies/*.py other_strategy.py
```

`hogconv` will automatically detect any duplicate team names and any Python files with no TEAM_NAME specified.

By default, `hogconv` will write out the `.strat` files to the current directory. If you wish to change the output directory, use the `-o` switch before the strategy file names:

```sh
hogconv -o ouput_dir student_strategies/*.py other_strategy.py
```


## License

Licensed under the Apache License, Version 2.0.
