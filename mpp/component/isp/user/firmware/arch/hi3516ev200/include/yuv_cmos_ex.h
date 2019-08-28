#ifndef __YUV_CMOS_EX_H_
#define __YUV_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

static const ISP_CMOS_CA_S gyuv_stIspCA = {
    /* CA */
    1,
    /* Y */
    {
        645, 647, 650, 653, 656, 659, 663, 668, 672, 676, 681, 685, 690, 694, 699, 703,
        707, 711, 715, 719, 723, 727, 731, 735, 739, 743, 748, 752, 757, 761, 766, 771,
        776, 782, 787, 793, 799, 806, 812, 818, 824, 831, 837, 843, 848, 854, 859, 864,
        869, 873, 877, 881, 885, 889, 893, 896, 900, 903, 906, 909, 912, 915, 918, 921,
        924, 926, 929, 931, 934, 936, 938, 941, 943, 945, 947, 949, 951, 952, 954, 956,
        958, 961, 962, 964, 966, 968, 969, 970, 971, 973, 974, 976, 977, 979, 980, 981,
        983, 984, 985, 986, 988, 989, 990, 991, 992, 993, 995, 996, 997, 998, 999, 1000,
        1001, 1004, 1005, 1006, 1007, 1009, 1010, 1011, 1012, 1014, 1016, 1017, 1019, 1020, 1022, 1024
    },
    /* ISO */
    /*  1,    2,    4,    8,   16,   32,   64,  128,  256,  512, 1024, 2048, 4096, 8192, 16384, 32768 */
    {1300, 1300, 1250, 1200, 1150, 1100, 1050, 1000,  950,  900,  900,  800,  800,  800,   800,   800}
};

