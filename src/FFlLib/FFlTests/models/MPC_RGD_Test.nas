ASSIGN dbc='01_MPC_Test_coarse.xdb',
unit=40 unformated delete
SOL 101
TIME 10000
CEND
  TITLE = C:\00_Work\02_Prosjekter\00_Demo\03_Bugmodels\02_MPC_Test\02_ANSYS\01_MPC_Test_coarse.nas
  ECHO = NONE
  DISPLACEMENT(PLOT) = ALL
  STRESS(PLOT) = ALL
  SPC = 1
BEGIN BULK
PARAM, WTMASS,1.000000
PARAM, POST, 0
$ CORD SYS ------------------------------
CORD2R  12      0       0.0000000.0000000.0000000.0000000.0000001.000000
        1.0000000.0000000.000000
$ Materials ------------------------------
MAT1    1       2.000+11        0.3000007850.0001.200-0522.00000
MAT1    2       2.000+11        0.3000007850.0001.200-0522.00000
$ Properties ------------------------------
PSHELL  1       1       0.0100001               1               0.000000
PSHELL  2       2       0.0050002               2               0.000000
$ RBE2s (Rigid)  ------------------------------
RBE2    70      65      123456  8       7       3       4
RBE2    75      67      123456  57      64      43      45
$ NODES ------------------------------
GRID    1       0       0.050000-0.050001.9000000
GRID    2       0       -0.05000-0.050001.9000000
GRID    3       0       -0.05000-0.050000.0000000
GRID    4       0       0.050000-0.050000.0000000
GRID    5       0       0.0500000.0500001.9000000
GRID    6       0       -0.050000.0500001.9000000
GRID    7       0       -0.050000.0500000.0000000
GRID    8       0       0.0500000.0500000.0000000
GRID    9       0       0.050000-0.050001.5833550
GRID    10      0       0.050000-0.050001.2666840
GRID    11      0       0.050000-0.050000.9500130
GRID    12      0       0.050000-0.050000.6333420
GRID    13      0       0.050000-0.050000.3166710
GRID    14      0       0.0500000.0500001.5833640
GRID    15      0       0.0500000.0500001.2666910
GRID    16      0       0.0500000.0500000.9500180
GRID    17      0       0.0500000.0500000.6333450
GRID    18      0       0.0500000.0500000.3166730
GRID    19      0       -0.050000.0500001.5833330
GRID    20      0       -0.050000.0500001.2666670
GRID    21      0       -0.050000.0500000.9500000
GRID    22      0       -0.050000.0500000.6333330
GRID    23      0       -0.050000.0500000.3166670
GRID    24      0       -0.05000-0.050001.5833640
GRID    25      0       -0.05000-0.050001.2666910
GRID    26      0       -0.05000-0.050000.9500180
GRID    27      0       -0.05000-0.050000.6333450
GRID    28      0       -0.05000-0.050000.3166730
GRID    29      0       -0.05000-0.050002.0000000
GRID    30      0       -0.050000.0500002.0000000
GRID    31      0       -0.050000.0000002.0000000
GRID    32      0       0.050000-0.050002.0000000
GRID    33      0       0.000000-0.050002.0000000
GRID    34      0       0.0500000.0500002.0000000
GRID    35      0       0.0000000.0500002.0000000
GRID    36      0       0.0500000.0000002.0000000
GRID    37      0       0.0500000.1500002.0000000
GRID    38      0       -0.133330.1500002.0000000
GRID    39      0       -0.316670.1500002.0000000
GRID    40      0       -0.500000.1500002.0000000
GRID    41      0       -0.683330.1500002.0000000
GRID    42      0       -0.866670.1500002.0000000
GRID    43      0       -1.050000.1500002.0000000
GRID    44      0       0.0500000.1500001.9000000
GRID    45      0       -1.050000.1500001.9000000
GRID    46      0       -0.133330.1500001.9000000
GRID    47      0       -0.316670.1500001.9000000
GRID    48      0       -0.500000.1500001.9000000
GRID    49      0       -0.683330.1500001.9000000
GRID    50      0       -0.866670.1500001.9000000
GRID    51      0       0.0500000.0500001.9000000
GRID    52      0       -0.133330.0500001.9000000
GRID    53      0       -0.316670.0500001.9000000
GRID    54      0       -0.500000.0500001.9000000
GRID    55      0       -0.683330.0500001.9000000
GRID    56      0       -0.866670.0500001.9000000
GRID    57      0       -1.050000.0500001.9000000
GRID    58      0       0.0500000.0500002.0000000
GRID    59      0       -0.133330.0500002.0000000
GRID    60      0       -0.316670.0500002.0000000
GRID    61      0       -0.500000.0500002.0000000
GRID    62      0       -0.683330.0500002.0000000
GRID    63      0       -0.866670.0500002.0000000
GRID    64      0       -1.050000.0500002.0000000
GRID    65      0       0.0000000.0000000.0000000
GRID    67      0       -1.050000.1000001.9500000
$ ELEMENTS ------------------------------
$ ANSYS: Combin14 (longitudinal)
$ ANSYS: Combin14 (coincident)
$ ANSYS: Combin250
$ ANSYS: Link180
$ ANSYS: Beam188
$ ANSYS: Shell 181
CTRIA3  1       1       35      34      5
CQUAD4  2       1       6       30      35      5
CTRIA3  3       1       36      32      1
CQUAD4  4       1       5       34      36      1
CTRIA3  5       1       31      30      6
CQUAD4  6       1       2       29      31      6
CQUAD4  7       1       18      13      4       8
CQUAD4  8       1       17      12      13      18
CQUAD4  9       1       16      11      12      17
CQUAD4  10      1       15      10      11      16
CQUAD4  11      1       14      9       10      15
CQUAD4  12      1       5       1       9       14
CTRIA3  13      1       33      29      2
CQUAD4  14      1       1       32      33      2
CQUAD4  15      1       4       13      28      3
CQUAD4  16      1       13      12      27      28
CQUAD4  17      1       12      11      26      27
CQUAD4  18      1       11      10      25      26
CQUAD4  19      1       10      9       24      25
CQUAD4  20      1       9       1       2       24
CQUAD4  21      1       19      24      2       6
CQUAD4  22      1       20      25      24      19
CQUAD4  23      1       21      26      25      20
CQUAD4  24      1       22      27      26      21
CQUAD4  25      1       23      28      27      22
CQUAD4  26      1       7       3       28      23
CQUAD4  27      1       14      19      6       5
CQUAD4  28      1       15      20      19      14
CQUAD4  29      1       16      21      20      15
CQUAD4  30      1       17      22      21      16
CQUAD4  31      1       18      23      22      17
CQUAD4  32      1       8       7       23      18
CQUAD4  33      2       40      61      62      41
CQUAD4  34      2       39      60      61      40
CQUAD4  35      2       63      42      41      62
CQUAD4  36      2       38      59      60      39
CQUAD4  37      2       37      58      59      38
CQUAD4  38      2       64      43      42      63
CQUAD4  39      2       48      40      41      49
CQUAD4  40      2       47      39      40      48
CQUAD4  41      2       42      50      49      41
CQUAD4  42      2       46      38      39      47
CQUAD4  43      2       43      45      50      42
CQUAD4  44      2       44      37      38      46
CQUAD4  45      2       54      61      60      53
CQUAD4  46      2       55      62      61      54
CQUAD4  47      2       59      52      53      60
CQUAD4  48      2       56      63      62      55
CQUAD4  49      2       58      51      52      59
CQUAD4  50      2       57      64      63      56
CQUAD4  51      2       48      54      53      47
CQUAD4  52      2       49      55      54      48
CQUAD4  53      2       52      46      47      53
CQUAD4  54      2       50      56      55      49
CQUAD4  55      2       51      44      46      52
CQUAD4  56      2       45      57      56      50
$ ANSYS: Shell 281
$ ANSYS: Solid 185
$ ANSYS: Solid 186
$ ANSYS: Solid 187
$ ANSYS: Mass 21
$ MPCs
MPC     1       34      4       1.00000058      4       -1.00000

