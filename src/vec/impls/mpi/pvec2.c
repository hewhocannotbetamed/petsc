
/* $Id: pvec2.c,v 1.7 1995/06/20 01:45:52 bsmith Exp bsmith $ */

#include <math.h>
#include "pvecimpl.h" 
#include "inline/dot.h"

static int VecMDot_MPI( int nv, Vec xin, Vec *y, Scalar *z )
{
  Scalar *work;
  work = (Scalar *)PETSCMALLOC( nv * sizeof(Scalar) );  CHKPTRQ(work);
  VecMDot_Seq(  nv, xin, y, work );
#if defined(PETSC_COMPLEX)
  MPI_Allreduce((void *) work,(void *)z,2*nv,MPI_DOUBLE,MPI_SUM,xin->comm );
#else
  MPI_Allreduce((void *) work,(void *)z,nv,MPI_DOUBLE,MPI_SUM,xin->comm );
#endif
  PETSCFREE(work);
  return 0;
}

static int VecNorm_MPI(  Vec xin, double *z )
{
  Vec_MPI *x = (Vec_MPI *) xin->data;
  double sum, work = 0.0;
  Scalar  *xx = x->array;
  register int n = x->n;
#if defined(PETSC_COMPLEX)
  int i;
  for (i=0; i<n; i++ ) {
    work += real(conj(xx[i])*xx[i]);
  }
#else
  SQR(work,xx,n);
#endif
  MPI_Allreduce((void *) &work,(void *) &sum,1,MPI_DOUBLE,MPI_SUM,xin->comm );
  *z = sqrt( sum );
  return 0;
}


static int VecAMax_MPI( Vec xin, int *idx, double *z )
{
  double work;
  /* Find the local max */
  VecAMax_Seq( xin, idx, &work );

  /* Find the global max */
  if (!idx) {
    MPI_Allreduce((void *) &work,(void *) z,1,MPI_DOUBLE,MPI_MAX,xin->comm );
  }
  else {
    /* Need to use special linked max */
    SETERRQ( 1, "Parallel max with index not yet supported" );
  }
  return 0;
}

static int VecMax_MPI( Vec xin, int *idx, double *z )
{
  double work;
  /* Find the local max */
  VecMax_Seq( xin, idx, &work );

  /* Find the global max */
  if (!idx) {
    MPI_Allreduce((void *) &work,(void *) z,1,MPI_DOUBLE,MPI_MAX,xin->comm );
  }
  else {
    /* Need to use special linked max */
    SETERRQ( 1, "Parallel max with index not yet supported" );
  }
  return 0;
}

static int VecMin_MPI( Vec xin, int *idx, double *z )
{
  double work;
  /* Find the local Min */
  VecMin_Seq( xin, idx, &work );

  /* Find the global Min */
  if (!idx) {
    MPI_Allreduce((void *) &work,(void *) z,1,MPI_DOUBLE,MPI_MIN,xin->comm );
  }
  else {
    /* Need to use special linked Min */
    SETERRQ( 1, "Parallel Min with index not yet supported" );
  }
  return 0;
}
