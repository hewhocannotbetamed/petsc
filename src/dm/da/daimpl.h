/* $Id: daimpl.h,v 1.8 1995/10/24 21:54:28 bsmith Exp bsmith $ */

/*

*/

#if !defined(_DAIMPL_H)
#define _DAIMPL_H
#include "da.h"

struct _DA {
  PETSCHEADER
  int           M,N,P;             /* array dimensions */
  int           m,n,p;             /* processor layout */
  int           w;                 /* degrees of freedome per node */
  int           s;                 /* stencil width */
  int           xs,xe,ys,ye,zs,ze; /* range of local values */
  int           Xs,Xe,Ys,Ye,Zs,Ze; /* range including ghost values*/
  int           *idx,Nl;           /* local to global map */
  int           base;              /* global number of 1st local node */
  int           wrap;              /* indicates if periodic boundaries */
  VecScatter    gtol,ltog,ltol;      
  Vec           global,local;
  DAStencilType stencil_type;
};

/*
     gtol - Global representation to local 
            Global has on each processor interior degrees of freedom and no
                   ghost points. This vector is what the solvers usually see.
            Local has on each processor the ghost points as well. This is 
                  what code to calculate Jacobians etc. sees
     ltog - Local representation to global (involves no communication)
     ltol - Local representation to local representation
                  updates the ghostpoint values in the second vector from 
                  (correct) interior values in the first vector.
                  This is good for explicit nearest neighbor time-stepping.
*/
#endif
