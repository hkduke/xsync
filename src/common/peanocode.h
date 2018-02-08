/**
 * peanocode.h
 * 2013-6-12
 * cheungmine
 * All rights reserved
 */
#ifndef _PEANOCODE_H_
#define _PEANOCODE_H_

#include <assert.h>
#include <math.h>


#if defined (_SVR4) || defined (SVR4) || defined (__OpenBSD__) || defined (_sgi) || defined (__sun) || defined (sun) || defined (__digital__) || defined (__HP_cc)
    #include <inttypes.h>

    #elif defined (_MSC_VER) && _MSC_VER < 1600
        /* VS 2010 (_MSC_VER 1600) has stdint.h */
        typedef __int8 int8_t;
        typedef unsigned __int8 uint8_t;
        typedef __int16 int16_t;
        typedef unsigned __int16 uint16_t;
        typedef __int32 int32_t;
        typedef unsigned __int32 uint32_t;
        typedef __int64 int64_t;
        typedef unsigned __int64 uint64_t;
    #elif defined (_AIX)
        # include <sys/inttypes.h>
    #else
        # include <stdint.h>
#endif

#ifndef byte_t
    typedef unsigned char byte_t;
#endif


/* DO NOT CHANGE BELOW LINE */
#define PCODE_LEVEL_DEPTH  24

/* DO NOT CHANGE BELOW LINE */
#define PCODE_GRIDS_MAX    512

/**
 * __peanocode
 *
 *  +-------+-------+ DY
 *  |       |       |
 *  |   0   |   2   |
 *  |       |       |
 *  +-------+-------+ Sy
 *  |       |       |
 *  |   1   |   3   |
 *  |       |       |
 *  +-------+-------+
 *  0      Sx       DX
 *
 *  level0 = 0
 *  Level < PCODE_LEVEL_DEPTH
 */
static void __peanocode (
    double DX0,    /* X-extent of level0 */
    double DY0,    /* Y-extent of level0 */
    int level0,    /* level0 = 0 */
    double x,      /* x-coord of point */
    double y,      /* y-coord of point */
    char pcode[],  /* peano code of grid where point (x,y) lies in */
    int Level)     /* level number of grid */
{
    if (level0 == Level) {
        pcode[Level] = 0;
        return;
    } else {
        double Sx = DX0/2;
        double Sy = DY0/2;

        if (x >= 0 && x < Sx && y >= 0 && y < Sy) {
            pcode[level0++] = '0';
            __peanocode (Sx, Sy, level0, x, y, pcode, Level);
        } else if (x >= Sx && x < DX0 && y >= 0 && y < Sy) {
            pcode[level0++] = '2';
            __peanocode (Sx, Sy, level0, x-Sx, y, pcode, Level);
        } else if (x >= Sx && x < DX0 && y >= Sy && y < DY0) {
            pcode[level0++] = '3';
            __peanocode (Sx, Sy, level0, x-Sx, y-Sy, pcode, Level);
        } else if (x >= 0 && x < Sx && y >= Sy && y < DY0) {
            pcode[level0++] = '1';
            __peanocode (Sx, Sy, level0, x, y-Sy, pcode, Level);
        }
    }
}


/**
 * pt2pcode
 */
static const char* pt2pcode (
    double DX0,   /* X-extent of level0 */
    double DY0,   /* Y-extent of level0 */
    double x,     /* x-coord of point */
    double y,     /* y-coord of point */
    char pcode[], /* peano code of grid where point (x,y) lies in */
    int Level)    /* level number of grid */
{
    __peanocode(DX0, DY0, 0, x, y, pcode, Level);
    return pcode;
}


/**
 * pcode2rowkey
 */
static int64_t pcode2rowkey (
    char pcode[])
{
    int64_t rowkey = 0;

    if (pcode[0]) {
        int i = 1;
        int64_t quad = 1;
        int64_t level = 0;

        for (;; i++) {
            ++level;

            if (pcode[i] == 0) {
                rowkey += (pcode[0] - '0');
                rowkey |= (level << 56);
                break;
            }

            quad = quad << 2;
            rowkey += quad * (pcode[i] - '0');
        }
    }

    return rowkey;
}


/**
 * rowkey2pcode
 */
static int rowkey2pcode (int64_t rowkey, char pcode[])
{
    int64_t key = rowkey & 0x0000FFFFFFFFFFFF;
    int level = (int) ((rowkey & 0xFF00000000000000) >> 56);

    int i = 0;
    for (; i < level; i++) {
        pcode[i] = (char) (key % 4 + '0');
        key = key >> 2;
    }
    pcode[i] = 0;
    return level;
}


/**
 * pcode2grid
 */
