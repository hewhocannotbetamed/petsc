/*$Id: zksp.c,v 1.43 2000/01/11 21:03:48 bsmith Exp balay $*/

#include "src/fortran/custom/zpetsc.h"
#include "ksp.h"




#ifdef PETSC_HAVE_FORTRAN_CAPS
#define kspfgmressetmodifypc_      KSPFGMRESSETMODIFYPC
#define kspfgmresmodifypcsles_     KSPFGMRESMODIFYPCSLES
#define kspfgmresmodifypcnochange_ KSPFGMRESMODIFYPCNOCHANGE
#define kspdefaultconverged_       KSPDEFAULTCONVERGED
#define kspdefaultmonitor_         KSPDEFAULTMONITOR
#define ksptruemonitor_            KSPTRUEMONITOR
#define kspvecviewmonitor_         KSPVECVIEWMONITOR
#define ksplgmonitor_              KSPLGMONITOR
#define ksplgtruemonitor_          KSPLGTRUEMONITOR
#define kspsingularvaluemonitor_   KSPSINGULARVALUEMONITOR
#define kspregisterdestroy_        KSPREGISTERDESTROY
#define kspdestroy_                KSPDESTROY
#define ksplgmonitordestroy_       KSPLGMONITORDESTROY
#define ksplgmonitorcreate_        KSPLGMONITORCREATE
#define kspgetrhs_                 KSPGETRHS
#define kspgetsolution_            KSPGETSOLUTION
#define kspgetpc_                  KSPGETPC
#define kspsetmonitor_             KSPSETMONITOR
#define kspsetconvergencetest_     KSPSETCONVERGENCETEST
#define kspcreate_                 KSPCREATE
#define kspsetoptionsprefix_       KSPSETOPTIONSPREFIX
#define kspappendoptionsprefix_    KSPAPPENDOPTIONSPREFIX
#define kspgettype_                KSPGETTYPE
#define kspgetpreconditionerside_  KSPGETPRECONDITIONERSIDE
#define kspbuildsolution_          KSPBUILDSOLUTION
#define kspsettype_                KSPSETTYPE           
#define kspgetresidualhistory_     KSPGETRESIDUALHISTORY
#define kspgetoptionsprefix_       KSPGETOPTIONSPREFIX
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define kspfgmressetmodifypc_      kspfgmressetmodifypc
#define kspfgmresmodifypcsles_     kspfgmresmodifypcsles
#define kspfgmresmodifypcnochange_ kspfgmresmodifypcnochange
#define kspdefaultconverged_       kspdefaultconverged
#define kspsingularvaluemonitor_   kspsingularvaluemonitor
#define kspdefaultmonitor_         kspdefaultmonitor
#define ksptruemonitor_            ksptruemonitor
#define kspvecviewmonitor_         kspvecviewmonitor
#define ksplgmonitor_              ksplgmonitor
#define ksplgtruemonitor_          ksplgtruemonitor
#define kspgetresidualhistory_     kspgetresidualhistory
#define kspsettype_                kspsettype
#define kspregisterdestroy_        kspregisterdestroy
#define kspdestroy_                kspdestroy
#define ksplgmonitordestroy_       ksplgmonitordestroy
#define ksplgmonitorcreate_        ksplgmonitorcreate
#define kspgetrhs_                 kspgetrhs
#define kspgetsolution_            kspgetsolution
#define kspgetpc_                  kspgetpc
#define kspsetmonitor_             kspsetmonitor
#define kspsetconvergencetest_     kspsetconvergencetest
#define kspcreate_                 kspcreate
#define kspsetoptionsprefix_       kspsetoptionsprefix
#define kspappendoptionsprefix_    kspappendoptionsprefix
#define kspgettype_                kspgettype
#define kspgetpreconditionerside_  kspgetpreconditionerside
#define kspbuildsolution_          kspbuildsolution
#define kspgetoptionsprefix_       kspgetoptionsprefix
#endif

