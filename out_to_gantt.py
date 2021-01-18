from sys import argv
import numpy as np
import matplotlib.pyplot as plt
ax=plt.gca()
[ax.spines[i].set_visible(False) for i in ["top","right"]]

if len(argv) < 3: exit("<in> <out>")

with open(argv[1], "r") as In:
    dGCD = 4
    l = int(In.readline())
    n = int(In.readline())
    I = []
    for i in range(n):
        J = dict()
        J['m'] = int(In.readline())
        J['w'] = In.readline()
        J['O'] = [dict() for _ in range(J['m'])]
        for j in range(J['m']):
            line = list(map(int,In.readline().split()))
            J['O'][j]['s'] = line[0]
            J['O'][j]['d'] = line[1]
            J['O'][j]['y'] = [0 for _ in range(line[0])]
        I.append(J)
    with open(argv[2], "r") as Out:
        for J in I:
            for O in J['O']:
                line = list(map(int,Out.readline().split()))
                O['x'] = line[0]
                O['y'] = line[1:]
                assert(len(O['y']) == O['s'])

def gatt(I):
    # 10分钟用Python或MATLAB制作漂亮的甘特图（Gantt）
    # https://my.oschina.net/u/4131402/blog/4373110
    for i in range(n):
        J = I[i]
        for j in range(len(J['O'])):
            O = J['O'][j]
            Color = '#'+''.join(['0123456789abcdef'[np.random.randint(0,15)] for _ in range(6)])
            for y in O['y']:
                plt.barh(y-1,O['d'],left=O['x'],color=Color,height=0.95)
                plt.text(O['x']+O['d']/8,y-1,'J%s O%s\nD%s'%(i+1,j+1,O['d']),color="white",size=8)

if __name__=="__main__":
    gatt(I)
    plt.yticks(np.arange(l),np.arange(1,l+1))
    plt.subplots_adjust(left=0.02, right=0.98)
    plt.show()
