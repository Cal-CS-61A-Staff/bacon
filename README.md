## Overview


### What does Bacon do?

Bacon is an an analysis program for Hog, a dice game from the 
CS61A class project [Hog](https://cs61a.org/proj/hog/). Bacon was created for the [Hog Contest](https://cs61a.org/proj/hog_contest/), which is a Hog strategy contest students are encouraged to participate in.

Bacon may be used by students to construct and test strategies, or by instructors to run contests. The system is designed to be highly efficient and a vast improvement over the old Hog Contest calculations.

The core portion of Bacon is written entirely in C++ and is highly optimized. 
On average, the computation of the exact theoretical winrate between two strategies takes approximately 100 milliseconds.

Moreover, the tournament procedure is multithreaded and may be split up to run on an arbitrary number of threads. A test tournament with 132 random strategies (8646 games) finished in less than **11.5 minutes**, a lot faster than the old contest system, which is taking almost a week to compute the results of a tournament with 120 strategies.

### Components
This project has two main components: `bacon` and `hogconv[.py]`.

`bacon` is the main binary, used for computing exact win rates, analyzing strategies, running tournaments etc.

`hogconv.py` is a Python script that converts Hog strategies written in Python (containing final_strategy and TEAM_NAME as specified in the [Hog Contest](https://cs61a.org/proj/hog_contest/)) to `.strat` files which `bacon` can understand.

## Installation

### Linux / Mac OS X / etc.

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

If you are using a computer where you have no root/sudo access, use the following instead to install to `$HOME/bin`:
```sh
make install_user
```

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

Then just like with Linux/Mac, `cd` into the `bacon` directory and enter `make`. 
Do not use `make install` however. Instead, simply copy the `bacon.exe` file inside `bin/` and `hogconv.py` inside the root directory to somewhere convenient and run them.


## bacon: Usage


### Usage from System Shell

Bacon may be used directly from the system shell by directly passing arguments to the `bacon` binary.

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

#### Other cool things you can do:

Draw a diagram of a strategy (must use appropriate monospaced font):
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

You can also measure the runtime of any command using `time`:
```sh
bacon time -t threads -f output_file
```

### Bacon Interactive Shell
Another way to use Bacon is through the interactive shell. You may start the interactive shell by simply typing `bacon`, without any arguments:
```sh
bacon
```

This shell works sort of like Python's, except obviously a lot simpler. You may enter any bacon command and receive a response.

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

### List of All the Commands

You can get a list of commands just like the one below by entering `bacon -h` in the shell or typing `help` into the Bacon console. The parts in brackets (`-p`, `-t`, etc.) are the shorthands for the commands. Some commands have no shorthands.

#### The Game of Hog

|  Command	 |  Description    |
|  -------------  |  -------------  |
| play (-p): |  simulate a game of Hog between two strategies (or play against one of them). |
| tournament (-t): |  run a tournament with all the imported strategies. Use the -f switch to specify output file path: |  bacon -t -f output.txt |

#### Learning

|  Command	 |  Description    |
|  -------------  |  -------------  |
| train (-l): |  start training against a specified strategy (improves the 'learn' strategy). |
| learnfrom(-lf): |  sets the 'learn' strategy to a copy of the specified strategy. The 'train' command will now train this new strategy. |

#### Strategic Analysis

|  Command	 |  Description    |
|  -------------  |  -------------  |
| winrate (-r): |  get the theoretical win rate of a strategy against another one. |
| avgwinrate: |  get the average win rate of a strategy against another one using sampling. |
| winrate0 (-r0), winrate1 (-r1), avgwinrate0, avgwinrate1: |  force the first strategy to play as player #. |
| mkfinal: |  re-compute the 'final' strategy; saves the result to the specified strategy name. |
| mkrandom: |  creates a randomized strategy and saves the result to the specified strategy name. |
| get (-s): |  see what a given strategy would roll at a given set of scores. |
| diff (-d): |  get the differences in between two strategies. |
| graph (-g): |  get a graphic representation of a strategy. |
| graphdiff (-gd): |  get a graphic representation of the differences between two strategies. |

#### Strategy Manager

|  Command	 |  Description    |
|  -------------  |  -------------  |
| list (-ls): |  show a list of available strategies.  |
| import (-i): |  add a new strategy from a file. Use the -f switch to specify import file path(s): |  bacon -i mystrategy -f a.strat |
| export (-e): |  export a strategy to a file. Use the -f switch parameter to specify output file path: |  bacon -o final -f final.strat  |
| exportpy: |  export a strategy to a Python script that defines a function called 'strategy'. |
| clone (-c): |  clones an existing strategy and saves a cached copy of it to a new name. |
| remove (-r): |  remove an imported strategy and restore an internal strategy, if available. Enter 'remove all' or '-r all' to clear all imported strategies. |

#### Logistics

|  Command	 |  Description    |
|  -------------  |  -------------  |
| help (-h): |  display this help. |
| version (-v): |  display the version number. |
| option (-o): |  adjust options (turn on/off Swine Swap, Time Trot). |
| time: |  measure the runtime of any bacon command. |
| exit: |  get out of here!	

## hogconv: Usage

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