static const ISP_CMOS_SHARPEN_S gyuv_stIspYuvSharpen = {
    /* u8SkinUmin */
    110,
    /* u8SkinVmin */
    128,
    /* u8SkinUmax */
    128,
    /* u8SkinVmax */
    149,

    /* Manual Para */
    {
        /* au8LumaWgt */
        {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31},
        /* u16TextureStr */
        {250, 420, 390, 390, 390, 390, 390, 370, 350, 330, 310, 290, 270, 270, 270, 270, 270, 270, 266, 260, 244, 230, 230, 230, 230, 230, 230, 210, 190, 190, 170, 150},
        /* u16EdgeStr */
        {120, 123, 125, 128, 130, 135, 140, 148, 160, 168, 180, 190, 200, 210, 210, 210, 210, 210, 200, 190, 185, 175, 165, 160, 146, 136, 130, 128, 125, 123, 120, 120},
        /* u16TextureFreq; */
        160,
        /* u16EdgeFreq; */
        100,
        /* u8OverShoot; */
        55,
        /* u8UnderShoot; */
        70,
        /* u8shootSupStr; */
        10,
        /* u8shootSupAdj; */
        9,
        /* u8DetailCtrl; */
        128,
        /* u8DetailCtrlThr; */
        180,
        /* u8EdgeFiltStr; */
        60,
        /*u8EdgeFiltMaxCap; */
        18,
        /*u8RGain; */
        28,
        /* u8GGain; */
        32,
        /* u8BGain; */
        31,
        /* u8SkinGain; */
        23,
        /* u8MaxSharpGain; */
        67,
        /* u8WeakDetailGain */
        6

    },
    /* Auto Para */
    {
        /* au16LumaWgt */
        /* ISO */
        /*     100,    200,    400,    800,   1600,   3200,   6400,  12800,  25600,  51200, 102400, 204800, 409600, 819200,1638400,3276800 */
        {   {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,      1,      1,      1},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,      2,      2,      2},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,      4,      4,      4},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,      5,      5,      5},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,      7,      7,      7},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,      8,      8,      8},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,      9,      9,      9},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     10,     10,     10},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     12,     12,     12},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     13,     13,     13},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     14,     14,     14},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     15,     15,     15},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     17,     17,     17},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     18,     18,     18},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     19,     19,     19},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     20,     20,     20},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     22,     22,     22},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     23,     23,     23},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     24,     24,     24},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     26,     26,     26},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     26,     26,     26},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     26,     26,     26},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     26,     26,     26},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     27,     27,     27},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     27,     27,     27},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     27,     27,     27},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     27,     27,     27},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     28,     28,     28},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     28,     28,     28},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     28,     28,     28},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     28,     28,     28},
            {   31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     31,     28,     28,     28}
        },
        /* au16TextureStr  */
        /* ISO */
        /*     100,    200,    400,    800,   1600,   3200,   6400,  12800,  25600,  51200, 102400, 204800, 409600, 819200,1638400,3276800 */
        {   {  164,    135,    148,    146,    138,    145,    140,    140,    140,    113,     87,     87,     76,     28,     28,     28},
            {  196,    169,    177,    171,    164,    164,    163,    159,    159,    135,    101,    101,    130,     43,     43,     43},
            {  228,    203,    207,    195,    191,    191,    185,    178,    178,    157,    116,    116,    185,     59,     59,     59},
            {  256,    233,    232,    216,    212,    213,    204,    196,    196,    177,    132,    132,    238,     75,     75,     75},
            {  273,    252,    247,    228,    223,    223,    214,    208,    208,    191,    147,    147,    288,     91,     91,     91},
            {  288,    269,    261,    238,    232,    231,    222,    219,    219,    203,    162,    162,    332,    106,    106,    106},
            {  300,    282,    272,    246,    237,    237,    228,    229,    229,    214,    176,    176,    372,    121,    121,    121},
            {  308,    293,    280,    252,    241,    241,    232,    237,    237,    223,    188,    188,    410,    136,    136,    136},
            {  312,    300,    285,    255,    242,    243,    234,    243,    243,    229,    199,    199,    444,    152,    152,    152},
            {  313,    303,    288,    255,    240,    244,    233,    246,    246,    232,    207,    207,    475,    167,    167,    167},
            {  311,    305,    289,    254,    237,    243,    231,    248,    248,    234,    214,    214,    503,    183,    183,    183},
            {  308,    305,    288,    252,    233,    241,    228,    249,    249,    235,    220,    220,    527,    199,    199,    199},
            {  303,    304,    285,    249,    229,    238,    225,    249,    249,    234,    225,    225,    546,    214,    214,    214},
            {  295,    302,    281,    244,    224,    233,    220,    248,    248,    232,    229,    229,    559,    228,    228,    228},
            {  286,    299,    275,    239,    218,    227,    216,    246,    246,    229,    232,    232,    568,    241,    241,    241},
            {  277,    296,    268,    233,    213,    221,    211,    243,    243,    225,    233,    233,    572,    254,    254,    254},
            {  269,    292,    260,    227,    208,    215,    207,    240,    240,    221,    235,    235,    573,    267,    267,    267},
            {  261,    288,    251,    221,    202,    208,    203,    236,    236,    215,    235,    235,    568,    283,    283,    283},
            {  252,    284,    241,    215,    197,    201,    198,    232,    232,    209,    234,    234,    556,    298,    298,    298},
            {  243,    279,    232,    208,    192,    193,    193,    227,    227,    204,    232,    232,    535,    313,    313,    313},
            {  233,    273,    223,    201,    187,    186,    187,    222,    222,    199,    228,    228,    511,    325,    325,    325},
            {  222,    267,    214,    195,    183,    177,    181,    216,    216,    194,    223,    223,    482,    338,    338,    338},
            {  212,    261,    206,    188,    178,    169,    174,    209,    209,    189,    217,    217,    450,    348,    348,    348},
            {  201,    255,    197,    181,    173,    161,    168,    201,    201,    183,    210,    210,    417,    353,    353,    353},
            {  191,    248,    190,    174,    168,    153,    162,    192,    192,    175,    201,    201,    383,    350,    350,    350},
            {  180,    242,    182,    167,    162,    145,    156,    181,    181,    166,    191,    191,    347,    343,    343,    343},
            {  170,    235,    175,    160,    157,    137,    150,    170,    170,    157,    180,    180,    309,    330,    330,    330},
            {  161,    228,    167,    153,    151,    129,    144,    159,    159,    147,    168,    168,    269,    313,    313,    313},
            {  152,    221,    158,    147,    145,    120,    138,    148,    148,    137,    155,    155,    224,    287,    287,    287},
            {  144,    213,    149,    140,    140,    111,    132,    136,    136,    126,    142,    142,    117,    254,    254,    254},
            {  136,    205,    140,    134,    134,    102,    125,    125,    125,    116,    129,    129,    128,    219,    219,    219},
            {  128,    197,    131,    128,    128,     93,    119,    113,    113,    105,    118,    118,     80,    186,    186,    186}
        },
        /* au16EdgeStr     */
        /* ISO */
        /* 100,    200,    400,    800,   1600,   3200,   6400,  12800,  25600,  51200, 102400, 204800, 409600, 819200,1638400, 3276800 */
        {   {  219,    219,    193,    193,    172,    165,    131,    120,    432,    252,    214,    214,    224,    224,    224,    224},
            {  224,    224,    199,    199,    177,    169,    139,    126,    448,    262,    223,    223,    244,    244,    244,    244},
            {  228,    228,    205,    205,    181,    173,    147,    132,    463,    274,    233,    233,    265,    265,    265,    265},
            {  233,    233,    212,    212,    186,    177,    155,    139,    480,    284,    244,    244,    285,    285,    285,    285},
            {  237,    237,    218,    218,    190,    181,    161,    145,    494,    294,    254,    254,    303,    303,    303,    303},
            {  242,    242,    224,    224,    195,    185,    168,    151,    509,    301,    264,    264,    319,    319,    319,    319},
            {  246,    246,    230,    230,    199,    189,    174,    157,    524,    308,    273,    273,    333,    333,    333,    333},
            {  251,    251,    235,    235,    204,    193,    180,    163,    539,    314,    281,    281,    346,    346,    346,    346},
            {  255,    255,    239,    239,    208,    197,    185,    168,    554,    319,    288,    288,    357,    357,    357,    357},
            {  260,    260,    243,    243,    213,    201,    190,    173,    569,    322,    293,    293,    364,    364,    364,    364},
            {  265,    265,    247,    247,    217,    205,    195,    179,    583,    324,    297,    297,    370,    370,    370,    370},
            {  270,    270,    251,    251,    222,    209,    200,    184,    595,    326,    300,    300,    374,    374,    374,    374},
            {  274,    274,    254,    254,    226,    214,    204,    189,    605,    328,    302,    302,    379,    379,    379,    379},
            {  278,    278,    257,    257,    230,    217,    209,    195,    613,    329,    304,    304,    384,    384,    384,    384},
            {  282,    282,    260,    260,    234,    221,    213,    200,    619,    329,    305,    305,    390,    390,    390,    390},
            {  284,    284,    263,    263,    237,    224,    217,    205,    624,    329,    306,    306,    394,    394,    394,    394},
            {  286,    286,    265,    265,    241,    227,    222,    210,    627,    330,    307,    307,    398,    398,    398,    398},
            {  287,    287,    268,    268,    245,    231,    227,    215,    627,    330,    307,    307,    398,    398,    398,    398},
            {  288,    288,    272,    272,    249,    234,    232,    219,    627,    330,    307,    307,    398,    398,    398,    398},
            {  289,    289,    274,    274,    252,    237,    235,    223,    627,    330,    306,    306,    398,    398,    398,    398},
            {  288,    288,    276,    276,    255,    239,    237,    226,    628,    329,    305,    305,    398,    398,    398,    398},
            {  287,    287,    276,    276,    256,    240,    239,    229,    628,    329,    303,    303,    398,    398,    398,    398},
            {  284,    284,    275,    275,    255,    240,    239,    231,    628,    328,    300,    300,    399,    399,    399,    399},
            {  279,    279,    271,    271,    251,    237,    237,    229,    627,    328,    296,    296,    402,    402,    402,    402},
            {  272,    272,    264,    264,    244,    232,    232,    223,    627,    326,    291,    291,    405,    405,    405,    405},
            {  264,    264,    255,    255,    235,    225,    225,    214,    627,    325,    285,    285,    408,    408,    408,    408},
            {  254,    254,    244,    244,    223,    216,    216,    203,    623,    323,    279,    279,    411,    411,    411,    411},
            {  242,    242,    231,    231,    210,    205,    207,    193,    613,    320,    272,    272,    413,    413,    413,    413},
            {  231,    231,    218,    218,    196,    195,    197,    183,    593,    314,    264,    264,    411,    411,    411,    411},
            {  219,    219,    204,    204,    182,    184,    187,    173,    566,    307,    257,    257,    408,    408,    408,    408},
            {  208,    208,    191,    191,    169,    173,    177,    163,    536,    299,    249,    249,    405,    405,    405,    405},
            {  235,    197,    179,    179,    156,    163,    169,    153,    507,    292,    243,    243,    402,    402,    402,    402}
        },
        /* au16TextureFreq */
        /* ISO */
        /* 100,    200,    400,    800,    1600,    3200,    6400,   12800,   25600,   51200,  102400,  204800,  409600,  819200, 1638400, 3276800 */
        {  200,    200,    180,    180,     160,     160,     155,     170,     170,     170,     170,     170,     170,     170,     170,    170},

        /* au16EdgeFreq */
        {  130,    115,    100,    100,     100,     100,     100,     100,     100,     100,      96,      96,      96,      96,      96,     96},

        /* au8OverShoot */
        {   55,     60,     70,     70,      65,      55,      55,      55,      50,      45,      40,      40,      10,      10,      10,     10},

        /* au8UnderShoot */
        {   65,     75,     80,     80,      80,      70,      60,      55,      45,      45,      50,      50,      15,      15,      15,     15},

        /* au16shootSupStr */
        {    6,      6,      5,      5,       4,       4,       2,       2,       0,       0,       0,       0,       0,       0,       0,      0},

        /* au8ShootSupAdj */
        {    8,      8,      6,      6,       4,       4,       2,       2,       0,       0,       0,       0,       0,       0,       0,      0},

        /* au8DetailCtrl */
        {  135,    135,    128,     128,    128,     128,     128,     128,     128,     128,     120,     120,     120,     120,     120,    120},

        /* au8DetailCtrlThr */
        {  160,    160,    160,     160,    160,     160,     160,     160,     160,     160,     160,     160,     160,     160,     160,    160},

        /* au8EdgeFiltStr */
        {   50,     50,     50,      55,     57,      60,      61,      62,      62,      62,      62,      62,      62,      62,      62,     62},

        /*au8EdgeFiltMaxCap[ISP_AUTO_ISO_STRENGTH_NUM];*/
        { 18,   18,    18 ,  18,    18,   18,    18 ,    18 ,      18  ,   18 ,    18 ,    18 ,    18,    18  ,    18  ,   18 },

        /*au8RGain[ISP_AUTO_ISO_STRENGTH_NUM];*/
        {   28,     25,     20,      20,     20,      20,      20,      31,      31,      31,      31,      31,      31,      31,      31,     31},

        /* au8GGain */
        {   32,     32,     32,      32,     32,      32,      32,      32,      32,      32,      32,      32,      32,      32,      32,     32},

        /* au8BGain */
        {   31,     31,     31,      31,     31,      31,      31,      31,      31,      31,      31,      31,      31,      31,      31,     31},

        /* au8SkinGain */
        {   27,     27,     31,      31,     31,      31,      31,      31,      31,      31,      31,      31,      31,      31,      31,     31},

        /* u8MaxSharpGain */
        {   67,     70,     72,      74,     80,      80,      80,      80,      80,      80,      80,      80,      80,      80,      80,     80},
        /* au8WeakDetailGain                          */
        {   0,       0,     0,       0,     0,      0,         0,         0,     0,         0,     0,      0,      0,      0,        0,      0  }
    },
};

