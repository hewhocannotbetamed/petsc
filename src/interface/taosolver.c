#define TAOSOLVER_DLL

#include "include/private/taosolver_impl.h" /*I "taosolver.h" I*/

PetscBool TaoSolverRegisterAllCalled = PETSC_FALSE;
PetscFList TaoSolverList = PETSC_NULL;

PetscClassId TAOSOLVER_CLASSID;
PetscLogEvent TaoSolver_Solve, TaoSolver_ObjectiveEval, TaoSolver_GradientEval, TaoSolver_ObjGradientEval, TaoSolver_HessianEval, TaoSolver_ConstraintsEval, TaoSolver_JacobianEval;


#undef __FUNCT__
#define __FUNCT__ "TaoSolverCreate"
/*@
  TaoSolverCreate - Creates a TAO solver

  Collective on MPI_Comm

  Input Parameter:
. comm - MPI communicator

  Output Parameter:
. newtao - the new TaoSolver context

  Available methods include:
+    tao_nls - Newton's method with line search for unconstrained minimization
.    tao_ntr - Newton's method with trust region for unconstrained minimization
.    tao_ntl - Newton's method with trust region, line search for unconstrained minimization
.    tao_lmvm - Limited memory variable metric method for unconstrained minimization
.    tao_cg - Nonlinear conjugate gradient method for unconstrained minimization
.    tao_nm - Nelder-Mead algorithm for derivate-free unconstrained minimization
.    tao_pounder - Model-based algorithm for derivate-free unconstrained minimiz
ation
.    tao_tron - Newton Trust Region method for bound constrained minimization
.    tao_gpcg - Newton Trust Region method for quadratic bound constrained minimization
.    tao_blmvm - Limited memory variable metric method for bound constrained minimization
-    tao_pounders - Model-based algorithm pounder extended for nonlinear least squares 

   Options Database Keys:
+   -tao_method - select which method TAO should use
-   -tao_type - identical to -tao_method

   Level: beginner

.seealso: TaoSolverSolve(), TaoSolverDestroy()
@*/
PetscErrorCode TaoSolverCreate(MPI_Comm comm, TaoSolver *newtao)
{
    PetscErrorCode ierr;
    TaoSolver tao;
    
    PetscFunctionBegin;
    PetscValidPointer(newtao,2);
    *newtao = PETSC_NULL;

#ifndef PETSC_USE_DYNAMIC_LIBRARIES
    ierr = TaoSolverInitializePackage(PETSC_NULL); CHKERRQ(ierr);
#endif

    ierr = PetscHeaderCreate(tao,_p_TaoSolver, struct _TaoSolverOps, TAOSOLVER_CLASSID,0,"TaoSolver",comm,TaoSolverDestroy,TaoSolverView); CHKERRQ(ierr);
    
    tao->ops->computeobjective=0;
    tao->ops->computeobjectiveandgradient=0;
    tao->ops->computegradient=0;
    tao->ops->computehessian=0;
    tao->ops->computeseparableobjective=0;
    tao->ops->computeconstraints=0;
    tao->ops->computejacobian=0;
    tao->ops->convergencetest=TaoSolverDefaultConvergenceTest;
    tao->ops->convergencedestroy=0;
    tao->ops->computedual=0;
    tao->ops->setup=0;
    tao->ops->solve=0;
    tao->ops->view=0;
    tao->ops->setfromoptions=0;
    tao->ops->destroy=0;

    tao->solution=PETSC_NULL;
    tao->gradient=PETSC_NULL;
    tao->sep_objective = PETSC_NULL;
    tao->constraints=PETSC_NULL;
    tao->stepdirection=PETSC_NULL;
    tao->XL = PETSC_NULL;
    tao->XU = PETSC_NULL;
    tao->hessian = PETSC_NULL;
    tao->jacobian = PETSC_NULL;

    tao->max_its     = 10000;
    tao->max_funcs   = 10000;
    tao->fatol       = 1e-8;
    tao->frtol       = 1e-8;
    tao->gatol       = 1e-8;
    tao->grtol       = 1e-8;
    tao->gttol       = 0.0;
    tao->catol       = 0.0;
    tao->crtol       = 0.0;
    tao->xtol        = 0.0;
    tao->trtol       = 0.0;
    tao->fmin        = -1e100;
    tao->hist_reset = PETSC_TRUE;
    tao->hist_max = 0;
    tao->hist_len = 0;
    tao->hist_obj = PETSC_NULL;
    tao->hist_resid = PETSC_NULL;
    tao->hist_cnorm = PETSC_NULL;

    tao->numbermonitors=0;
    tao->viewsolution=PETSC_FALSE;
    tao->viewhessian=PETSC_FALSE;
    tao->viewgradient=PETSC_FALSE;
    tao->viewjacobian=PETSC_FALSE;
    tao->viewconstraints = PETSC_FALSE;
    tao->viewtao = PETSC_FALSE;
    
    ierr = TaoSolverResetStatistics(tao); CHKERRQ(ierr);


    *newtao = tao; 
    PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "TaoSolverSolve"
/*@ 
  TaoSolverSolve - Solves an optimization problem min F(x) s.t. l <= x <= u

  Collective on TaoSolver
  
  Input Parameters:
. tao - the TaoSolver context

  Notes:
  The user must set up the TaoSolver with calls to TaoSolverSetInitialVector(),
  TaoSolverSetObjectiveRoutine(),
  TaoSolverSetGradientRoutine(), and (if using 2nd order method) TaoSolverSetHessianRoutine().

  Level: beginner
  .seealso: TaoSolverCreate(), TaoSolverSetObjectiveRoutine(), TaoSolverSetGradientRoutine(), TaoSolverSetHessianRoutine()
 @*/
PetscErrorCode TaoSolverSolve(TaoSolver tao)
{
  PetscErrorCode ierr;
  char           filename[PETSC_MAX_PATH_LEN];
  PetscBool      flg;
  PetscViewer    viewer;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);

  ierr = TaoSolverSetUp(tao);CHKERRQ(ierr);
  ierr = TaoSolverResetStatistics(tao); CHKERRQ(ierr);

  ierr = PetscLogEventBegin(TaoSolver_Solve,tao,0,0,0); CHKERRQ(ierr);
  if (tao->ops->solve){ ierr = (*tao->ops->solve)(tao);CHKERRQ(ierr); }
  ierr = PetscLogEventEnd(TaoSolver_Solve,tao,0,0,0); CHKERRQ(ierr);


  
  ierr = PetscOptionsGetString(((PetscObject)tao)->prefix,"-tao_view",filename,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
  if (flg && !PetscPreLoadingOn) {
    ierr = PetscViewerASCIIOpen(((PetscObject)tao)->comm,filename,&viewer);CHKERRQ(ierr);
    ierr = TaoSolverView(tao,viewer);CHKERRQ(ierr); 
    ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
  }
  

  if (tao->printreason) { 
      if (tao->reason > 0) {
	  ierr = PetscPrintf(((PetscObject)tao)->comm,"TAO solve converged due to %s\n",TaoSolverTerminationReasons[tao->reason]); CHKERRQ(ierr);
      } else {
	  ierr = PetscPrintf(((PetscObject)tao)->comm,"TAO solve did not converge due to %s\n",TaoSolverTerminationReasons[tao->reason]); CHKERRQ(ierr);
      }
  }
  

  PetscFunctionReturn(0);

    
}


#undef __FUNCT__
#define __FUNCT__ "TaoSolverSetUp"
/*@ 
  TaoSolverSetUp - Sets up the internal data structures for the later use
  of a Tao solver

  Collective on tao
  
  Input Parameters:
. tao - the TAO context

  Notes:
  The user will not need to explicitly call TaoSolverSetUp(), as it will 
  automatically be called in TaoSolverSolve().  However, if the user
  desires to call it explicitly, it should come after TaoSolverCreate()
  and TaoSolverSetXXX(), but before TaoSolverSolve().

  Level: advanced

.seealso: TaoSolverCreate(), TaoSolverSolve()
@*/
PetscErrorCode TaoSolverSetUp(TaoSolver tao)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao, TAOSOLVER_CLASSID,1); 
  if (tao->setupcalled) PetscFunctionReturn(0);

  if (!tao->solution) {
      SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Must call TaoSolverSetInitialVector");
  }
  if (!tao->ops->computeobjective && !tao->ops->computeobjectiveandgradient &&
      !tao->ops->computeseparableobjective) {
      SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Must call TaoSolverSetObjective or TaoSolverSetObjectiveAndGradient");
  }
  ierr = TaoSolverComputeVariableBounds(tao); CHKERRQ(ierr);
  if (tao->ops->setup) {
    ierr = (*tao->ops->setup)(tao); CHKERRQ(ierr);
  }
  
  tao->setupcalled = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverDestroy"
