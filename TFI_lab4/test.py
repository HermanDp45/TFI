termials = ['a', 'b']
nonterminals = ['S']
grammar = [('S',['S', 'S']), ('S',['a', 'b', 'S']), ('S',['b']) ]
regular = True
leftRG = False
rightRG = False
for leftSide, rightSide in grammar:
  for nonterminal in nonterminals:
    if not(leftRG or rightRG):
      if len(rightSide) > 1:
        if (nonterminal in rightSide[0]):
          leftRG = True
        elif (nonterminal in rightSide[-1]):
          rightRG = True
        else:
          regular = regular and not (nonterminal in rightSide)
    if rightRG: 
      regular = regular and not (nonterminal in rightSide[:-1])
    if leftRG:
      regular = regular and not (nonterminal in rightSide[1:])
print(regular)  