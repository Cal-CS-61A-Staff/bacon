import os, sys, importlib.util

GOAL = 100
FUNCTION_NAME = 'final_strategy'
TEAM_NAME_VAR = 'TEAM_NAME' # the variable that decides the output name of the function

count, out_dir, out_sw = 0, '', False

output_names = {}

def convert(file):
	module_path = file[:-3]
	
	# import module
	module_name = os.path.basename(module_path)
	
	spec = importlib.util.spec_from_file_location(module_name, file)
	module = importlib.util.module_from_spec(spec)
	spec.loader.exec_module(module)

	strat = getattr(module, FUNCTION_NAME)
	
	try:
		output_name = getattr(module, TEAM_NAME_VAR)
	except:
		print("WARNING: ", file, "does not specify", TEAM_NAME_VAR + ". Using module name as team name...")
		output_name = module_name
	
	# avoid duplication
	if output_name in output_names:
		full_output_name = output_name + "__" + str(output_names[output_name]) + '.strat'
		output_names[output_name] += 1
		print("WARNING: found multiple teams with name", output_name + ". Writing to output file",
			   full_output_name, "instead to disambiguate...")
		
	else:
		output_names[output_name] = 1
		full_output_name = output_name + '.strat'

	os.makedirs(out_dir, exist_ok = True)
	out = open(os.path.join(out_dir, full_output_name), 'w')
	
	for i in range(GOAL):
		for j in range(GOAL):
			if j: out.write(' ')
			out.write(str(strat(i, j)))
		out.write('\n')
	
	out.flush()
	out.close()
	
	print ("converted:", file)
	
	return 1
	

def convert_dir(dir = os.path.dirname(__file__)):
	count = 0	
	
	for file in os.listdir(dir or None):
		if file == '__init__.py' or file == __file__ or file[-3:] != '.py': continue
		count += convert(file, count)
	
	return count
		
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
		print ("can't access", path + ", skipping...")	
	
if len(sys.argv) <= 1:
	print ("""\nusage: python3 cache.py [-o output_dir] [file1] [file2]\n
Converts each python strategy file to a strategy file with extension .strat.
Saves resulting files to the current directory by default. Use -o to specify a different directory.\n""")
	
else:
	print ("\nconverted a total of", count, ("strategies." if count != 1 else "strategy."))	
		
	