MPC     2       34      5       1.00000058      5       -1.00000

MPC     3       34      6       1.00000058      6       -1.00000

MPC     4       34      1       1.00000058      1       -1.00000

MPC     5       34      2       1.00000058      2       -1.00000

MPC     6       34      3       1.00000058      3       -1.00000

MPC     7       35      1       1.00000059      1       -0.30732
                58      1       -0.3073252      1       -0.19268
                51      1       -0.1926858      3       0.105096
                51      3       0.10509659      3       -0.10510
                52      3       -0.10510
MPC     8       35      2       1.00000058      2       -0.61364
                59      2       -0.3863652      2       0.113636
                51      2       -0.11364
MPC     9       35      3       1.00000058      3       -0.33758
                51      3       -0.3375859      3       -0.16242
                52      3       -0.1624259      1       0.047771
                58      1       0.04777152      1       -0.04777
                51      1       -0.04777
MPC     10      35      4       1.00000059      2       5.000000
                58      2       5.00000052      2       -5.00000
                51      2       -5.00000
MPC     11      35      5       1.00000058      3       2.101911
                51      3       2.10191159      3       -2.10191
                52      3       -2.1019152      1       1.146497
                51      1       1.14649759      1       -1.14650
                58      1       -1.14650
MPC     12      35      6       1.00000059      2       2.727273
                52      2       2.72727358      2       -2.72727
                51      2       -2.72727