static int pcode2grid (char pcode[], uint32_t *xi, uint32_t *yi)
{
    char c;
    int level = 0;

    *xi = 0;
    *yi = 0;

    for (;; level++) {
        if (pcode[level] == 0) {
            break;
        }

        c = pcode[level];

        if (c == '0') {
            *xi = *xi << 1;
            *yi = *yi << 1;
        } else if (c == '1') {
            *xi = *xi << 1;
            *yi = (*yi << 1) + 1;
        } else if (c == '2') {
            *xi = (*xi << 1) + 1;
            *yi = *yi << 1;
        } else if (c == '3') {
            *xi = (*xi << 1) + 1;
            *yi = (*yi << 1) + 1;
        }
    }

    return level;
}


/**
 * grid2pcode
 */
static void grid2pcode (uint32_t xi, uint32_t yi, int level, char pcode[])
{
    int64_t one = 1;
    double Dxy = (double) (one<<level);
    pt2pcode(Dxy, Dxy, xi, yi, pcode, level);
}


/**
 * grid2pt
 */
static void grid2pt (double DX, double DY, uint32_t xi, uint32_t yi, int level, double *x, double *y)
{
    int64_t one = 1;
    double grids = (double) (one << level);

    *x = xi * DX / grids;
    *y = yi * DY / grids;
}


/**
 * grid2mbr
 */
static void grid2mbr (double DX, double DY,
    uint32_t xi, uint32_t yi, int Level,
    double *Xmin, double *Ymin, double *Xmax, double *Ymax)
{
    grid2pt(DX, DY, xi, yi, Level, Xmin, Ymin);
    grid2pt(DX, DY, xi+1, yi+1, Level, Xmax, Ymax);
}


/**
 * __mbr_grids
 *   get count of grids of mbr
 */
static int __mbr_grids (double DX, double DY, char pcode[], int level,
    double Xmin, double Ymin, double Xmax, double Ymax, /* mbr */
    uint32_t *x1, uint32_t *y1, uint32_t *x2, uint32_t *y2)
{
    pt2pcode(DX, DY, Xmin, Ymin, pcode, level);
    pcode2grid(pcode, x1, y1);

    pt2pcode(DX, DY, Xmax, Ymax, pcode, level);
    pcode2grid(pcode, x2, y2);

    *x2 += 1;
    *y2 += 1;

    if ((*x2 - *x1) > PCODE_GRIDS_MAX || (*y2 - *y1) > PCODE_GRIDS_MAX) {
        return PCODE_GRIDS_MAX;
    } else {
        return (*x2 - *x1) * (*y2 - *y1);
    }
}


/**
 * mbr2gbr
 *
 */
static int mbr2gbr (double DX, double DY,
    int LevelNo, /* start level no. 0 < LevelNo < LEVEL_DEPTH */
    double Xmin, double Ymin, double Xmax, double Ymax,                /* mbr */
    uint32_t *MinXI, uint32_t *MinYI, uint32_t *MaxXI, uint32_t *MaxYI /* gbr */)
{
    int num;
    char pcode[PCODE_LEVEL_DEPTH];

    int level = LevelNo;

    for (; level > 0; level--) {
        num = __mbr_grids(DX, DY, pcode, level,
            Xmin, Ymin, Xmax, Ymax, MinXI, MinYI, MaxXI, MaxYI);

        if (num <= 4) {
            if (level == 1 || num == 1) {
                return level;
            } else {
                uint32_t x1, y1, x2, y2, n, n2 = num;

                while (level > 1 && (n = __mbr_grids(DX, DY, pcode, level-1,
                    Xmin, Ymin, Xmax, Ymax, &x1, &y1, &x2, &y2)) < n2) {
                    n2 = n;
                    --level;

                    *MinXI = x1;
                    *MinYI = y1;
                    *MaxXI = x2;
                    *MaxYI = y2;

                    if (n == 1) {
                        break;
                    }
                }
                break;
            }
        }
    }
    return level;
}


#ifdef PCODE_TEST_ENABLED

struct MBR
{
    double Xmin, Ymin, Xmax, Ymax;
};

/*
           Level=1
                      (W0=8, 8)
 8 +-----------+-----------+
   | 11  |  13 |           |
   |     |     |           |
   |-----1-----|    3      |    row:1
   | 10  |  12 |           |
   |     |.P   |           |
 4 +-----+-----+-----------+
   |           |           |
   |           |           |
   |     0     |    2      |     row:0
   |           |           |
   |           |           |
   +-----+-----+-----------+
   0     2     4           8

      col:0       col:1

  grid(col, row)

  P(2.1, 4.3)
*/

static MBR plygons[] = {
    {2.6, 1.2, 2.9, 1.6}, // A
    {1.7, 1.1, 2.6, 2.9}, // B
    {4.3, 6.3, 5.8, 7.9}, // C
    {3.3, 1.6, 6.7, 4.8}, // D
    {0.6, 3.6, 2.3, 4.4}  // E
};

const char * names[] = {
    "A",
    "B",
    "C",
    "D",
    "E"
};