EXTERN_C_BEGIN
/* function */
void kspdefaultconverged_(KSP *ksp,int *n,double *rnorm,KSPConvergedReason *flag,void *dummy,int *ierr)
{
  if (FORTRANNULLOBJECT(dummy)) dummy = PETSC_NULL;
  *ierr = KSPDefaultConverged(*ksp,*n,*rnorm,flag,dummy);
}

void PETSC_STDCALL kspgetresidualhistory_(KSP *ksp,int *na,int *ierr)
{
  *ierr = KSPGetResidualHistory(*ksp,PETSC_NULL,na);
}

void PETSC_STDCALL kspsettype_(KSP *ksp,CHAR type PETSC_MIXED_LEN(len),int *ierr PETSC_END_LEN(len))
{
  char *t;

  FIXCHAR(type,len,t);
  *ierr = KSPSetType(*ksp,t);
  FREECHAR(type,t);
}

void PETSC_STDCALL kspgettype_(KSP *ksp,CHAR name PETSC_MIXED_LEN(len),int *ierr PETSC_END_LEN(len))
{
  char *tname;

  *ierr = KSPGetType(*ksp,&tname);if (*ierr) return;
#if defined(PETSC_USES_CPTOFCD)
  {
    char *t = _fcdtocp(name); int len1 = _fcdlen(name);
    *ierr = PetscStrncpy(t,tname,len1); 
  }
#else
  *ierr = PetscStrncpy(name,tname,len);
#endif
}

void PETSC_STDCALL kspgetpreconditionerside_(KSP *ksp,PCSide *side,int *ierr){
*ierr = KSPGetPreconditionerSide(*ksp,side);
}

void PETSC_STDCALL kspsetoptionsprefix_(KSP *ksp,CHAR prefix PETSC_MIXED_LEN(len),
                                        int *ierr PETSC_END_LEN(len))
{
  char *t;

  FIXCHAR(prefix,len,t);
  *ierr = KSPSetOptionsPrefix(*ksp,t);
  FREECHAR(prefix,t);
}

void PETSC_STDCALL kspappendoptionsprefix_(KSP *ksp,CHAR prefix PETSC_MIXED_LEN(len),
                                           int *ierr PETSC_END_LEN(len))
{
  char *t;

  FIXCHAR(prefix,len,t);
  *ierr = KSPAppendOptionsPrefix(*ksp,t);
  FREECHAR(prefix,t);
}

void PETSC_STDCALL kspcreate_(MPI_Comm *comm,KSP *ksp,int *ierr){
  *ierr = KSPCreate((MPI_Comm)PetscToPointerComm(*comm),ksp);
}

static void (*f2)(KSP*,int*,double*,KSPConvergedReason*,void*,int*);
static int ourtest(KSP ksp,int i,double d,KSPConvergedReason *reason,void* ctx)
{
  int ierr;
  (*f2)(&ksp,&i,&d,reason,ctx,&ierr);CHKERRQ(ierr);
  return 0;
}
void PETSC_STDCALL kspsetconvergencetest_(KSP *ksp,
      void (*converge)(KSP*,int*,double*,KSPConvergedReason*,void*,int*),void *cctx,int *ierr)
{
  if ((void *)converge == (void *)kspdefaultconverged_) {
    *ierr = KSPSetConvergenceTest(*ksp,KSPDefaultConverged,0);
  } else {
    f2 = converge;
    *ierr = KSPSetConvergenceTest(*ksp,ourtest,cctx);
  }
}

/*
        These are not usually called from Fortran but allow Fortran users 
   to transparently set these monitors from .F code
   
   functions, hence no STDCALL
*/
void  kspdefaultmonitor_(KSP *ksp,int *it,double *norm,void *ctx,int *ierr)
{
  *ierr = KSPDefaultMonitor(*ksp,*it,*norm,ctx);
}
 
void  kspsingularvaluemonitor_(KSP *ksp,int *it,double *norm,void *ctx,int *ierr)
{
  *ierr = KSPSingularValueMonitor(*ksp,*it,*norm,ctx);
}

