#!/bin/python3
from __future__ import print_function
import os, sys, imp

GOAL = 100 # goal score for Hog

STRATEGY_FUNC_ATTR = 'final_strategy' # attribute of student's module that contains the strategy function
TEAM_NAME_ATTRS = ['PLAYER_NAME', 'TEAM_NAME'] # attributes for the team name (should really be "PLAYER_NAME" but keeping "TEAM_NAME" for compatibility)

MIN_ROLLS, MAX_ROLLS = 0, 10 # min, max roll numbers
ERROR_DEFAULT_ROLL = 5 # default roll in case of invalid strategy function (will tell you if this happens somehow)

# output file format
OUTPUT_FILE_PREFIX = "PLAYER_NAME = '{}'\ndef final_strategy(score, opponent_score):\n    return [" # outputted before matrix of roll numbers
OUTPUT_FILE_SUFFIX = "][score][opponent_score]\n" # outputted after matrix

def eprint(*args, **kwargs):
    """ print to stderr """
    print(*args, file=sys.stderr, **kwargs)
	
def convert(file, out_path):  
    """ convert a single file to matrix form """
    module_path = file[:-3] # cut off .py
    module_dir, module_name = os.path.split(module_path)
    
    # make sure module's dependencies work by appending to path
    sys.path[-1] = module_dir
    sys.path.append(os.path.dirname(os.path.realpath(__file__)))
    
    # import module
    try:
        module = imp.load_source(module_name, file)
    except Exception as e:
        # report any errors encountered while importing
        eprint ("\nERROR: error occurred while loading " + file + ":")
        eprint (type(e).__name__ + ': ' + str(e))
        import traceback
        traceback.print_stack() # try to be helpful
        sys.exit(1)
    
    if hasattr(module, STRATEGY_FUNC_ATTR):
        strat = getattr(module, STRATEGY_FUNC_ATTR)
    else:
        eprint ("ERROR: the python script you selected " + file + " has no required function '" + STRATEGY_FUNC_ATTR + "'!")
        sys.exit(2)
    
    strat_name = ""
    for attr in TEAM_NAME_ATTRS:
        if hasattr(module, attr):
            val = getattr(module, attr)
            if val:
                strat_name = getattr(module, attr)
            setattr(module, attr, "")
            
    if not strat_name:
        eprint ("ERROR: you have not specified a player name! Please fill in the PLAYER_NAME='' filed at the top of the hog_contest.py.")
        sys.exit(3)
    
    # check for team names that are too long
    out = open(out_path, 'w', encoding='utf-8')
    
    # write out new strategy
    try:
        skip_rest = False
        out.write(OUTPUT_FILE_PREFIX.format(strat_name))
        
        for i in range(GOAL):
            if i:
                out.write(',\n')
            out.write('[')
            for j in range(GOAL):
                if j:
                    out.write(', ')
                if not skip_rest:
                    rolls = strat(i, j)
                
                    # check if output valid
                    if type(rolls) != int or rolls < MIN_ROLLS or rolls > MAX_ROLLS:
                        if type(rolls) != int:
                            eprint("WARNING: for (score, opponent_score) = {} your strategy function outputted something other than a number! Pretending you roll 5...".format((i,j)))
                        else:
                            eprint("WARNING: for (score, opponent_score) = {} your strategy function outputted an invalid number of rolls:",
                                rolls + "! Pretending you roll 5...".format((i,j)))
                        rolls = ERROR_DEFAULT_ROLL
                
                out.write(str(rolls))
            out.write(']')
        out.write(OUTPUT_FILE_SUFFIX)
            
    except Exception as e:
        # report errors encountered while running strategy
        eprint ("\nERROR: error occurred while running '" + STRATEGY_FUNC_ATTR + "' in " + file + ":") 
        eprint (type(e).__name__ + ': ' + str(e))
        import traceback
        traceback.print_stack() # try to be helpful
        sys.exit(4)
    out.close()
    
sys.path.append('')
if len(sys.argv) != 3:
    print ("""hogmat.py - A tool for "streamlining" hog contest strategies.
Created by FA2018 CS61A course staff / Alex Yu

usage: python3 hogmat.py hog_contest.py output.py
where hog_contest.py is your original hog_contest.py file, and output.py is the python file to output to

This script will run your final_strategy function for every possible pair of inputs and output a "cached" version of the function that does not depend on any external files.""")
    sys.exit(0)

in_file, out_file = sys.argv[1:]
convert(in_file, out_file)
   
print ("Conversion success :) output saved to: " + out_file)
print ("You should now be able to submit " + out_file + " directly to okpy. GL HF!")
