#!/bin/python3

import sys, datetime

STAFF_SOLN_NAME = "&lt;staff solution&gt;"

if len(sys.argv) < 4:
    print ("usage: python3 hoghtml.py [template_file] [bacon_results_file] [output_file]")
    sys.exit(0)

templatef = open(sys.argv[1], "r", encoding="UTF-8")
resultsf = open(sys.argv[2], "r", encoding="UTF-8")

html = templatef.read()
results = filter(None, resultsf.read().split('\n'))
ol, teams, win_rates, team_names = [], [], [], []
staff = None

win_rates_mode = False
cnt = 0

for s in results:
    if s.strip() == "Win rates:":
        win_rates_mode = True
    elif win_rates_mode:
        win_rates.append(list(map(float, s.split(','))))
    elif '.' in s:
        st = s.strip().replace('&','&amp;').replace('<','&lt;').replace('>','&gt;')
        dot_idx = st.index('. ')
        with_idx = st.rindex(' with')
        wins_idx = st.rindex(' wins')
        team_name = st[dot_idx+2:with_idx].strip()
        wins = int(st[with_idx + len(' wins') : wins_idx])
        team_names.append(team_name)

        if team_name != STAFF_SOLN_NAME:
            teams.append((wins, team_name, cnt))
        else:
            staff = (wins, team_name, cnt)
        cnt += 1
        

teams = list(reversed(sorted(teams)))
rank, equiv = 0, 0
for i in range(len(teams)):
    wins, team_name, index = teams[i]
        
    if staff and wins <= staff[0]:
        ol.append("<li id=\"team-" + str(staff[2]) + "\" class=\"rank rank-staff\">-. <strong>" + staff[1] + "</strong> with " + str(staff[0]) + " wins</li>")
        staff = None
        
    if i > 0 and teams[i-1][0] > wins:
        rank += equiv
        equiv = 0
    equiv += 1
        
    classes = ["rank", "rank-" + str(rank)]
    ol.append("<li id=\"team-" + str(index) + "\" class=\"" + " ".join(classes) + "\">" + 
        str(rank+1) + ". <strong>" + team_name + "</strong> with " + str(wins) + " wins</li>")
        

html = html.replace("{%RANKINGS%}", "\n".join(ol))
html = html.replace("{%TIMESTAMP%}", str(datetime.datetime.now()))
html = html.replace("{%TEAMS%}", str(team_names))
html = html.replace("{%WINRATE_MATRIX%}", str(win_rates))

out = open(sys.argv[3], "w", encoding="UTF-8")

out.write(html)
out.close()