void  ksplgmonitor_(KSP *ksp,int *it,double *norm,void *ctx,int *ierr)
{
  *ierr = KSPLGMonitor(*ksp,*it,*norm,ctx);
}

void  ksplgtruemonitor_(KSP *ksp,int *it,double *norm,void *ctx,int *ierr)
{
  *ierr = KSPLGTrueMonitor(*ksp,*it,*norm,ctx);
}

void  ksptruemonitor_(KSP *ksp,int *it,double *norm,void *ctx,int *ierr)
{
  *ierr = KSPTrueMonitor(*ksp,*it,*norm,ctx);
}

void  kspvecviewmonitor_(KSP *ksp,int *it,double *norm,void *ctx,int *ierr)
{
  *ierr = KSPVecViewMonitor(*ksp,*it,*norm,ctx);
}

static void (*f1)(KSP*,int*,double*,void*,int*);
static int ourmonitor(KSP ksp,int i,double d,void* ctx)
{
  int ierr = 0;
  (*f1)(&ksp,&i,&d,ctx,&ierr);CHKERRQ(ierr);
  return 0;
}
static void (*f21)(void*,int*);
static int ourdestroy(void* ctx)
{
  int ierr = 0;
  (*f21)(ctx,&ierr);CHKERRQ(ierr);
  return 0;
}

void PETSC_STDCALL kspsetmonitor_(KSP *ksp,void (*monitor)(KSP*,int*,double*,void*,int*),
                    void *mctx,void (*monitordestroy)(void *,int *),int *ierr)
{
  if ((void*)monitor == (void*)kspdefaultmonitor_) {
    *ierr = KSPSetMonitor(*ksp,KSPDefaultMonitor,0,0);
  } else if ((void*)monitor == (void*)ksplgmonitor_) {
    *ierr = KSPSetMonitor(*ksp,KSPLGMonitor,0,0);
  } else if ((void*)monitor == (void*)ksplgtruemonitor_) {
    *ierr = KSPSetMonitor(*ksp,KSPLGTrueMonitor,0,0);
  } else if ((void*)monitor == (void*)kspvecviewmonitor_) {
    *ierr = KSPSetMonitor(*ksp,KSPVecViewMonitor,0,0);
  } else if ((void*)monitor == (void*)ksptruemonitor_) {
    *ierr = KSPSetMonitor(*ksp,KSPTrueMonitor,0,0);
  } else if ((void*)monitor == (void*)kspsingularvaluemonitor_) {
    *ierr = KSPSetMonitor(*ksp,KSPSingularValueMonitor,0,0);
  } else {
    f1  = monitor;
    if (FORTRANNULLFUNCTION(monitordestroy)) {
      *ierr = KSPSetMonitor(*ksp,ourmonitor,mctx,0);
    } else {
      f21 = monitordestroy;
      *ierr = KSPSetMonitor(*ksp,ourmonitor,mctx,ourdestroy);
    }
  }
}

void PETSC_STDCALL kspgetpc_(KSP *ksp,PC *B,int *ierr)
{
  *ierr = KSPGetPC(*ksp,B);
}

void PETSC_STDCALL kspgetsolution_(KSP *ksp,Vec *v,int *ierr)
{
  *ierr = KSPGetSolution(*ksp,v);
}

void PETSC_STDCALL kspgetrhs_(KSP *ksp,Vec *r,int *ierr)
{
  *ierr = KSPGetRhs(*ksp,r);
}

/*
   Possible bleeds memory but cannot be helped.
*/
void PETSC_STDCALL ksplgmonitorcreate_(CHAR host PETSC_MIXED_LEN(len1),
                    CHAR label PETSC_MIXED_LEN(len2),int *x,int *y,int *m,int *n,DrawLG *ctx,
                    int *ierr PETSC_END_LEN(len1) PETSC_END_LEN(len2))
{
  char   *t1,*t2;

  FIXCHAR(host,len1,t1);
  FIXCHAR(label,len2,t2);
  *ierr = KSPLGMonitorCreate(t1,t2,*x,*y,*m,*n,ctx);
}

