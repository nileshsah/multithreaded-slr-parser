# Multithreaded SLR(1) Parser in C++11

A multithreaded SLR(1) parser developed as a part of the Programming Language Translator course using pthreads library.

### Compile

```
g++ -std=c++11 SLR_Parser.cpp 
```

### Sample Input

```
Enter number of Production Rules: 4
Enter start symbol: S
Enter Production Rules:
S->E
E->E+T|T
T->T*F|F
F->(E)|i
```

### Sample Output

```
FOLLOW: E -> $, ), +, 
FOLLOW: F -> $, ), *, +, 
FOLLOW: T -> $, ), *, +, 

[ACTION TABLE]
GOTO(I0,() = S1
GOTO(I0,E) = G2
GOTO(I0,F) = G3
GOTO(I0,T) = G4
GOTO(I0,i) = S5
.
.
.

Total Table Entries: 45
```

### String Validation

```
Enter string to validate: i*(i+i)

[STATE BUFFER ACTION]
 i*(i+i)$:  5i0
  *(i+i)$:  3F0
  *(i+i)$:  4T0
  *(i+i)$:  8*4T0
   (i+i)$:  1(8*4T0
    i+i)$:  5i1(8*4T0
     +i)$:  3F1(8*4T0
     +i)$:  4T1(8*4T0
     +i)$:  6E1(8*4T0
     +i)$:  7+6E1(8*4T0
      i)$:  5i7+6E1(8*4T0
       )$:  3F7+6E1(8*4T0
       )$:  10T7+6E1(8*4T0
       )$:  6E1(8*4T0
       )$:  9)6E1(8*4T0
        $:  11F8*4T0
        $:  4T0
        $:  2E0
        $:  0S0
i*(i+i) ACCEPTED
```