static const ISP_CMOS_LDCI_S gyuv_stIspLdci = {
    /* bEnable */
    1,

    /* u8GaussLPFSigma */
    36,

    /* 1,    2,    4,    8,   16,   32,   64,  128,  256,  512, 1024, 2048, 4096, 8192, 16384, 32768 */
    /* au8HePosWgt */
    { 16,   16,   12,    8,    4,    0,    0,    0,    0,    0,    0,    0,    0,    0,     0,     0},

    /* au8HePosSigma */
    { 80,   80,   80,   80,   80,   80,   80,   80,   80,   80,   80,   80,   80,   80,    80,    80},

    /* au8HePosMean */
    { 32,   32,   32,   20,   20,   20,   20,   20,   20,   32,   32,   32,   32,   32,    32,    32},

    /* au8HeNegWgt */
    { 32,   32,   40,   45,   45,   24,   12,    8,    6,    0,    0,    0,    0,    0,     0,     0},

    /* au8HeNegSigma */
    { 80,   80,   80,   80,   80,   80,   80,   80,   80,   80,   80,   80,   80,   80,    80,    80},

    /* au8HeNegMean */
    {180,  180,  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,   128,   128},

    /* au16BlcCtrl */
    { 20,   20,   20,   20,   30,   30,   30,   30,   30,   30,   30,   30,   30,   30,    30,    30}
};

static const ISP_CMOS_DNG_COLORPARAM_S gyuv_stDngColorParam = {
    {378, 256, 430},
    {439, 256, 439}
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __YUV_CMOS_EX_H_ */