/*=====================================================================
  Layer Data Rect (0, 0, 8, 8)
  Layer Shapes Table:
  +-----------+-----------------------------+
  |   Shape   |              MBR            |
  +-----------+-----------------------------+
  |    SID    |  Xmin   Ymin     Xmax  Ymax |
  +-----------+-----------------------------+
  |     A     |  2.6    1.2      2.9   1.6  |
  |     B     |  1.7    1.1      2.6   2.9  |
  |     C     |  4.3    6.3      5.8   7.9  |
  |     D     |  3.3    1.6      6.7   4.8  |
  |     E     |  0.6    3.6      2.3   4.4  |
  +-----------------------------------------+

  Layer Index Table:
  +-------+-------------------+----------+-----------------------+
  | Shape |        GRID       | (Unused) |       HBase           |
  +-------+-------------------+          +-----------------------+
  |  SID  |  XI   YI   Level  |  Pcode   |     Rowkey (8 Bytes)  |
  +-------+-------------------+----------+-----------------------+
  |   A   |   2    1    <3>   |    021   |   216172782113783832  |
  |   B   |   1    1    <3>   |    003   |   216172782113783856  |
  |   B   |   1    2    <3>   |    012   |   216172782113783844  |
  |   B   |   2    1    <3>   |    021   |   216172782113783832  |
  |   B   |   2    2    <3>   |    030   |   216172782113783820  |
  |   C   |   2    3    <2>   |     31   |   144115188075855879  |
  |   D   |   0    0    <1>   |      0   |    72057594037927936  |
  |   D   |   0    1    <1>   |      1   |    72057594037927937  |
  |   D   |   1    0    <1>   |      2   |    72057594037927938  |
  |   D   |   1    1    <1>   |      3   |    72057594037927939  |
  |   E   |   0    0    <1>   |      0   |    72057594037927936  |
  |   E   |   0    1    <1>   |      1   |    72057594037927937  |
  +-------+-------------------+----------+-----------------------+
=====================================================================*/

static void pcode_test()
{
    /* suppose data MBR is 8x8 */
    double DX0 = 8;
    double DY0 = 8;
    int    level0 = 0;

    int    MaxLevelNo = MAX_INDEX_DEPTH - 1;

    int level;
    int64_t rowkey;

    uint32_t xi;
    uint32_t yi;
    int lvl;

    // GBR
    uint32_t MinXI, MinYI, MaxXI, MaxYI;

    char pcode[MAX_INDEX_DEPTH] = {0};
    char code2[MAX_INDEX_DEPTH] = {0};

    printf("  Layer Data Rect (0, 0, %.0lf, %.0lf)\n", DX0, DY0);
    printf("  Layer Shapes Table:\n");
    printf("  +-----------+-----------------------------+\n");
    printf("  |   Shape   |              MBR            |\n");
    printf("  +-----------+-----------------------------+\n");
    printf("  |    SID    |  Xmin   Ymin     Xmax  Ymax |\n");
    printf("  +-----------+-----------------------------+\n");
    for (int i=0; i<sizeof(plygons)/sizeof(plygons[0]); i++) {
        MBR p = plygons[i];
        printf("  |     %s     |  %.1f    %.1f      %.1f   %.1f  |\n", names[i], p.Xmin, p.Ymin, p.Xmax, p.Ymax);
    }
    printf("  +-----------------------------------------+\n\n");

    printf("  Layer Index Table:\n");
    printf("  +-------+-------------------+----------+-----------------------+\n");
    printf("  | Shape |        GRID       | (Unused) |       HBase           |\n");
    printf("  +-------+-------------------+          +-----------------------+\n");
    printf("  |  SID  |  XI   YI   Level  |  Pcode   |     Rowkey (8 Bytes)  |\n");
    printf("  +-------+-------------------+----------+-----------------------+\n");

    for (int i=0; i<sizeof(plygons)/sizeof(plygons[0]); i++) {
        MBR p = plygons[i];
        lvl = mbr2gbr(DX0, DY0, 3, p.Xmin, p.Ymin, p.Xmax, p.Ymax, &MinXI, &MinYI, &MaxXI, &MaxYI);

        for (xi = MinXI; xi < MaxXI; xi++) {
            for (yi = MinYI; yi < MaxYI; yi++) {
                grid2pcode(xi, yi, lvl, pcode);
                rowkey = pcode2rowkey(pcode);
                printf("  |   %s   |   %d    %d    <%d>   | %6s   | %20lld  |\n", names[i], xi, yi, lvl, pcode, rowkey);
                level = rowkey2pcode(rowkey, code2);
                assert(lvl==level);
                assert(!strcmp(pcode, code2));
            }
        }
    }
    printf("  +-------+-------------------+----------+-----------------------+\n\n");
}

#endif  /* PCODE_TEST_ENABLED */

#endif  /* _PEANOCODE_H_ */