/*@ 
  TaoSolverDestroy - Destroys the TAO context that was created with 
  TaoSolverCreate()

  Collective on TaoSolver

  Input Parameter
. tao - the TaoSolver context

  Level: beginner

.seealse: TaoSolverCreate(), TaoSolverSolve()
@*/
PetscErrorCode TaoSolverDestroy(TaoSolver tao)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);

  if (--((PetscObject)tao)->refct > 0) PetscFunctionReturn(0);

  ierr = PetscObjectDepublish(tao); CHKERRQ(ierr);
  
  if (tao->ops->destroy) {
      ierr = (*tao->ops->destroy)(tao); CHKERRQ(ierr);
  }
//  ierr = TaoSolverMonitorCancel(tao); CHKERRQ(ierr);
  if (tao->ksp) {
    ierr = KSPDestroy(&tao->ksp); CHKERRQ(ierr);
    tao->ksp = PETSC_NULL;
  }
  if (tao->linesearch) {
    ierr = TaoLineSearchDestroy(tao->linesearch); CHKERRQ(ierr);
    tao->linesearch = PETSC_NULL;
  }
  if (tao->ops->convergencedestroy) {
      ierr = (*tao->ops->convergencedestroy)(tao->cnvP); CHKERRQ(ierr);
  }
  if (tao->solution) {
    ierr = VecDestroy(&tao->solution); CHKERRQ(ierr);
    tao->solution = PETSC_NULL;
  }
  if (tao->gradient) {
    ierr = VecDestroy(&tao->gradient); CHKERRQ(ierr);
    tao->gradient = PETSC_NULL;
  }
  if (tao->XL) {
    ierr = VecDestroy(&tao->XL); CHKERRQ(ierr);
    tao->XL = PETSC_NULL;
  }
  if (tao->XU) {
    ierr = VecDestroy(&tao->XU); CHKERRQ(ierr);
    tao->XU = PETSC_NULL;
  }
  if (tao->stepdirection) {
    ierr = VecDestroy(&tao->stepdirection); CHKERRQ(ierr);
    tao->stepdirection = PETSC_NULL;
  }
  if (tao->hessian_pre) {
    ierr = MatDestroy(&tao->hessian_pre); CHKERRQ(ierr);
    tao->hessian_pre = PETSC_NULL;
  }
  if (tao->hessian) {
    ierr = MatDestroy(&tao->hessian); CHKERRQ(ierr);
    tao->hessian = PETSC_NULL;
  }
  if (tao->jacobian_pre) {
    ierr = MatDestroy(&tao->jacobian_pre); CHKERRQ(ierr);
    tao->jacobian_pre = PETSC_NULL;
  }
  if (tao->jacobian) {
    ierr = MatDestroy(&tao->jacobian); CHKERRQ(ierr);
    tao->jacobian = PETSC_NULL;
  }
  ierr = PetscHeaderDestroy(&tao); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverSetFromOptions"
/*@
  TaoSolverSetFromOptions - Sets various TaoSolver parameters from user
  options.

  Collective on TaoSolver

  Input Paremeter:
. tao - the TaoSolver solver context

  Options Database Keys:
+ -tao_method <type> - The algorithm that TAO uses (tao_lmvm, tao_nls, etc.)
. -tao_fatol <fatol>
. -tao_frtol <frtol>
. -tao_gatol <gatol>
. -tao_grtol <grtol>
. -tao_gttol <gttol>
. -tao_no_convergence_test
. -tao_monitor
. -tao_smonitor
. -tao_cmonitor
. -tao_vecmonitor
. -tao_vecmonitor_update
. -tao_view
- -tao_fdgrad


  Notes:
  To see all options, run your program with the -help option or consult the 
  user's manual

  Level: beginner
@*/
PetscErrorCode TaoSolverSetFromOptions(TaoSolver tao)
{
    PetscErrorCode ierr;
    const TaoSolverType default_type = "tao_lmvm";
    const char *prefix;
    char type[256];
    PetscBool flg;
    
    PetscFunctionBegin;
    PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
    ierr = TaoSolverGetOptionsPrefix(tao,&prefix);
    /* So no warnings are given about unused options */
    ierr = PetscOptionsHasName(prefix,"-tao_ksp_type",&flg);
    ierr = PetscOptionsHasName(prefix,"-tao_pc_type",&flg);
    

    ierr = PetscOptionsBegin(((PetscObject)tao)->comm, ((PetscObject)tao)->prefix,"TaoSolver options","TaoSolver"); CHKERRQ(ierr);
    {
							

	if (!TaoSolverRegisterAllCalled) {
	    ierr = TaoSolverRegisterAll(PETSC_NULL); CHKERRQ(ierr);
	}
	if (((PetscObject)tao)->type_name) {
	    default_type = ((PetscObject)tao)->type_name;
	}
	/* Check for type from options */
	ierr = PetscOptionsList("-tao_type","Tao Solver type","TaoSolverSetType",TaoSolverList,default_type,type,256,&flg); CHKERRQ(ierr);
	if (flg) {
	    ierr = TaoSolverSetType(tao,type); CHKERRQ(ierr);
	} else {
	  ierr = PetscOptionsList("-tao_method","Tao Solver type","TaoSolverSetType",TaoSolverList,default_type,type,256,&flg); CHKERRQ(ierr);
	  if (flg) {
  	    ierr = TaoSolverSetType(tao,type); CHKERRQ(ierr);
	  } else if (!((PetscObject)tao)->type_name) {
	    ierr = TaoSolverSetType(tao,default_type);
	  }
	}

	if (tao->ops->setfromoptions) {
	    ierr = (*tao->ops->setfromoptions)(tao); CHKERRQ(ierr);
	}


	ierr = PetscOptionsName("-tao_view","view TAO_SOLVER info after each minimization has completed","TaoView",&flg);CHKERRQ(ierr);
	if (flg) tao->viewtao = PETSC_TRUE;
	ierr = PetscOptionsReal("-tao_fatol","Stop if solution within","TaoSetTolerances",tao->fatol,&tao->fatol,&flg);CHKERRQ(ierr);
	ierr = PetscOptionsReal("-tao_frtol","Stop if relative solution within","TaoSetTolerances",tao->frtol,&tao->frtol,&flg);CHKERRQ(ierr);
	ierr = PetscOptionsReal("-tao_catol","Stop if constraints violations within","TaoSetTolerances",tao->catol,&tao->catol,&flg);CHKERRQ(ierr);
	ierr = PetscOptionsReal("-tao_crtol","Stop if relative contraint violations within","TaoSetTolerances",tao->crtol,&tao->crtol,&flg);CHKERRQ(ierr);
	ierr = PetscOptionsReal("-tao_gatol","Stop if norm of gradient less than","TaoSetGradientTolerances",tao->gatol,&tao->gatol,&flg);CHKERRQ(ierr);
	ierr = PetscOptionsReal("-tao_grtol","Stop if norm of gradient divided by the function value is less than","TaoSetGradientTolerances",tao->grtol,&tao->grtol,&flg);CHKERRQ(ierr); 
	ierr = PetscOptionsReal("-tao_gttol","Stop if the norm of the gradient is less than the norm of the initial gradient times","TaoSetGradientTolerances",tao->gttol,&tao->gttol,&flg);CHKERRQ(ierr); 
	ierr = PetscOptionsInt("-tao_max_its","Stop if iteration number exceeds",
			    "TaoSetMaximumIterates",tao->max_its,&tao->max_its,
			    &flg);CHKERRQ(ierr);
	ierr = PetscOptionsInt("-tao_max_funcs","Stop if number of function evaluations exceeds","TaoSetMaximumFunctionEvaluations",tao->max_funcs,&tao->max_funcs,&flg); CHKERRQ(ierr);
	ierr = PetscOptionsReal("-tao_fmin","Stop if function less than","TaoSetFunctionLowerBound",tao->fmin,&tao->fmin,&flg); CHKERRQ(ierr);
	ierr = PetscOptionsReal("-tao_steptol","Stop if step size or trust region radius less than","TaoSetTrustRegionRadius",tao->trtol,&tao->trtol,&flg);CHKERRQ(ierr);
/*	ierr = PetscOptionsReal("-tao_trust0","Initial trust region radius","TaoSetTrustRegionRadius",tao->trust0,&tao->trust0,&flg);CHKERRQ(ierr); */


/*	ierr = PetscOptionsName("-tao_unitstep","Always use unit step length","TaoCreateUnitLineSearch",&flg); CHKERRQ(ierr); 
	if (flg){ierr=TaoCreateUnitLineSearch(tao);CHKERRQ(ierr);} */
/*	ierr = PetscOptionsName("-tao_lmvmh","User supplies approximate hessian for LMVM solvers","TaoLMVMSetH0",&flg); CHKERRQ(ierr);
	if (flg){ierr=TaoBLMVMSetH0(tao,PETSC_TRUE);CHKERRQ(ierr);ierr=TaoLMVMSetH0(tao,PETSC_TRUE);CHKERRQ(ierr);}*/
  
	ierr = PetscOptionsName("-tao_view_hessian","view Hessian after each evaluation","None",&flg);CHKERRQ(ierr);
	if (flg) tao->viewhessian = PETSC_TRUE;
	ierr = PetscOptionsName("-tao_view_gradient","view gradient after each evaluation","None",&flg);CHKERRQ(ierr);
	if (flg) tao->viewgradient = PETSC_TRUE;
	ierr = PetscOptionsName("-tao_view_jacobian","view jacobian after each evaluation","None",&flg);CHKERRQ(ierr);  
	if (flg) tao->viewjacobian = PETSC_TRUE;
/*	ierr = PetscOptionsName("-tao_view_constraints","view constraint function after each evaluation","None",&flg);CHKERRQ(ierr);  
	if (flg) tao->viewvfunc = PETSC_TRUE; */

/*	ierr = PetscOptionsName("-tao_cancelmonitors","cancel all monitors hardwired in code","TaoSolverClearMonitor",&flg);CHKERRQ(ierr); 
	if (flg) {ierr = TaoSolverClearMonitor(tao);CHKERRQ(ierr);} */
	ierr = PetscOptionsName("-tao_monitor","Use the default convergence monitor","TaoSetMonitor",&flg);CHKERRQ(ierr);
	if (flg) {
	  ierr = TaoSolverSetMonitor(tao,TaoSolverDefaultMonitor,PETSC_NULL);CHKERRQ(ierr);
	}
	ierr = PetscOptionsName("-tao_smonitor","Use short monitor","None",&flg);CHKERRQ(ierr);
	if (flg) {ierr = TaoSolverSetMonitor(tao,TaoSolverDefaultSMonitor,PETSC_NULL);CHKERRQ(ierr);}
	ierr = PetscOptionsName("-tao_cmonitor","Use constraint monitor","None",&flg);CHKERRQ(ierr);
	if (flg) {ierr = TaoSolverSetMonitor(tao,TaoSolverDefaultCMonitor,PETSC_NULL);CHKERRQ(ierr);}
/*	ierr = PetscOptionsName("-tao_vecmonitor","Plot solution vector at each iteration","TaoVecViewMonitor",&flg);CHKERRQ(ierr);
	if (flg) {ierr = TaoSetMonitor(tao,TaoVecViewMonitor,PETSC_NULL);CHKERRQ(ierr);} 
	ierr = PetscOptionsName("-tao_vecmonitor_update","plots step direction at each iteration","TaoVecViewMonitorUpdate",&flg);CHKERRQ(ierr);
	if (flg) {ierr = TaoSetMonitor(tao,TaoVecViewMonitorUpdate,PETSC_NULL);CHKERRQ(ierr);} */
    }    
    ierr = PetscOptionsEnd(); CHKERRQ(ierr);
    PetscFunctionReturn(0);

}


