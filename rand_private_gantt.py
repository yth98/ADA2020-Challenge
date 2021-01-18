from sys import argv
import random
import bisect
import numpy as np
import matplotlib.pyplot as plt
ax=plt.gca()
[ax.spines[i].set_visible(False) for i in ["top","right"]]

def constrained_sum_sample_pos(n, total):
    """Return a randomly chosen list of n positive integers summing to total.
    Each such list is equally likely to occur."""
    dividers = sorted(random.sample(range(1, total), n - 1))
    return [a - b for a, b in zip(dividers + [total], [0] + dividers)]

# https://stackoverflow.com/a/3590105
def constrained_sum_sample_nonneg(n, total):
    """Return a randomly chosen list of n nonnegative integers summing to total.
    Each such list is equally likely to occur."""
    return [x - 1 for x in constrained_sum_sample_pos(n, total + n)]

l = 5

Slice = [list() for _ in range(l)]
Slice_back = [0 for _ in range(l)]
Slice_Ops = list(map(lambda x: x+6, constrained_sum_sample_nonneg(l, random.randint(0, 6))))
print(Slice_Ops)
Operations = []
for q in range(l):
    for _ in range(Slice_Ops[q]):
        Slice_back[q] += random.randint(0, 12) # spacing
        Slice[q].append(Slice_back[q]) # Start
        Slice_back[q] += [53, 59, 61, 67, 71, 73, 79, 83, 89][random.randint(0, 8)]
        Slice[q].append(Slice_back[q]) # End

for _ in range(10):
    my_slice, neighbor_slice = random.sample(range(l), 2)
    my_idx = random.randint(0,len(Slice[my_slice])//2-1)*2
    my_start = Slice[my_slice][my_idx]
    neighbor_idx = bisect.bisect_right(Slice[neighbor_slice], my_start)
    if neighbor_idx % 2 != 0 or neighbor_idx >= len(Slice[neighbor_slice]): continue
    neighbor_start = Slice[neighbor_slice][neighbor_idx]
    if Slice[my_slice][my_idx+1] - neighbor_start >= 96: continue
    if neighbor_idx + 2 < len(Slice[neighbor_slice]) and Slice[neighbor_slice][neighbor_idx+2] < Slice[my_slice][my_idx+1]: continue
    print(Slice[neighbor_slice][neighbor_idx], Slice[neighbor_slice][neighbor_idx+1], Slice[my_slice][my_idx], Slice[my_slice][my_idx+1], my_slice, neighbor_slice)
    Slice[neighbor_slice][neighbor_idx+1] = Slice[my_slice][my_idx+1]
    Slice[my_slice][my_idx] = neighbor_start
    print(Slice[neighbor_slice][neighbor_idx], Slice[neighbor_slice][neighbor_idx+1], Slice[my_slice][my_idx], Slice[my_slice][my_idx+1])

for q in range(l):
    for i in range(Slice_Ops[q]):
        O = dict()
        O['y'] = [q+1]
        O['x'] = Slice[q][i*2]
        O['d'] = Slice[q][i*2+1] - Slice[q][i*2]
        Operations.append(O)

random.shuffle(Operations)

for O in Operations:
    Color = '#'+''.join(['0123456789abcdef'[np.random.randint(0,15)] for _ in range(6)])
    for y in O['y']:
        plt.barh(y-1,O['d'],left=O['x'],color=Color,height=0.95)
        plt.text(O['x']+O['d']/8,y-1,'D%s'%(O['d']),color="white",size=8)
plt.yticks(np.arange(l),np.arange(1,l+1))
plt.subplots_adjust(left=0.02, right=0.98)
plt.show()