void PETSC_STDCALL ksplgmonitordestroy_(DrawLG *ctx,int *ierr)
{
  *ierr = KSPLGMonitorDestroy(*ctx);
}

void PETSC_STDCALL kspdestroy_(KSP *ksp,int *ierr)
{
  *ierr = KSPDestroy(*ksp);
}

void PETSC_STDCALL kspregisterdestroy_(int* ierr)
{
  *ierr = KSPRegisterDestroy();
}

void PETSC_STDCALL kspbuildsolution_(KSP *ctx,Vec *v,Vec *V,int *ierr)
{
  *ierr = KSPBuildSolution(*ctx,*v,V);
}

void PETSC_STDCALL kspbuildresidual_(KSP *ctx,Vec *t,Vec *v,Vec *V,int *ierr)
{
  *ierr = KSPBuildResidual(*ctx,*t,*v,V);
}

void PETSC_STDCALL kspgetoptionsprefix_(KSP *ksp,CHAR prefix PETSC_MIXED_LEN(len),int *ierr PETSC_END_LEN(len))
{
  char *tname;

  *ierr = KSPGetOptionsPrefix(*ksp,&tname);
#if defined(PETSC_USES_CPTOFCD)
  {
    char *t = _fcdtocp(prefix); int len1 = _fcdlen(prefix);
    *ierr = PetscStrncpy(t,tname,len1); if (*ierr) return;
  }
#else
  *ierr = PetscStrncpy(prefix,tname,len); if (*ierr) return;
#endif
}

static void (*f109)(KSP*,int*,int*,double*,void*,int*);
static int ourmodify(KSP ksp,int i,int i2,double d,void* ctx)
{
  int ierr = 0;
  (*f109)(&ksp,&i,&i2,&d,ctx,&ierr);CHKERRQ(ierr);
  return 0;
}

static void (*f210)(void*,int*);
static int ourmoddestroy(void* ctx)
{
  int ierr = 0;
  (*f210)(ctx,&ierr);CHKERRQ(ierr);
  return 0;
}

void PETSC_STDCALL kspfgmresmodifypcnochange_(KSP *ksp,int *total_its,int *loc_its,double *res_norm,void* dummy,int *__ierr)
{
  *__ierr = KSPFGMRESModifyPCNoChange(*ksp,*total_its,*loc_its,*res_norm,dummy);
}

void PETSC_STDCALL kspfgmresmodifypcsles_(KSP *ksp,int *total_its,int *loc_its,double *res_norm,void*dummy,int *__ierr)
{
  *__ierr = KSPFGMRESModifyPCSLES(*ksp,*total_its,*loc_its,*res_norm,dummy);
}

void PETSC_STDCALL kspfgmressetmodifypc_(KSP *ksp,void (*fcn)(KSP*,int*,int*,double*,void*,int*),void* ctx,void (*d)(void*,int*),int *__ierr)
{
  if ((void*)fcn == (void*)kspfgmresmodifypcsles_) {
    *__ierr = KSPFGMRESSetModifyPC(*ksp,KSPFGMRESModifyPCSLES,0,0);
  } else if ((void*)fcn == (void*)kspfgmresmodifypcnochange_) {
    *__ierr = KSPFGMRESSetModifyPC(*ksp,KSPFGMRESModifyPCNoChange,0,0);
  } else {
    f109 = fcn;
    if (FORTRANNULLFUNCTION(d)) {
      *__ierr = KSPFGMRESSetModifyPC(*ksp,ourmodify,ctx,0);
    } else {
      f210 = d;
      *__ierr = KSPFGMRESSetModifyPC(*ksp,ourmodify,ctx,ourmoddestroy);
    }
  }
}

EXTERN_C_END