#undef __FUNCT__
#define __FUNCT__ "TaoSolverView"
/*@C
  TaoSolverView - Prints information about the TaoSolver
 
  Collective on TaoSolver

  InputParameters:
+ tao - the TaoSolver context
- viewer - visualization context

  Options Database Key:
. -tao_view - Calls TaoSolverView() at the end of TaoSolverSolve()

  Notes:
  The available visualization contexts include
+     PETSC_VIEWER_STDOUT_SELF - standard output (default)
-     PETSC_VIEWER_STDOUT_WORLD - synchronized standard
         output where only the first processor opens
         the file.  All other processors send their 
         data to the first processor to print. 

  Level: beginner

.seealso: PetscViewerASCIIOpen()
@*/
PetscErrorCode TaoSolverView(TaoSolver tao, PetscViewer viewer)
{
    PetscErrorCode ierr;
    PetscBool isascii,isstring;
    TaoSolverType type;
    PetscFunctionBegin;
    PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
    if (!viewer) {
      ierr = PetscViewerASCIIGetStdout(((PetscObject)tao)->comm,&viewer); CHKERRQ(ierr);
    }
    PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
    PetscCheckSameComm(tao,1,viewer,2);

    ierr = PetscTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii); CHKERRQ(ierr);
    ierr = PetscTypeCompare((PetscObject)viewer,PETSCVIEWERSTRING,&isstring); CHKERRQ(ierr);
    if (isascii) {
	if (((PetscObject)tao)->prefix) {
	    ierr = PetscViewerASCIIPrintf(viewer,"TaoSolver Object:(%s)\n",((PetscObject)tao)->prefix); CHKERRQ(ierr);
        } else {
	    ierr = PetscViewerASCIIPrintf(viewer,"TaoSolver Object:\n"); CHKERRQ(ierr); CHKERRQ(ierr);
	}
	ierr = TaoSolverGetType(tao,&type);
	if (type) {
	    ierr = PetscViewerASCIIPrintf(viewer,"  type: %s\n",type); CHKERRQ(ierr);
	} else {
	    ierr = PetscViewerASCIIPrintf(viewer,"  type: not set yet\n"); CHKERRQ(ierr);
	}
	if (tao->ops->view) {
	    ierr = PetscViewerASCIIPushTab(viewer); CHKERRQ(ierr);
	    ierr = (*tao->ops->view)(tao,viewer); CHKERRQ(ierr);
	    ierr = PetscViewerASCIIPopTab(viewer); CHKERRQ(ierr);
	}
	ierr=PetscViewerASCIIPrintf(viewer,"  convergence tolerances: fatol=%g,",tao->fatol);CHKERRQ(ierr);
	ierr=PetscViewerASCIIPrintf(viewer," frtol=%g\n",tao->frtol);CHKERRQ(ierr);

	ierr=PetscViewerASCIIPrintf(viewer,"  convergence tolerances: gatol=%g,",tao->gatol);CHKERRQ(ierr);
//	ierr=PetscViewerASCIIPrintf(viewer," trtol=%g,",tao->trtol);CHKERRQ(ierr);
	ierr=PetscViewerASCIIPrintf(viewer," gttol=%g\n",tao->gttol);CHKERRQ(ierr);

	ierr = PetscViewerASCIIPrintf(viewer,"  Residual in Function/Gradient:=%e\n",tao->residual);CHKERRQ(ierr);

	if (tao->cnorm>0 || tao->catol>0 || tao->crtol>0){
	    ierr=PetscViewerASCIIPrintf(viewer,"  convergence tolerances:");CHKERRQ(ierr);
	    ierr=PetscViewerASCIIPrintf(viewer," catol=%g,",tao->catol);CHKERRQ(ierr);
	    ierr=PetscViewerASCIIPrintf(viewer," crtol=%g\n",tao->crtol);CHKERRQ(ierr);
	    ierr = PetscViewerASCIIPrintf(viewer,"  Residual in Constraints:=%e\n",tao->cnorm);CHKERRQ(ierr);
	}

	if (tao->trtol>0){
	    ierr=PetscViewerASCIIPrintf(viewer,"  convergence tolerances: trtol=%g\n",tao->trtol);CHKERRQ(ierr);
	    ierr=PetscViewerASCIIPrintf(viewer,"  Final step size/trust region radius:=%g\n",tao->step);CHKERRQ(ierr);
	}

	if (tao->fmin>-1.e25){
	    ierr=PetscViewerASCIIPrintf(viewer,"  convergence tolerances: function minimum=%g\n"
					,tao->fmin);CHKERRQ(ierr);
	}
	ierr = PetscViewerASCIIPrintf(viewer,"  Objective value=%e\n",
				      tao->fc);CHKERRQ(ierr);

	ierr = PetscViewerASCIIPrintf(viewer,"  total number of iterations=%d,          ",
				      tao->niter);CHKERRQ(ierr);
	ierr = PetscViewerASCIIPrintf(viewer,"              (max: %d)\n",tao->max_its);CHKERRQ(ierr);

	if (tao->nfuncs>0){
	    ierr = PetscViewerASCIIPrintf(viewer,"  total number of function evaluations=%d,",
					  tao->nfuncs);CHKERRQ(ierr);
	    ierr = PetscViewerASCIIPrintf(viewer,"                max: %d\n",
					  tao->max_funcs);CHKERRQ(ierr);
	}
	if (tao->ngrads>0){
	    ierr = PetscViewerASCIIPrintf(viewer,"  total number of gradient evaluations=%d,",
					  tao->ngrads);CHKERRQ(ierr);
	    ierr = PetscViewerASCIIPrintf(viewer,"                max: %d\n",
					  tao->max_funcs);CHKERRQ(ierr);
	}
	if (tao->nfuncgrads>0){
	    ierr = PetscViewerASCIIPrintf(viewer,"  total number of function/gradient evaluations=%d,",
					  tao->nfuncgrads);CHKERRQ(ierr);
	    ierr = PetscViewerASCIIPrintf(viewer,"    (max: %d)\n",
					  tao->max_funcs);CHKERRQ(ierr);
	}
	if (tao->nhess>0){
	    ierr = PetscViewerASCIIPrintf(viewer,"  total number of Hessian evaluations=%d\n",
					  tao->nhess);CHKERRQ(ierr);
	}
/*	if (tao->linear_its>0){
	    ierr = PetscViewerASCIIPrintf(viewer,"  total Krylov method iterations=%d\n",
					  tao->linear_its);CHKERRQ(ierr);
					  }*/
	if (tao->nconstraints>0){
	    ierr = PetscViewerASCIIPrintf(viewer,"  total number of constraint function evaluations=%d\n",
					  tao->nconstraints);CHKERRQ(ierr);
	}
	if (tao->njac>0){
	    ierr = PetscViewerASCIIPrintf(viewer,"  total number of Jacobian evaluations=%d\n",
					  tao->njac);CHKERRQ(ierr);
	}

	if (tao->reason>0){
	    ierr = PetscViewerASCIIPrintf(viewer,    "  Solution found: ");CHKERRQ(ierr);
	    switch (tao->reason) {
		case TAO_CONVERGED_ATOL:
		    ierr = PetscViewerASCIIPrintf(viewer," Atol -- F < F_minabs\n"); CHKERRQ(ierr);
		    break;
		case TAO_CONVERGED_RTOL:
		    ierr = PetscViewerASCIIPrintf(viewer," Rtol -- F < F_mintol*F_init\n"); CHKERRQ(ierr);
		    break;
		case TAO_CONVERGED_TRTOL:
		    ierr = PetscViewerASCIIPrintf(viewer," Trtol -- step size small\n"); CHKERRQ(ierr);
		    break;
		case TAO_CONVERGED_MINF:
		    ierr = PetscViewerASCIIPrintf(viewer," Minf -- grad F < grad F_min\n"); CHKERRQ(ierr);
		    break;
		case TAO_CONVERGED_USER:
		    ierr = PetscViewerASCIIPrintf(viewer," User Terminated\n"); CHKERRQ(ierr);
		    break;
		default:
		    break;
	    }		
	    
	} else {
	    ierr = PetscViewerASCIIPrintf(viewer,"  Solver terminated: %d\n",tao->reason);CHKERRQ(ierr);
	}
	
    } else if (isstring) {
	ierr = TaoSolverGetType(tao,&type); CHKERRQ(ierr);
	ierr = PetscViewerStringSPrintf(viewer," %-3.3s",type); CHKERRQ(ierr);
    }
    PetscFunctionReturn(0);
    
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverSetTolerances"
/*@
  TaoSolverSetTolerances - Sets parameters used in TAO convergence tests

  Collective on TaoSolver

  Input Parameters
+ tao - the TaoSolver context
. fatol - absolute convergence tolerance
. frtol - relative convergence tolerance
. gatol - stop if norm of gradient is less than this
. grtol - stop if relative norm of gradient is less than this
- gttol - stop if norm of gradient is reduced by a this factor

  Options Database Keys:
+ -tao_fatol <fatol> - Sets fatol
. -tao_frtol <frtol> - Sets frtol
. -tao_gatol <catol> - Sets gatol
. -tao_grtol <catol> - Sets gatol
- .tao_gttol <crtol> - Sets gttol

  Absolute Stopping Criteria
$ f_{k+1} <= f_k + fatol

  Relative Stopping Criteria
$ f_{k+1} <= f_k + frtol*|f_k|

  Notes: Use PETSC_DEFAULT to leave one or more tolerances unchanged.

  Level: beginner

.seealso: TaoSolverGetTolerances()

@*/
PetscErrorCode TaoSolverSetTolerances(TaoSolver tao, PetscReal fatol, PetscReal frtol, PetscReal gatol, PetscReal grtol, PetscReal gttol)
{
    PetscErrorCode ierr;
    PetscFunctionBegin;
    PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
    
    if (fatol != PETSC_DEFAULT) {
      if (fatol<0) {
	ierr = PetscInfo(tao,"Tried to set negative fatol -- ignored.");
	CHKERRQ(ierr);
      } else {
	tao->fatol = PetscMax(0,fatol);
      }
    }
    
    if (frtol != PETSC_DEFAULT) {
      if (frtol<0) {
	ierr = PetscInfo(tao,"Tried to set negative frtol -- ignored.");
	CHKERRQ(ierr);
      } else {
	tao->frtol = PetscMax(0,frtol);
      }
    }

    if (gatol != PETSC_DEFAULT) {
      if (gatol<0) {
	ierr = PetscInfo(tao,"Tried to set negative gatol -- ignored.");
	CHKERRQ(ierr);
      } else {
	tao->gatol = PetscMax(0,gatol);
      }
    }

    if (grtol != PETSC_DEFAULT) {
      if (grtol<0) {
	ierr = PetscInfo(tao,"Tried to set negative grtol -- ignored.");
	CHKERRQ(ierr);
      } else {
	tao->grtol = PetscMax(0,grtol);
      }
    }

    if (gttol != PETSC_DEFAULT) {
      if (gttol<0) {
	ierr = PetscInfo(tao,"Tried to set negative gttol -- ignored.");
	CHKERRQ(ierr);
      } else {
	tao->gttol = PetscMax(0,gttol);
      }
    }

    PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverGetTolerances"
/*@
  TaoSolverGetTolerances - gets the current values of tolerances

  Collective on TaoSolver

  Input Parameters:
. tao - the TaoSolver context
  
  Output Parameters:
+ fatol - absolute convergence tolerance
. frtol - relative convergence tolerance
. gatol - stop if norm of gradient is less than this
. grtol - stop if relative norm of gradient is less than this
- gttol - stop if norm of gradient is reduced by a this factor

  Notes: Use PETSC_NULL as an argument to ignore one or more tolerances.
.seealso TaoSolverSetTolerances()
 
  Level: intermediate
@*/
PetscErrorCode TaoSolverGetTolerances(TaoSolver tao, PetscReal *fatol, PetscReal *frtol, PetscReal *gatol, PetscReal *grtol, PetscReal *gttol)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
  if (fatol) *fatol=tao->fatol;
  if (frtol) *frtol=tao->frtol;
  if (gatol) *gatol=tao->gatol;
  if (grtol) *grtol=tao->grtol;
  if (gttol) *gttol=tao->gttol;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "TaoSolverGetKSP"
/*@C
  TaoGetKSP - Gets the linear solver used by the optimization solver.
  Application writers should use TaoAppGetKSP if they need direct access
  to the PETSc KSP object.
  
   Input Parameters:
.  tao - the TAO solver

   Output Parameters:
.  ksp - the KSP linear solver used in the optimization solver

   Level: intermediate

.keywords: Application
@*/
PetscErrorCode TaoSolverGetKSP(TaoSolver tao, KSP *ksp) {
  PetscFunctionBegin;
  *ksp = tao->ksp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverGetSolutionVector"
/*@C
  TaoSolverGetSolutionVector - Returns the vector with the current TAO solution

  Input Parameter:
. tao - the TaoSolver context

  Output Parameter:
. X - the current solution

  Level: advanced
 
  Note:  The returned vector will be the same object that was passed into TaoSolverSetInitialSolution()
@*/
PetscErrorCode TaoSolverGetSolutionVector(TaoSolver tao, Vec *X)
{
    PetscFunctionBegin;
    *X = tao->solution;
    PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "TaoSolverResetStatistics"
/*@
   TaoSolverResetStatistics - Initialize the statistics used by TAO for all of the solvers.
   These statistics include the iteration number, residual norms, and convergence status.
   This routine gets called before solving each optimization problem.

   Collective on TaoSolver

   Input Parameters:
.  solver - the TaoSolver context

   Level: developer

.keywords: options, defaults

.seealso: TaoSolverCreate(), TaoSolverSolve()
@*/
PetscErrorCode TaoSolverResetStatistics(TaoSolver tao)
{
    PetscFunctionBegin;
    PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
    tao->niter        = 0;
    tao->nfuncs       = 0;
    tao->nfuncgrads   = 0;
    tao->ngrads       = 0;
    tao->nhess        = 0;
    tao->njac         = 0;
    tao->nconstraints = 0;
    tao->reason       = TAO_CONTINUE_ITERATING;
    tao->residual     = 0.0;
    tao->cnorm        = 0.0;
    tao->step         = 0.0;
    tao->lsflag       = PETSC_FALSE;
    if (tao->hist_reset) tao->hist_len=0;
    PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "TaoSolverSetDefaultMonitors"
/*@
   TaoSolverSetDefaultMonitors - Set the default monitors and viewing options available in TAO.
   This routine is generally called only in TaoSolverCreate().

   Collective on TaoSolver

   Input Parameters:
.  solver - the TaoSolver context

   Level: developer

.seealso: TaoSolverCreate(), TaoSolverResetStatistics(), TaoSolverMonitor()
@*/

PetscErrorCode TaoSolverSetDefaultMonitors(TaoSolver tao)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
  
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverSetMonitor"
/*@C
   TaoSolverSetMonitor - Sets an ADDITIONAL function that is to be used at every
   iteration of the unconstrained minimization solver to display the iteration's 
   progress.   

   Collective on TAO_SOLVER

   Input Parameters:
+  tao - the TAO_SOLVER solver context
.  mymonitor - monitoring routine
-  mctx - [optional] user-defined context for private data for the 
          monitor routine (may be TAO_NULL)

   Calling sequence of mymonitor:
$     int mymonitor(TAO_SOLVER tao,void *mctx)

+    tao - the TAO_SOLVER solver context
-    mctx - [optional] monitoring context


   Options Database Keys:
+    -tao_monitor        - sets TaoDefaultMonitor()
.    -tao_smonitor        - sets short monitor
-    -tao_cancelmonitors - cancels all monitors that have been hardwired into a code by calls to TaoSetMonitor(), but does not cancel those set via the options database.

   Notes: 
   Several different monitoring routines may be set by calling
   TaoSetMonitor() multiple times; all will be called in the 
   order in which they were set.

   Level: intermediate

.keywords: options, monitor, View

.seealso: TaoSolverDefaultMonitor(), TaoSolverClearMonitors(),  TaoSolverSetDestroyRoutine()
@*/
PetscErrorCode TaoSolverSetMonitor(TaoSolver tao, PetscErrorCode (*func)(TaoSolver, void*), void *ctx)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
  if (tao->numbermonitors >= MAXTAOMONITORS) {
      SETERRQ1(PETSC_COMM_SELF,1,"Cannot attach another monitor -- max=",MAXTAOMONITORS);
  }
  tao->monitor[tao->numbermonitors] = func;
  tao->monitorcontext[tao->numbermonitors] = ctx;
  ++tao->numbermonitors;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "TaoSolverDefaultMonitor"
/*@C
   TaoSolverDefaultMonitor - Default routine for monitoring progress of the 
   TaoSolver solvers (default).  This monitor prints the function value and gradient
   norm at each iteration.  It can be turned on from the command line using the 
   -tao_smonitor option

   Collective on TaoSolver

   Input Parameters:
+  tao - the TaoSolver context
-  dummy - unused context

   Options Database Keys:
.  -tao_monitor

   Level: advanced

.seealso: TaoSolverDefaultSMonitor(), TaoSolverSetMonitor
@*/
PetscErrorCode TaoSolverDefaultMonitor(TaoSolver tao, void *dummy)
{
  PetscErrorCode ierr;
  PetscInt its;
  PetscReal fct,gnorm;

  PetscFunctionBegin;
  its=tao->niter;
  fct=tao->fc;
  gnorm=tao->residual;
  ierr=PetscPrintf(((PetscObject)tao)->comm,"iter = %d,",its); CHKERRQ(ierr);
  ierr=PetscPrintf(((PetscObject)tao)->comm," Function value: %12.10e,",fct); CHKERRQ(ierr);
  ierr=PetscPrintf(((PetscObject)tao)->comm,"  Residual: %12.10e \n",gnorm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}



#undef __FUNCT__
#define __FUNCT__ "TaoSolverDefaultSMonitor"
/*@C
   TaoSolverDefaultSMonitor - Default routine for monitoring progress of the 

   same as TaoDefaultMonitor() except
   it prints fewer digits of the residual as the residual gets smaller.
   This is because the later digits are meaningless and are often 
   different on different machines; by using this routine different 
   machines will usually generate the same output.
   
   Collective on TaoSolver

   Input Parameters:
+  tao - the TaoSolver context
-  dummy - unused context

   Options Database Keys:
.  -tao_smonitor

   Level: advanced

.seealso: TaoSolverDefaultMonitor(), TaoSolverSetMonitor()
@*/
PetscErrorCode TaoSolverDefaultSMonitor(TaoSolver tao, void *dummy)
{
  PetscErrorCode ierr;
  PetscInt its;
  PetscReal fct,gnorm;

  PetscFunctionBegin;
  its=tao->niter;
  fct=tao->fc;
  gnorm=tao->residual;
  ierr=PetscPrintf(((PetscObject)tao)->comm,"iter = %d,",its); CHKERRQ(ierr);
  ierr=PetscPrintf(((PetscObject)tao)->comm," Function value %g,",fct); CHKERRQ(ierr);
  if (gnorm > 1.e-6) {
    ierr=PetscPrintf(((PetscObject)tao)->comm," Residual: %7.6f \n",gnorm);CHKERRQ(ierr);
  } else if (gnorm > 1.e-11) {
    ierr=PetscPrintf(((PetscObject)tao)->comm," Residual: < 1.0e-6 \n"); CHKERRQ(ierr);
  } else {
    ierr=PetscPrintf(((PetscObject)tao)->comm," Residual: < 1.0e-11 \n");CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "TaoSolverDefaultCMonitor"
/*@C
   TaoSolverDefaultCMonitor - Default routine for monitoring progress of the 

   same as TaoDefaultMonitor() except
   it prints the norm of the constraints function.
   
   Collective on TaoSolver

   Input Parameters:
+  tao - the TaoSolver context
-  dummy - unused context

   Options Database Keys:
.  -tao_cmonitor

   Level: advanced

.seealso: TaoSolverDefaultMonitor(), TaoSolverSetMonitor()
@*/
PetscErrorCode TaoSolverDefaultCMonitor(TaoSolver tao, void *dummy)
{
  PetscErrorCode ierr;
  PetscInt its;
  PetscReal fct,gnorm;

  PetscFunctionBegin;
  its=tao->niter;
  fct=tao->fc;
  gnorm=tao->residual;
  ierr=PetscPrintf(((PetscObject)tao)->comm,"iter = %d,",its); CHKERRQ(ierr);
  ierr=PetscPrintf(((PetscObject)tao)->comm," Function value: %12.10e,",fct); CHKERRQ(ierr);
  ierr=PetscPrintf(((PetscObject)tao)->comm,"  Residual: %12.10e ",gnorm);CHKERRQ(ierr);
  ierr = PetscPrintf(((PetscObject)tao)->comm,"  Constraint: %12.10e \n",tao->cnorm); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "TaoSolverDefaultConvergenceTest"
/*@ 
   TaoSolverDefaultConvergenceTest - Determines whether the solver should continue iterating
   or terminate. 

   Collective on TAO_SOLVER

   Input Parameters:
+  tao - the TAO_SOLVER context
-  dummy - unused dummy context

   Output Parameter:
.  reason - for terminating

   Notes:
   This routine checks the residual in the optimality conditions, the 
   relative residual in the optimity conditions, the number of function
   evaluations, and the function value to test convergence.  Some
   solvers may use different convergence routines.

   Level: developer

.seealso: TaoSolverSetTolerances(),TaoSolverGetTerminationReason(),TaoSolverSetTerminationReason()
@*/

PetscErrorCode TaoSolverDefaultConvergenceTest(TaoSolver tao,void *dummy)
{
  PetscInt niter=tao->niter, nfuncs=tao->nfuncs, max_funcs=tao->max_funcs;
  PetscReal gnorm=tao->residual, gnorm0=tao->gnorm0;
  PetscReal f=tao->fc, trtol=tao->trtol,trradius=tao->step;
  PetscReal gatol=tao->gatol,grtol=tao->grtol,gttol=tao->gttol;
  PetscReal fatol=tao->fatol,frtol=tao->frtol,catol=tao->catol,crtol=tao->crtol;
  PetscReal fmin=tao->fmin, cnorm=tao->cnorm, cnorm0=tao->cnorm0;
  PetscReal gnorm2;
  TaoSolverTerminationReason reason=TAO_CONTINUE_ITERATING;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao, TAOSOLVER_CLASSID,1);

  gnorm2=gnorm*gnorm;

  if (PetscIsInfOrNanReal(f)) {
    ierr = PetscInfo(tao,"Failed to converged, function value is Inf or NaN\n"); CHKERRQ(ierr);
    reason = TAO_DIVERGED_NAN;
  } else if (f <= fmin && cnorm <=catol) {
    ierr = PetscInfo2(tao,"Converged due to function value %g < minimum function value %g\n", f,fmin); CHKERRQ(ierr);
    reason = TAO_CONVERGED_MINF;
  } else if (gnorm2 <= fatol && cnorm <=catol) {
    ierr = PetscInfo2(tao,"Converged due to residual norm %g < %g\n",gnorm2,fatol); CHKERRQ(ierr);
    reason = TAO_CONVERGED_ATOL;
  } else if (gnorm2 / PetscAbsReal(f+1.0e-10)<= frtol && cnorm/PetscMax(cnorm0,1.0) <= crtol) {
    ierr = PetscInfo2(tao,"Converged due to relative residual norm %g < %g\n",gnorm2/PetscAbsReal(f+1.0e-10),frtol); CHKERRQ(ierr);
    reason = TAO_CONVERGED_RTOL;
  } else if (gnorm<= gatol && cnorm <=catol) {
    ierr = PetscInfo2(tao,"Converged due to residual norm %g < %g\n",gnorm,gatol); CHKERRQ(ierr);
    reason = TAO_CONVERGED_ATOL;
  } else if ( f!=0 && PetscAbsReal(gnorm/f) <= grtol && cnorm <= crtol) {
    ierr = PetscInfo3(tao,"Converged due to residual norm %g < |%g| %g\n",gnorm,f,grtol); CHKERRQ(ierr);
    reason = TAO_CONVERGED_ATOL;
  } else if (gnorm/gnorm0 <= gttol && cnorm <= crtol) {
    ierr = PetscInfo2(tao,"Converged due to relative residual norm %g < %g\n",gnorm/gnorm0,gttol); CHKERRQ(ierr);
    reason = TAO_CONVERGED_RTOL;
  } else if (nfuncs > max_funcs){
    ierr = PetscInfo2(tao,"Exceeded maximum number of function evaluations: %d > %d\n", nfuncs,max_funcs); CHKERRQ(ierr);
    reason = TAO_DIVERGED_MAXFCN;
  } else if ( tao->lsflag != 0 ){
    ierr = PetscInfo(tao,"Tao Line Search failure.\n"); CHKERRQ(ierr);
    reason = TAO_DIVERGED_LS_FAILURE;
  } else if (trradius < trtol && niter > 0){
    ierr = PetscInfo2(tao,"Trust region/step size too small: %g < %g\n", trradius,trtol); CHKERRQ(ierr);
    reason = TAO_CONVERGED_TRTOL;
  } else if (niter > tao->max_its) {
      ierr = PetscInfo2(tao,"Exceeded maximum number of iterations: %d > %d\n",niter,tao->max_its);
      reason = TAO_DIVERGED_MAXITS;
  } else {
    reason = TAO_CONTINUE_ITERATING;
  }
  tao->reason = reason;

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverSetOptionsPrefix"
/*@C
   TaoSolverSetOptionsPrefix - Sets the prefix used for searching for all
   TAO options in the database.


   Collective on TaoSolver

   Input Parameters:
+  tao - the TaoSolver context
-  prefix - the prefix string to prepend to all TAO option requests

   Notes:
   A hyphen (-) must NOT be given at the beginning of the prefix name.
   The first character of all runtime options is AUTOMATICALLY the hyphen.

   For example, to distinguish between the runtime options for two
   different TAO solvers, one could call
.vb
      TaoSolverSetOptionsPrefix(tao1,"sys1_")
      TaoSolverSetOptionsPrefix(tao2,"sys2_")
.ve

   This would enable use of different options for each system, such as
.vb
      -sys1_tao_method blmvm -sys1_tao_gtol 1.e-3
      -sys2_tao_method lmvm  -sys2_tao_gtol 1.e-4
.ve


   Level: advanced

.keywords: options

.seealso: TaoSolverAppendOptionsPrefix(), TaoSolverGetOptionsPrefix()
@*/

PetscErrorCode TaoSolverSetOptionsPrefix(TaoSolver tao, const char p[])
{
  PetscObjectSetOptionsPrefix((PetscObject)tao,p);
  return(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverAppendOptionsPrefix"
/*@C
   TaoSolverAppendOptionsPrefix - Appends to the prefix used for searching for all
   TAO options in the database.


   Collective on TaoSolver

   Input Parameters:
+  tao - the TaoSolver solver context
-  prefix - the prefix string to prepend to all TAO option requests

   Notes:
   A hyphen (-) must NOT be given at the beginning of the prefix name.
   The first character of all runtime options is AUTOMATICALLY the hyphen.


   Level: advanced

.keywords: options

.seealso: TaoSolverSetOptionsPrefix(), TaoSolverGetOptionsPrefix()
@*/
PetscErrorCode TaoSolverAppendOptionsPrefix(TaoSolver tao, const char p[])
{
  PetscObjectAppendOptionsPrefix((PetscObject)tao,p);
  return(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverGetOptionsPrefix"
/*@C
  TaoSolverGetOptionsPrefix - Gets the prefix used for searching for all 
  TAO options in the database

  Not Collective

  Input Parameters:
. tao - the TaoSolver context
  
  Output Parameters:
. prefix - pointer to the prefix string used is returned

  Notes: On the fortran side, the user should pass in a string 'prefix' of
  sufficient length to hold the prefix.

  Level: advanced

.keywords: options

.seealso: TaoSolverSetOptionsPrefix(), TaoSolverAppendOptionsPrefix()
@*/
PetscErrorCode TaoSolverGetOptionsPrefix(TaoSolver tao, const char *p[])
{
   PetscObjectGetOptionsPrefix((PetscObject)tao,p);
   return 0;
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverSetDefaultKSPType"
/*@
  TaoSolverSetDefaultKSPType - Sets the default KSP type if a KSP object
  is created.

  Collective on TaoSolver

  InputParameters:
+ tao - the TaoSolver context
- ktype - the KSP type TAO will use by default

  Note: Some solvers may require a particular KSP type and will not work 
  correctly if the default value is changed

  Options Database Key:
- tao_ksp_type

  Level: advanced
@*/
PetscErrorCode TaoSolverSetDefaultKSPType(TaoSolver tao, KSPType ktype)
{
  PetscFunctionBegin;
  const char *prefix=0;
  char *option=0;
  size_t n1,n2;
  PetscErrorCode ierr;
  PetscFunctionBegin;
  
  PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
  ierr = TaoSolverGetOptionsPrefix(tao,&prefix); CHKERRQ(ierr);
  ierr = PetscStrlen(prefix,&n1);
  ierr = PetscStrlen("_ksp_type",&n2);
  ierr = PetscMalloc(n1+n2+1,&option);
  ierr = PetscStrncpy(option,prefix,n1+1);
  ierr = PetscStrncat(option,"_ksp_type",n2+1);
  ierr = PetscOptionsSetValue(option,ktype); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverSetDefaultPCType"
/*@
  TaoSolverSetDefaultPCType - Sets the default KSP type if a KSP object
  is created.

  Collective on TaoSolver

  InputParameters:
+ tao - the TaoSolver context
- pctype - the preconditioner type TAO will use by default

  Note: Some solvers may require a particular PC type and will not work 
  correctly if the default value is changed

  Options Database Key:
- tao_pc_type

  Level: advanced
@*/
PetscErrorCode TaoSolverSetDefaultPCType(TaoSolver tao, PCType pctype)
{
  
  const char *prefix=0;
  char *option=0;
  size_t n1,n2;
  PetscErrorCode ierr;
  PetscFunctionBegin;
  
  PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
  ierr = TaoSolverGetOptionsPrefix(tao,&prefix); CHKERRQ(ierr);
  ierr = PetscStrlen(prefix,&n1);
  ierr = PetscStrlen("_pc_type",&n2);
  ierr = PetscMalloc(n1+n2+1,&option);
  ierr = PetscStrncpy(option,prefix,n1+1);
  ierr = PetscStrncat(option,"_pc_type",n2+1);
  ierr = PetscOptionsSetValue(option,pctype); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverSetType"
/*@C
   TaoSolverSetType - Sets the method for the unconstrained minimization solver.  

   Collective on TaoSolver

   Input Parameters:
+  solver - the TaoSolver solver context
-  type - a known method

   Options Database Key:
+  -tao_method <type> - Sets the method; use -help for a list
   of available methods (for instance, "-tao_method tao_lmvm" or 
   "-tao_method tao_tron")
-  -tao_type <type> - identical to -tao_method

   Available methods include:
+    tao_nls - Newton's method with line search for unconstrained minimization
.    tao_ntr - Newton's method with trust region for unconstrained minimization
.    tao_ntl - Newton's method with trust region, line search for unconstrained minimization
.    tao_lmvm - Limited memory variable metric method for unconstrained minimization
.    tao_cg - Nonlinear conjugate gradient method for unconstrained minimization
.    tao_nm - Nelder-Mead algorithm for derivate-free unconstrained minimization
.    tao_pounder - Model-based algorithm for derivate-free unconstrained minimization
.    tao_tron - Newton Trust Region method for bound constrained minimization
.    tao_gpcg - Newton Trust Region method for quadratic bound constrained minimization
.    tao_blmvm - Limited memory variable metric method for bound constrained minimization
+    tao_pounders - Model-based algorithm pounder extended for nonlinear least squares 

  Level: intermediate

.seealso: TaoSolverCreate(), TaoSolverGetMethod(), TaoSolverGetType()

@*/
PetscErrorCode TaoSolverSetType(TaoSolver tao, const TaoSolverType type)
{
    PetscErrorCode ierr;
    PetscErrorCode (*create_xxx)(TaoSolver);
    PetscBool  issame;

    PetscFunctionBegin;
    PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
    
    ierr = PetscTypeCompare((PetscObject)tao,type,&issame); CHKERRQ(ierr);
    if (issame) PetscFunctionReturn(0);

    ierr = PetscFListFind(TaoSolverList,((PetscObject)tao)->comm, type, PETSC_TRUE, (void(**)(void))&create_xxx); CHKERRQ(ierr);
    if (!create_xxx) {
	SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_UNKNOWN_TYPE,"Unable to find requested TaoSolver type %s",type);
    }
    

    /* Destroy the existing solver information */
    if (tao->ops->destroy) {
	ierr = (*tao->ops->destroy)(tao); CHKERRQ(ierr);
    }
    if (tao->ksp) {
      ierr = KSPDestroy(&tao->ksp); CHKERRQ(ierr);
      tao->ksp = PETSC_NULL;
    }
    if (tao->linesearch) {
      ierr = TaoLineSearchDestroy(tao->linesearch); CHKERRQ(ierr);
      tao->linesearch = PETSC_NULL;
    }
    if (tao->gradient) {
      ierr = VecDestroy(&tao->gradient); CHKERRQ(ierr);
      tao->gradient = PETSC_NULL;
    }
    if (tao->stepdirection) {
      ierr = VecDestroy(&tao->stepdirection); CHKERRQ(ierr);
      tao->stepdirection = PETSC_NULL;
    }

    tao->ops->setup = 0;
    tao->ops->solve = 0;
    tao->ops->view  = 0;
    tao->ops->setfromoptions = 0;
    tao->ops->destroy = 0;

    tao->setupcalled = PETSC_FALSE;

    ierr = (*create_xxx)(tao); CHKERRQ(ierr);
    ierr = PetscObjectChangeTypeName((PetscObject)tao,type); CHKERRQ(ierr);
    
    PetscFunctionReturn(0);
    
}
#undef __FUNCT__
#define __FUNCT__ "TaoSolverRegister"
/*MC
   TaoSolverRegister- Adds a method to the TAO package for unconstrained minimization.

   Synopsis:
   TaoSolverRegister(char *name_solver,char *path,char *name_Create,int (*routine_Create)(TaoSolver))

   Not collective

   Input Parameters:
+  sname - name of a new user-defined solver
.  path - path (either absolute or relative) the library containing this solver
.  cname - name of routine to Create method context
-  func - routine to Create method context

   Notes:
   TaoSolverRegister() may be called multiple times to add several user-defined solvers.

   If dynamic libraries are used, then the fourth input argument (func)
   is ignored.

   Environmental variables such as ${TAO_DIR}, ${PETSC_ARCH}, ${PETSC_DIR}, 
   and others of the form ${any_environmental_variable} occuring in pathname will be 
   replaced with the appropriate values used when PETSc and TAO were compiled.

   Sample usage:
.vb
   TaoSolverRegister("my_solver","/home/username/mylibraries/${PETSC_ARCH}/mylib.a",
                "MySolverCreate",MySolverCreate);
.ve

   Then, your solver can be chosen with the procedural interface via
$     TaoSolverSetType(tao,"my_solver")
   or at runtime via the option
$     -tao_method my_solver

   Level: advanced

.seealso: TaoSolverRegisterAll(), TaoSolverRegisterDestroy()
M*/
PetscErrorCode TaoSolverRegister(const char sname[], const char path[], const char cname[], PetscErrorCode (*func)(TaoSolver))
{
    char fullname[PETSC_MAX_PATH_LEN];
    PetscErrorCode ierr;

    PetscFunctionBegin;
    ierr = PetscFListConcat(path,cname,fullname); CHKERRQ(ierr);
    ierr = PetscFListAdd(&TaoSolverList,sname, fullname,(void (*)(void))func); CHKERRQ(ierr);
    PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverRegisterDestroy"
/*@C
   TaoSolverRegisterDestroy - Frees the list of minimization solvers that were
   registered by TaoSolverRegisterDynamic().

   Not Collective

   Level: advanced

.seealso: TaoSolverRegisterAll(), TaoSolverRegister()
@*/
PetscErrorCode TaoSolverRegisterDestroy(void)
{
    PetscErrorCode ierr;
    PetscFunctionBegin;
    ierr = PetscFListDestroy(&TaoSolverList); CHKERRQ(ierr);
    TaoSolverRegisterAllCalled = PETSC_FALSE;
    PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolverGetConvergedReason"
/*@ 
   TaoSolverGetConvergedReason - Gets the reason the TaoSolver iteration was stopped.

   Not Collective

   Input Parameter:
.  tao - the TAO_SOLVER solver context

   Output Parameter:
.  reason - one of

$    TAO_CONVERGED_ATOL (2),        ||g||^2 <= atol
$    TAO_CONVERGED_RTOL (3),        ||g||^2
$    TAO_CONVERGED_TRTOL (4),       step size small
$    TAO_CONVERGED_MINF (5),        F < F_min
$    TAO_CONVERGED_USER (6),        (user defined)

$    TAO_DIVERGED_MAXITS (-2),      (its>maxits)
$    TAO_DIVERGED_NAN (-4),         (Numerical problems)
$    TAO_DIVERGED_MAXFCN (-5),      (nfunc > maxnfuncts)
$    TAO_DIVERGED_LS_FAILURE (-6),  (line search failure)
$    TAO_DIVERGED_TR_REDUCTION (-7),
$    TAO_DIVERGED_USER (-8),        (user defined)

$    TAO_CONTINUE_ITERATING  (0)

   where
+  F - current function value
.  xdiff - current trust region size
.  f - function value
.  atol - absolute tolerance
.  rtol - relative tolerance
.  its - current iterate number
.  maxits - maximum number of iterates
.  nfunc - number of function evaluations
-  maxnfuncts - maximum number of function evaluations

   Level: intermediate

.keywords: convergence, View

.seealso: TaoSolverSetConvergenceTest(), TaoSolverSetTolerances()

@*/
PetscErrorCode TaoSolverGetConvergedReason(TaoSolver tao, TaoSolverTerminationReason *reason) 
{
    PetscFunctionBegin;
    PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
    PetscValidPointer(reason,2);
    *reason = tao->reason;
    PetscFunctionReturn(0);
}

#undef __FUNCT__ 
#define __FUNCT__ "TaoSolverGetType"
/*@C
   TaoSolverGetType - Gets the current TaoSolver algorithm.

   Not Collective

   Input Parameter:
.  tao - the TaoSolver solver context

   Output Parameter:
.  type - TaoSolver method 

   Level: intermediate

.keywords: method
@*/
PetscErrorCode TaoSolverGetType(TaoSolver tao, TaoSolverType *type)
{
    PetscFunctionBegin;
    PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
    PetscValidPointer(type,2); 
    *type=((PetscObject)tao)->type_name;
    PetscFunctionReturn(0);
}

#undef __FUNCT__ 
#define __FUNCT__ "TaoSolverMonitor"
/*@C
  TaoSolverMonitor - Monitor the solver and the current solution.  This
  routine will record the iteration number and residual statistics,
  call any monitors specified by the user, and calls the convergence-check routine.

   Input Parameters:
+  tao - the TAO_SOLVER context
.  its - the current iterate number (>=0)
.  f - the current objective function value
.  res - the gradient norm, square root of the duality gap, or other measure
indicating distince from optimality.  This measure will be recorded and
used for some termination tests.
.  cnorm - the infeasibility of the current solution with regard to the constraints.
-  steplength - multiple of the step direction added to the previous iterate.

   Output Parameters:
.  reason - The termination reason, which can equal TAO_CONTINUE_ITERATING

   Options Database Key:
.  -tao_monitor - Use the default monitor, which prints statistics to standard output

.seealso TaoSolverGetConvergedReason(), TaoSolverDefaultMonitor(), TaoSolverSetMonitor()

   Level: developer

@*/
PetscErrorCode TaoSolverMonitor(TaoSolver tao, PetscInt its, PetscReal f, PetscReal res, PetscReal cnorm, PetscReal steplength, TaoSolverTerminationReason *reason) 
{
    PetscErrorCode ierr;
    int i;
    PetscFunctionBegin;
    PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
    tao->fc = f;
    tao->residual = res;
    tao->cnorm = cnorm;
    tao->step = steplength;
    tao->niter=its;
    TaoSolverLogHistory(tao,f,res,cnorm);
    if (TaoInfOrNaN(f) || TaoInfOrNaN(res)) {
      SETERRQ(PETSC_COMM_SELF,1, "User provided compute function generated Inf or NaN");
    }
    if (tao->ops->convergencetest) {
      ierr = (*tao->ops->convergencetest)(tao,tao->cnvP); CHKERRQ(ierr);
    }
    for (i=0;i<tao->numbermonitors;i++) {
      ierr = (*tao->monitor[i])(tao,tao->monitorcontext[i]); CHKERRQ(ierr);
    }
    *reason = tao->reason;
    
    PetscFunctionReturn(0);

}

#undef __FUNCT__  
#define __FUNCT__ "TaoSolverSetHistory"
/*@
   TaoSolverSetHistory - Sets the array used to hold the convergence history.

   Collective on TAO_SOLVER

   Input Parameters:
+  tao - the TaoSolver solver context
.  obj   - array to hold objective value history
.  resid - array to hold residual history
.  cnorm - array to hold constraint violation history
.  na  - size of obj, resid, and cnorm
-  reset - PetscTrue indicates each new minimization resets the history counter to zero,
           else it continues storing new values for new minimizations after the old ones

   Notes:
   If set, TAO will fill the given arrays with the indicated
   information at each iteration.  If no information is desired
   for a given array, then PETSC_NULL may be used.

   This routine is useful, e.g., when running a code for purposes
   of accurate performance monitoring, when no I/O should be done
   during the section of code that is being timed.

   Level: intermediate

.keywords: options, view, monitor, convergence, history

.seealso: TaoSolverGetHistory()

@*/
PetscErrorCode TaoSolverSetHistory(TaoSolver tao, PetscReal *obj, PetscReal *resid, PetscReal *cnorm, PetscInt na,PetscBool reset)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
  tao->hist_obj = obj;
  tao->hist_resid = resid;
  tao->hist_cnorm = cnorm;
  tao->hist_max   = na;
  tao->hist_reset = reset;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "TaoSolverGetHistory"
/*@
   TaoSolverGetHistory - Gets the array used to hold the convergence history.

   Collective on TaoSolver

   Input Parameter:
+  tao - the TaoSolver solver context

   Output Parameters:
+  obj   - array used to hold objective value history
.  resid - array used to hold residual history
.  cnorm - array used to hold constraint violation history
-  na  - size of obj, resid, and cnorm (will be less than or equal to na given in TaoSolverSetHistory)


   Notes:
    The calling sequence for this routine in Fortran is
$   call TaoSolverGetHistory(TaoSolver tao, integer obj, integer resid, integer cnorm, integer na, integer info)

   This routine is useful, e.g., when running a code for purposes
   of accurate performance monitoring, when no I/O should be done
   during the section of code that is being timed.

   Level: advanced

.keywords: convergence, history, monitor, View

.seealso: TaoSolverSetConvergencHistory()

@*/
PetscErrorCode TaoSolverGetHistory(TaoSolver tao, PetscReal **obj, PetscReal **resid, PetscReal **cnorm, PetscInt *na)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAOSOLVER_CLASSID,1);
  if (obj)   *obj   = tao->hist_obj;
  if (cnorm) *cnorm = tao->hist_cnorm;
  if (resid) *resid = tao->hist_resid;
  if (na) *na   = tao->hist_len;
  PetscFunctionReturn(0);
}

