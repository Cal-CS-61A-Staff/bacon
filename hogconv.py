#!/usr/bin/python3
from __future__ import print_function
import os, sys, imp

GOAL = 100 # goal score for Hog

STRATEGY_FUNC_ATTR = 'final_strategy' # attribute of student's module that contains the strategy function
TEAM_NAME_ATTR = 'TEAM_NAME' # attribute of student's module that contains the team name

TEAM_NAME_MAX_LEN = 100 # max length for team names (set to 0 to remove limit)
DEF_EMPTY_TEAM_NAME = "(empty string)" # name for teams with an empty team name
DEF_LONG_TEAM_NAME = "(team name longer than 50 chars)" # name for teams with team names that are too long

MIN_ROLLS, MAX_ROLLS = 0, 10 # min, max roll numbers
ERROR_DEFAULT_ROLL = 4 # default roll in case of invalid strategy function (will report error)

count, out_dir, out_sw = 0, '', False

# dict of names, used to check for duplicate team names
output_names = {}

# print to stderr
def eprint(*args, **kwargs):
    """ print to stderr """
    print(*args, file=sys.stderr, **kwargs)
	
def convert(file):  
    """ convert a single file """
    module_path = file[:-3] # cut off .py
    
    module_dir, module_name = os.path.split(module_path)
    
    # make sure module's dependencies work
    sys.path[-1] = module_dir
    
    # import module 
    try:
        module = imp.load_source(module_name, file)
    except Exception as e:
        # report errors while importing
        eprint ("\nERROR: error occurred while loading " + file + ":")
        eprint (type(e).__name__ + ': ' + str(e))
        eprint ("skipping...\n")
        
        return 0
    
    try:
        strat = getattr(module, STRATEGY_FUNC_ATTR)
    except:
        eprint ("ERROR: " + file + " has no attribute " + STRATEGY_FUNC_ATTR + " , skipping...")
        return 0
    
    try:
        output_name = getattr(module, TEAM_NAME_ATTR)
    except:
        eprint ("WARNING: " + file + " does not specify " + TEAM_NAME_ATTR + ". Using module name as team name...")
        output_name = module_name
    
    # check for empty team names
    if not output_name:
        eprint ("WARNING: " + file + " has an empty team name. Setting team name to " + DEF_EMPTY_TEAM_NAME + "...")
        output_name = DEF_EMPTY_TEAM_NAME
        
    # check for team names that are too long
    if len(output_name) > TEAM_NAME_MAX_LEN and TEAM_NAME_MAX_LEN > 0:
        eprint ("WARNING: " + file + " has a team name longer than " + TEAM_NAME_MAX_LEN + 
               " chars. Setting team name to " + DEF_LONG_TEAM_NAME + "...")
        output_name = DEF_LONG_TEAM_NAME
    
    # check for duplicate team names
    if output_name in output_names:
        full_output_name = output_name + "__" + str(output_names[output_name]) + '.strat'
        output_names[output_name] += 1
        eprint("WARNING: found multiple teams with name", output_name + ". Writing to output file",
               full_output_name, "instead to disambiguate...")
        
    else:
        output_names[output_name] = 1
        full_output_name = output_name + '.strat'

    # make sure output directories exist
    if out_dir: 
        try:
            os.makedirs(out_dir)
        except:
            pass
    
    out = open(os.path.join(out_dir, full_output_name), 'w')
    
    # write out new strategy
    
    try:
        skip_rest = False
        
        for i in range(GOAL):
            for j in range(GOAL):
                if j: out.write(' ')
                
                if not skip_rest:
                    rolls = strat(i, j)
                
                    # check if output valid
                    if type(rolls) != int or rolls < MIN_ROLLS or rolls > MAX_ROLLS:
                        if type(rolls) != int:
                            eprint("WARNING: team", output_name + "'s strategy function outputted something other than a number!")
                        elif type(rolls) != int:
                            eprint("WARNING: team", output_name + "'s strategy function outputted an invalid number of rolls:", rolls + "!")
                            
                        eprint("Due to invalid strategy function, all rolls for team", output_name , "will default to 4. Please notify the students!")
                        
                        rolls = ERROR_DEFAULT_ROLL
                        skip_rest = True
                
                out.write(str(rolls))
            out.write('\n')
            
    except Exception as e:
        # report errors while running strategy
        eprint ("\nERROR: error occurred while running " + STRATEGY_FUNC_ATTR + ' in ' + file + ":") 
        eprint (type(e).__name__ + ': ' + str(e))
        eprint ("skipping...\n")
        
        return 0
    
    out.flush()
    out.close()
    
    print ("converted: " + file)
    
    return 1 # useful for counting
    

def convert_dir(dir = os.path.dirname(__file__)):
    """ convert all files in a directory (does not recurse) """
    count = 0   
    
    for file in os.listdir(dir or None):
        if file == '__init__.py' or file == __file__ or file[-3:] != '.py': continue
        count += convert(file, count)
    
    return count
        
# add an empty entry to sys.path so that we can add dependencies for each student module
sys.path.append('')

print ('')
        
for i in range(1, len(sys.argv)):
    path = sys.argv[i]
    
    if out_sw:
        out_sw = False
        out_dir = path
        continue
    
    if path == '-o':
        out_sw = True
        continue
    
    if os.path.exists(path):
        if os.path.isdir(path):
            count += convert_dir(path)
        else:
            count += convert(path)
            
    else:
        eprint ("ERROR: can't access " + path + ", skipping...")  
    
if len(sys.argv) <= 1:
    print ("""usage: python3 hogconv.py [-o output_dir] [file1] [file2] ...\n
Converts each Python Hog strategy to a .strat (space-separated matrix) file that may then be imported into Bacon.
Saves resulting files to the current directory by default. Use -o to specify a different directory.\n""")
    
else:
    print ("\nconverted a total of " + str(count) + (" strategies." if count != 1 else " strategy."))
    print ("\ntips: run 'bacon -i -f " + (out_dir + "/" if out_dir else "") + "*.strat' in bash to import the converted strategies into hog.")
    print ("in powershell: 'bacon -i -f (ls " + (out_dir + "\\" if out_dir else "") + "*.strat | % FullName)'\n")
    print ("after strategies have been imported, run 'bacon -t [num_threads] [-f output_path]' to run tournament.")
    print ("to clear all imported strategies, use 'bacon -rm all'.")
