#!/bin/python3
from __future__ import print_function
from functools import wraps
import os, sys, imp, random, string, re, errno, threading, time, queue

GOAL = 100 # goal score for Hog

STRATEGY_FUNC_ATTR = 'final_strategy' # attribute of student's module that contains the strategy function
TEAM_NAME_ATTRS = ['PLAYER_NAME', 'TEAM_NAME'] # allowed attributes of student's module that contains the team name

TEAM_NAME_MAX_LEN = 100 # max length for team names (set to 0 to remove limit)
DEF_EMPTY_TEAM_NAME = "<no name given, email starts with {}>" # name for teams with an empty team name

MIN_ROLLS, MAX_ROLLS = 0, 10 # min, max roll numbers
ERROR_DEFAULT_ROLL = 5 # default roll in case of invalid strategy function (will report error)

TIMEOUT_SECONDS = 45 # max time a student's submission should run

count, out_dir, out_sw = 0, '', False

# dict of names, used to check for duplicate team names
output_names = {}

empty_name_teams = []

# print to stderr
def eprint(*args, **kwargs):
    """ print to stderr """
    print(*args, file=sys.stderr, **kwargs)

class Worker(threading.Thread):
    def __init__ (self, func, args, kwargs, q):
        threading.Thread.__init__ (self)
        self.func = func
        self.args = args
        self.kwargs = kwargs
        self.q = q
        self.setDaemon (True)
    def run (self):
        self.q.put((True, self.func(*self.args, **self.kwargs)))
    
class Timer(threading.Thread):
    def __init__ (self, timeout, error_message, worker, q):
        threading.Thread.__init__ (self)
        self.timeout = timeout
        self.error_message = error_message
        self.worker = worker
        self.q = q
        self.setDaemon (True)
    def run (self):
        time.sleep(self.timeout)
        self.q.put((False, None))
       
def timeout(seconds=10, error_message=os.strerror(errno.ETIME)):
    """ makes function error if ran for too long """
    def decorator(func):
        def wrapper(*args, **kwargs):
            q = queue.Queue()
            worker = Worker(func, args, kwargs, q)
            timer = Timer(seconds, error_message, worker, q)
            worker.start()
            timer.start()
            code, result = q.get()
            if worker.isAlive():
                del worker
            if timer.isAlive():
                del timer
            if code:
                return result
            else:
                print("ERROR: Conversion timed out (> {} s) for: ".format(TIMEOUT_SECONDS) + args[0])

        return wraps(func)(wrapper)

    return decorator

@timeout(TIMEOUT_SECONDS)
def convert(file):  
    """ convert a single file """
    import shutil, os, sys
    sys.setrecursionlimit(200000)
    module_path = file[:-3] # cut off .py
    
    module_dir, module_name = os.path.split(module_path)
    
    # make sure module's dependencies work
    sys.path[-1] = module_dir
    sys.path.append(os.path.dirname(os.path.realpath(__file__)))
    
    # import module
    try:
        module = imp.load_source(module_name, file)
        # try to prevent use of dangerous libraries, although not going to be possible really
        module.subprocess = module.shutil = "trolled"
        if hasattr(module, "os"):
            module.os.rmdir = module.os.remove = "trolled"
        
    except Exception as e:
        # report errors while importing
        eprint ("\nERROR: error occurred while loading " + file + ":")
        eprint (type(e).__name__ + ': ' + str(e))
        eprint ("skipping...\n")
        return
    if hasattr(module, STRATEGY_FUNC_ATTR):
        strat = getattr(module, STRATEGY_FUNC_ATTR)
    else:
        eprint ("ERROR: " + file + " has no attribute " + STRATEGY_FUNC_ATTR + " , skipping...")
        del module
        return
    
    output_name = ""
    for attr in TEAM_NAME_ATTRS:
        if hasattr(module, attr):
            val = str(getattr(module, attr))
            if val:
                output_name = getattr(module, attr)
            setattr(module, attr, "")
            
    if not output_name:
        eprint ("WARNING: submission " + file + " has no team name. Using default name...")
        module_dir_name = ""
        if '/' in module_dir:
            module_dir_name = module_dir[module_dir.index('/')+1:]
        elif '\\' in module_dir:
            module_dir_name = module_dir[module_dir.index('\\')+1:]
        if not module_dir_name: module_dir_name = module_name
        output_name = DEF_EMPTY_TEAM_NAME.format(module_dir_name[0])
        empty_name_teams.append(module_dir_name)
    
    # check for team names that are too long
    if len(output_name) > TEAM_NAME_MAX_LEN and TEAM_NAME_MAX_LEN > 0:
        eprint ("WARNING: " + file + " has a team name longer than " + str(TEAM_NAME_MAX_LEN) + 
               " chars. Truncating...")
        output_name = output_name[:TEAM_NAME_MAX_LEN-3] + "..."
    
    # check for duplicate team names
    strat_name = output_name
    try:
        output_name = output_name.encode('ascii','ignore').decode('ascii')
        output_name = re.sub(r"[\\/:*?""<>|\r\n]", "", output_name)
    except:
        output_name = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(12))
    if output_name in output_names:
        full_output_name = output_name + "_" + str(output_names[output_name]) + '.strat'
        strat_name += "_" + str(output_names[output_name]) 
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
    
    out = open(os.path.join(out_dir, full_output_name), 'w', encoding='utf-8')
    
    # write out new strategy
    
    out.write('strategy ' + strat_name + '\n')
    nerror = 0
    errname = ""
    
    for i in range(GOAL):
        for j in range(GOAL):
            if j: out.write(' ')
            try:
                rolls = strat(i, j)
            
                # check if output valid
                if type(rolls) != int or rolls < MIN_ROLLS or rolls > MAX_ROLLS:
                    if type(rolls) != int:
                        eprint("WARNING: team", output_name + "'s strategy function outputted something other than a number!")
                    else:
                        eprint("WARNING: team", output_name + "'s strategy function outputted an invalid number of rolls:", rolls + "!")
                        
                    eprint("Due to invalid strategy function, all rolls for team", output_name , "will default to 4. Please notify the students!")
                    
                    rolls = ERROR_DEFAULT_ROLL
            
                out.write(str(rolls))
            except Exception as e:
                # report errors while running strategy
                nerror += 1
                errname = type(e).__name__ + " " + str(e)
                out.write(str(ERROR_DEFAULT_ROLL))
        out.write('\n')
        
    if nerror:
        eprint ("\nERROR: " + str(nerror) + " error(s) occurred while running " + STRATEGY_FUNC_ATTR + ' for ' + output_name + '(' + file + "):") 
        eprint (errname)
    
    out.flush()
    out.close()
    
    print (">> converted: " + strat_name + " (" + file + ")")
    
    del module
    global count
    count += 1 # counting how many converted
    

def convert_dir(dir = os.path.dirname(__file__)):
    """ convert all files in a directory (does not recurse) """
    for file in os.listdir(dir or None):
        path = os.path.join(dir, file)
        if os.path.isdir(path):
            convert_dir(path)
        elif file == '__init__.py' or file == __file__ or file[-3:] != '.py': continue
        else: convert(path)
        
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
            convert_dir(path)
        else:
            convert(path)
                
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

if empty_name_teams:
    print("WARNING: some teams " + str(empty_name_teams) + " did not specify team names in their submissions!")
