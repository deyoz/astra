#ifndef SERVERLIB_MAKENAME_DEFS_H
#define SERVERLIB_MAKENAME_DEFS_H

#define MakeName2(N1, N2) N1 ## N2
#define MakeName3(N1, N2, N3) N1 ## N2 ## N3
#define MakeName4(N1, N2, N3, N4) N1 ## N2 ## N3 ## N4
#define MakeName5(N1, N2, N3, N4, N5) N1 ## N2 ## N3 ## N4 ## N5

#define MakeNameWithLine2__(x, y, l) MakeName3(x, y,l)
#define MakeNameWithLine2(x, y) MakeNameWithLine2__(x, y, __LINE__)

#endif