MPC     13      5       4       1.00000051      4       -1.00000
MPC     14      5       5       1.00000051      5       -1.00000
MPC     15      5       6       1.00000051      6       -1.00000
MPC     16      5       1       1.00000051      1       -1.00000
MPC     17      5       2       1.00000051      2       -1.00000
MPC     18      5       3       1.00000051      3       -1.00000
MPC     19      30      1       1.00000059      1       -0.30732
                58      1       -0.3073252      1       -0.19268
                51      1       -0.1926858      3       0.105096
                51      3       0.10509659      3       -0.10510
                52      3       -0.10510
MPC     20      30      2       1.00000059      2       -0.52273
                58      2       -0.4772751      2       0.022727
                52      2       -0.02273
MPC     21      30      3       1.00000059      3       -0.26752
                52      3       -0.2675258      3       -0.23248
                51      3       -0.2324852      1       0.009554
                51      1       0.00955459      1       -0.00955
                58      1       -0.00955
MPC     22      30      4       1.00000059      2       5.000000
                58      2       5.00000052      2       -5.00000
                51      2       -5.00000
MPC     23      30      5       1.00000058      3       2.101911
                51      3       2.10191159      3       -2.10191
                52      3       -2.1019152      1       1.146497
                51      1       1.14649759      1       -1.14650
                58      1       -1.14650
MPC     24      30      6       1.00000059      2       2.727273
                52      2       2.72727358      2       -2.72727
                51      2       -2.72727
MPC     25      6       1       1.00000052      1       -0.30732
                51      1       -0.3073259      1       -0.19268
                58      1       -0.1926859      3       0.105096
                52      3       0.10509658      3       -0.10510
                51      3       -0.10510
MPC     26      6       2       1.00000052      2       -0.52273
                51      2       -0.4772758      2       0.022727
                59      2       -0.02273
MPC     27      6       3       1.00000059      3       -0.26752
                52      3       -0.2675258      3       -0.23248
                51      3       -0.2324852      1       0.009554
                51      1       0.00955459      1       -0.00955
                58      1       -0.00955
MPC     28      6       4       1.00000059      2       5.000000
                58      2       5.00000052      2       -5.00000
                51      2       -5.00000
MPC     29      6       5       1.00000058      3       2.101911
                51      3       2.10191159      3       -2.10191
                52      3       -2.1019152      1       1.146497
                51      1       1.14649759      1       -1.14650
                58      1       -1.14650
MPC     30      6       6       1.00000059      2       2.727273
                52      2       2.72727358      2       -2.72727
                51      2       -2.72727
ENDDATA