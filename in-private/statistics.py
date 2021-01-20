from os import listdir

T = []

for f in listdir():
    if f[-3:] != ".in": continue
    with open(f, "r") as In:
        l = int(In.readline())
        n = int(In.readline())
        O = 0
        I = []
        for i in range(n):
            J = dict()
            J['m'] = int(In.readline())
            print(J['m'])
            J['w'] = In.readline()
            J['O'] = [dict() for _ in range(J['m'])]
            for j in range(J['m']):
                line = list(map(int,In.readline().split()))
                J['O'][j]['s'] = line[0]
                J['O'][j]['d'] = line[1]
                J['O'][j]['y'] = [0 for _ in range(line[0])]
            O += J['m']
            I.append(J)
    T.append({'f': f, 'l': l, 'n': n, 'O': O})

T.sort(key=lambda t: (t['l'], t['O'], 100-t['n'], t['f'][:4]))
for t in T: print(t['f'], t['l'], t['n'], t['O'], sep="\t")
