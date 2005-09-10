// 
// File:          TOPS_StructuredSolver_Impl.cc
// Symbol:        TOPS.StructuredSolver-v0.0.0
// Symbol Type:   class
// Babel Version: 0.10.8
// Description:   Server-side implementation for TOPS.StructuredSolver
// 
// WARNING: Automatically generated; only changes within splicers preserved
// 
// babel-version = 0.10.8
// 
#include "TOPS_StructuredSolver_Impl.hh"

// DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver._includes)
#include <iostream>
#include "TOPS_Structured_Matrix_Impl.hh"
// Uses ports includes
  // This code is the same as DAVecGetArray() except instead of generating
  // raw C multidimensional arrays it gets a Babel array
::sidl::array<double> DAVecGetArrayBabel(DA da,Vec vec)
{
  double *uu;
  VecGetArray(vec,&uu);
  PetscInt  xs,ys,zs,xm,ym,zm,gxs,gys,gzs,gxm,gym,gzm,dim,dof,N;
  DAGetCorners(da,&xs,&ys,&zs,&xm,&ym,&zm);
  DAGetGhostCorners(da,&gxs,&gys,&gzs,&gxm,&gym,&gzm);
  DAGetInfo(da,&dim,0,0,0,0,0,0,&dof,0,0,0);

  /* Handle case where user passes in global vector as opposed to local */
  VecGetLocalSize(vec,&N);
  if (N == xm*ym*zm*dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  }

  sidl::array<double> ua;
  int lower[4],upper[4],stride[4];
  if (dof > 1) {
    dim++;
    lower[0] = 0; upper[0] = dof; stride[0] = 1;
    lower[1] = gxs; lower[2] = gys; lower[3] = gzs;
    upper[1] = gxm + gxs - 1; upper[2] = gym  + gys - 1; upper[3] = gzm + gzs - 1;
    stride[1] = dof; stride[2] = gxm*dof; stride[3] = gxm*gym*dof;
  } else {
    lower[0] = gxs; lower[1] = gys; lower[2] = gzs;
    upper[0] = gxm +gxs - 1; upper[1] = gym + gys - 1 ; upper[2] = gzm + gzs - 1;
    stride[0] = 1; stride[1] = gxm; stride[2] = gxm*gym;
  }
  ua.borrow(uu,dim,*&lower,*&upper,*&stride);
  return ua;
}

#undef __FUNCT__
#define __FUNCT__ "FormFunction"
static PetscErrorCode FormFunction(SNES snes,Vec uu,Vec f,void *vdmmg)
{
  PetscFunctionBegin;
  DMMG dmmg = (DMMG) vdmmg;
  TOPS::StructuredSolver *solver = (TOPS::StructuredSolver*) dmmg->user;
  TOPS::System::Compute::Residual system;
#ifdef USE_PORTS
  system = solver->getServices().getPort("TOPS.System.Compute.Residual");
  if (system._is_nil()) {
    std::cerr << "Error at " << __FILE__ << ":" << __LINE__ 
	      << ": TOPS.System.Compute.Residual port is nil, " 
	      << "possibly not connected." << std::endl;
    PetscFunctionReturn(1);
  }
#else
  system = (TOPS::System::Compute::Residual) solver->getSystem();
#endif

  DA da = (DA) dmmg->dm;
  Vec u; 
  DAGetLocalVector(da,&u);
  DAGlobalToLocalBegin(da,uu,INSERT_VALUES,u);
  DAGlobalToLocalEnd(da,uu,INSERT_VALUES,u);

  int mx,my,mz;
  DAGetInfo(da,0,&mx,&my,&mz,0,0,0,0,0,0,0);
  solver->setLength(0,mx);
  solver->setLength(1,my);
  solver->setLength(2,mz);
  sidl::array<double> ua = DAVecGetArrayBabel(da,u);
  sidl::array<double> fa = DAVecGetArrayBabel(da,f);;
  system.computeResidual(ua,fa);
  VecRestoreArray(u,0);
  DARestoreLocalVector(da,&u);
  VecRestoreArray(f,0);

#ifdef USE_PORTS
  solver->getServices().releasePort("TOPS.System.Compute.Residual");
#endif
  PetscFunctionReturn(0);
}

static PetscErrorCode FormInitialGuess(DMMG dmmg,Vec f)
{
  PetscFunctionBegin;
  TOPS::StructuredSolver *solver = (TOPS::StructuredSolver*) dmmg->user;
  TOPS::System::Compute::InitialGuess system;
#ifdef USE_PORTS
  system = solver->getServices().getPort("TOPS.System.Compute.InitialGuess");
  if (system._is_nil()) {
    std::cerr << "Error at " << __FILE__ << ":" << __LINE__ 
	      << ": TOPS.System.Compute.InitialGuess port is nil, " 
	      << "possibly not connected." << std::endl;
    PetscFunctionReturn(1);
  }
#else
  system = (TOPS::System::Compute::InitialGuess) solver->getSystem();
#endif

  int mx,my,mz;
  DAGetInfo((DA)dmmg->dm,0,&mx,&my,&mz,0,0,0,0,0,0,0);
  solver->setLength(0,mx);
  solver->setLength(1,my);
  solver->setLength(2,mz);
  sidl::array<double> fa = DAVecGetArrayBabel((DA)dmmg->dm,f);

  system.computeInitialGuess(fa);
  VecRestoreArray(f,0);
#ifdef USE_PORTS
  solver->getServices().releasePort("TOPS.System.Compute.InitialGuess");
#endif
  PetscFunctionReturn(0);
}

static PetscErrorCode FormMatrix(DMMG dmmg,Mat J,Mat B)
{
  PetscFunctionBegin;
  TOPS::StructuredSolver *solver = (TOPS::StructuredSolver*) dmmg->user;
  TOPS::System::Compute::Matrix system;

  TOPS::Structured::Matrix matrix1 = TOPS::Structured::Matrix::_create();
  TOPS::Structured::Matrix matrix2 = TOPS::Structured::Matrix::_create();

  PetscInt  xs,ys,zs,xm,ym,zm,gxs,gys,gzs,gxm,gym,gzm,dof,mx,my,mz;
  DAGetCorners((DA)dmmg->dm,&xs,&ys,&zs,&xm,&ym,&zm);
  DAGetGhostCorners((DA)dmmg->dm,&gxs,&gys,&gzs,&gxm,&gym,&gzm);
  DAGetInfo((DA)dmmg->dm,0,&mx,&my,&mz,0,0,0,&dof,0,0,0);
#define GetImpl(A,b) (!(A)b) ? 0 : reinterpret_cast<A ## _impl*>(((A) b)._get_ior()->d_data)

  // currently no support for dof > 1
  TOPS::Structured::Matrix_impl *imatrix1 = GetImpl(TOPS::Structured::Matrix,matrix1);
  imatrix1->vlength[0] = xm; imatrix1->vlength[1] = ym; imatrix1->vlength[2] = zm; 
  imatrix1->vlower[0] = xs; imatrix1->vlower[1] = ys; imatrix1->vlower[2] = zs; 
  imatrix1->gghostlength[0] = gxm; imatrix1->gghostlength[1] = gym; 
  imatrix1->gghostlength[2] = gzm; 
  imatrix1->gghostlower[0] = gxs; imatrix1->gghostlower[1] = gys; 
  imatrix1->gghostlower[2] = gzs; 
  imatrix1->vdimen = dof;

  TOPS::Structured::Matrix_impl *imatrix2 = GetImpl(TOPS::Structured::Matrix,matrix2);
  imatrix2->vlength[0] = xm; imatrix2->vlength[1] = ym; imatrix2->vlength[2] = zm; 
  imatrix2->vlower[0] = xs; imatrix2->vlower[1] = ys; imatrix2->vlower[2] = zs; 
  imatrix2->gghostlength[0] = gxm; imatrix2->gghostlength[1] = gym; 
  imatrix2->gghostlength[2] = gzm; 
  imatrix2->gghostlower[0] = gxs; imatrix2->gghostlower[1] = gys; 
  imatrix2->gghostlower[2] = gzs; 
  imatrix2->vdimen = dof;

  imatrix1->mat = J;
  imatrix2->mat = B;
  DAGetInfo((DA)dmmg->dm,0,&mx,&my,&mz,0,0,0,0,0,0,0);
  solver->setLength(0,mx);
  solver->setLength(1,my);
  solver->setLength(2,mz);

#ifdef USE_PORTS
  system = solver->getServices().getPort("TOPS.System.Compute.Matrix");
  if (system._is_nil()) {
    std::cerr << "Error at " << __FILE__ << ":" << __LINE__ 
	      << ": TOPS.System.Compute.Matrix port is nil, " 
	      << "possibly not connected." << std::endl;
    PetscFunctionReturn(1);
  }
#else
  system = (TOPS::System::Compute::Matrix) solver->getSystem();
#endif

  // Use the port
  if (system._not_nil()) {
    system.computeMatrix(matrix1,matrix2);
  }

#ifdef USE_PORTS
  solver->getServices().releasePort("TOPS.System.Compute.Matrix");
#endif

  MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);
  if (J != B) {
    MatAssemblyBegin(J,MAT_FINAL_ASSEMBLY);
    MatAssemblyEnd(J,MAT_FINAL_ASSEMBLY);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "FormRightHandSide"
static PetscErrorCode FormRightHandSide(DMMG dmmg,Vec f)
{
  PetscFunctionBegin;
  int mx,my,mz;
  TOPS::StructuredSolver *solver = (TOPS::StructuredSolver*) dmmg->user;
  TOPS::System::Compute::RightHandSide system;

#ifdef USE_PORTS
  system = solver->getServices().getPort("TOPS.System.Compute.RightHandSide");
#else
  system = (TOPS::System::Compute::RightHandSide) solver->getSystem();
#endif

  DAGetInfo((DA)dmmg->dm,0,&mx,&my,&mz,0,0,0,0,0,0,0);
  solver->setLength(0,mx);
  solver->setLength(1,my);
  solver->setLength(2,mz);
  sidl::array<double> fa = DAVecGetArrayBabel((DA)dmmg->dm,f);;

  if (system._not_nil()) {
    system.computeRightHandSide(fa);
  }

#ifdef USE_PORTS
  solver->getServices().releasePort("TOPS.System.Compute.RightHandSide");
#endif

  VecRestoreArray(f,0);
  PetscFunctionReturn(0);
}
// DO-NOT-DELETE splicer.end(TOPS.StructuredSolver._includes)

// user-defined constructor.
void TOPS::StructuredSolver_impl::_ctor() {
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver._ctor)
#undef __FUNCT__
#define __FUNCT__ "TOPS::StructuredSolver_impl::_ctor()"

  this->dmmg = PETSC_NULL;
  this->da   = PETSC_NULL;
  this->m    = PETSC_DECIDE;
  this->n    = PETSC_DECIDE;
  this->p    = PETSC_DECIDE;
  this->lengths[0] = 3;
  this->lengths[1] = 3;
  this->lengths[2] = 3;
  this->dim  = 2;
  this->s    = 1;
  this->wrap = DA_NONPERIODIC;
  this->bs   = 1;
  this->stencil_type = DA_STENCIL_STAR;
  this->levels       = 3;
  this->system       = PETSC_NULL;
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver._ctor)
}

// user-defined destructor.
void TOPS::StructuredSolver_impl::_dtor() {
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver._dtor)
#undef __FUNCT__
#define __FUNCT__ "TOPS::StructuredSolver_impl::_dtor()"

  if (this->dmmg) {DMMGDestroy(this->dmmg);}
  if (this->startedpetsc) {
    PetscFinalize();
  }
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver._dtor)
}

// static class initializer.
void TOPS::StructuredSolver_impl::_load() {
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver._load)
  // Insert-Code-Here {TOPS.StructuredSolver._load} (class initialization)
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver._load)
}

// user-defined static methods: (none)

// user-defined non-static methods:
/**
 * Method:  getServices[]
 */
::gov::cca::Services
TOPS::StructuredSolver_impl::getServices ()
throw () 

{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.getServices)
  // Insert-Code-Here {TOPS.StructuredSolver.getServices} (getServices method)
#undef __FUNCT__
#define __FUNCT__ "TOPS::StructuredSolver_impl::getServices()"

  return this->myServices;
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.getServices)
}

/**
 * Method:  setSystem[]
 */
void
TOPS::StructuredSolver_impl::setSystem (
  /* in */ ::TOPS::System::System system ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.setSystem)
#undef __FUNCT__
#define __FUNCT__ "TOPS::StructuredSolver_impl::setSystem()"
  this->system = system;
  system.setSolver(this->self);
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.setSystem)
}

/**
 * Method:  getSystem[]
 */
::TOPS::System::System
TOPS::StructuredSolver_impl::getSystem ()
throw () 

{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.getSystem)
#undef __FUNCT__
#define __FUNCT__ "TOPS::StructuredSolver_impl::getSystem()"
  return this->system;
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.getSystem)
}

/**
 * Method:  Initialize[]
 */
void
TOPS::StructuredSolver_impl::Initialize (
  /* in */ ::sidl::array< ::std::string> args ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.Initialize)
#undef __FUNCT__
#define __FUNCT__ "TOPS::StructuredSolver_impl::Initialize"
  PetscTruth initialized;
  PetscInitialized(&initialized);
  if (initialized) {
    this->startedpetsc = 0;
    return;
  }
  this->startedpetsc = 1;
  int          argc = args.upper(0) + 1;
  char       **argv = new char* [argc];
  std::string  arg;

  for(int i = 0; i < argc; i++) {
    arg     = args[i];
    argv[i] = new char [arg.length()+1];
    arg.copy(argv[i], arg.length(), 0);
    argv[i][arg.length()] = 0;
  }
  int    ierr = PetscInitialize(&argc,&argv,0,0); 
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.Initialize)
}

/**
 * Method:  solve[]
 */
void
TOPS::StructuredSolver_impl::solve ()
throw () 

{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.solve)
#undef __FUNCT__
#define __FUNCT__ "TOPS::StructuredSolver_impl::solve()"
  PetscErrorCode ierr;
  TOPS::System::System sys;

  if (!this->dmmg) {
    TOPS::System::Initialize::Once once;
#ifdef USE_PORTS
    once = myServices.getPort("TOPS.System.Initialize.Once");
#else
    once = (TOPS::System::Initialize::Once) this->system;
#endif
    if (once._not_nil()) {    
      once.initializeOnce();
    }
#ifdef USE_PORTS
    myServices.releasePort("TOPS.System.Initialize.Once");
#endif

    // create DMMG object 
    DMMGCreate(PETSC_COMM_WORLD,this->levels,(void*)&this->self,&this->dmmg);
    DACreate(PETSC_COMM_WORLD,this->dim,this->wrap,this->stencil_type,this->lengths[0],
	     this->lengths[1],this->lengths[2],this->m,this->n,
             this->p,this->bs,this->s,PETSC_NULL,PETSC_NULL,PETSC_NULL,&this->da);
    DMMGSetDM(this->dmmg,(DM)this->da);

    TOPS::System::Compute::Residual residual;
#ifdef USE_PORTS
    residual = myServices.getPort("TOPS.System.Compute.Residual");
#else
    residual = (TOPS::System::Compute::Residual) this->system;
#endif
    if (residual._not_nil()) {
      ierr = DMMGSetSNES(this->dmmg, FormFunction, 0);
    } else {
      ierr = DMMGSetKSP(this->dmmg,FormRightHandSide,FormMatrix);
    }
#ifdef USE_PORTS
    myServices.releasePort("TOPS.System.Compute.Residual");
#endif

    TOPS::System::Compute::InitialGuess guess;
#ifdef USE_PORTS
    guess = myServices.getPort("TOPS.System.Compute.InitialGuess");
#else
    guess = (TOPS::System::Compute::InitialGuess) this->system;
#endif
    if (guess._not_nil()) {
      ierr = DMMGSetInitialGuess(this->dmmg, FormInitialGuess);
    }
  }
#ifdef USE_PORTS
  myServices.releasePort("TOPS.System.Compute.InitialGuess");
#endif
  
  TOPS::System::Initialize::EverySolve every;
#ifdef USE_PORTS
  every = myServices.getPort("TOPS.System.Initialize.EverySolve");
#else
  every = (TOPS::System::Initialize::EverySolve)this->system;
#endif
  if (every._not_nil()) {    
    every.initializeEverySolve();
  }
#ifdef USE_PORTS
    myServices.releasePort("TOPS.System.Initialize.EverySolve");
#endif

  DMMGSolve(this->dmmg);
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.solve)
}

/**
 * Method:  setBlockSize[]
 */
void
TOPS::StructuredSolver_impl::setBlockSize (
  /* in */ int32_t bs ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.setBlockSize)
  this->bs = bs;
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.setBlockSize)
}

/**
 * Method:  getSolution[]
 */
::sidl::array<double>
TOPS::StructuredSolver_impl::getSolution ()
throw () 

{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.getSolution)
  // Insert-Code-Here {TOPS.StructuredSolver.getSolution} (getSolution method)
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.getSolution)
}

/**
 * Method:  setSolution[]
 */
void
TOPS::StructuredSolver_impl::setSolution (
  /* in */ ::sidl::array<double> location ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.setSolution)
  // Insert-Code-Here {TOPS.StructuredSolver.setSolution} (setSolution method)
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.setSolution)
}

/**
 * Method:  dimen[]
 */
int32_t
TOPS::StructuredSolver_impl::dimen ()
throw () 

{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.dimen)
  return dim;
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.dimen)
}

/**
 * Method:  length[]
 */
int32_t
TOPS::StructuredSolver_impl::length (
  /* in */ int32_t a ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.length)
  return this->lengths[a];
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.length)
}

/**
 * Method:  setDimen[]
 */
void
TOPS::StructuredSolver_impl::setDimen (
  /* in */ int32_t dim ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.setDimen)
  this->dim = dim;
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.setDimen)
}

/**
 * Method:  setLength[]
 */
void
TOPS::StructuredSolver_impl::setLength (
  /* in */ int32_t a,
  /* in */ int32_t l ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.setLength)
  this->lengths[a] = l;
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.setLength)
}

/**
 * Method:  setStencilWidth[]
 */
void
TOPS::StructuredSolver_impl::setStencilWidth (
  /* in */ int32_t width ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.setStencilWidth)
  // Insert-Code-Here {TOPS.StructuredSolver.setStencilWidth} (setStencilWidth method)
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.setStencilWidth)
}

/**
 * Method:  getStencilWidth[]
 */
int32_t
TOPS::StructuredSolver_impl::getStencilWidth ()
throw () 

{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.getStencilWidth)
  // Insert-Code-Here {TOPS.StructuredSolver.getStencilWidth} (getStencilWidth method)
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.getStencilWidth)
}

/**
 * Method:  setLevels[]
 */
void
TOPS::StructuredSolver_impl::setLevels (
  /* in */ int32_t levels ) 
throw () 
{
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.setLevels)
  this->levels = levels;
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.setLevels)
}

/**
 * Starts up a component presence in the calling framework.
 * @param services the component instance's handle on the framework world.
 * Contracts concerning Svc and setServices:
 * 
 * The component interaction with the CCA framework
 * and Ports begins on the call to setServices by the framework.
 * 
 * This function is called exactly once for each instance created
 * by the framework.
 * 
 * The argument Svc will never be nil/null.
 * 
 * Those uses ports which are automatically connected by the framework
 * (so-called service-ports) may be obtained via getPort during
 * setServices.
 */
void
TOPS::StructuredSolver_impl::setServices (
  /* in */ ::gov::cca::Services services ) 
throw ( 
  ::gov::cca::CCAException
){
  // DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver.setServices)

  myServices = services;
  gov::cca::TypeMap tm = services.createTypeMap();
  if(tm._is_nil()) {
    fprintf(stderr, "Error:: %s:%d: gov::cca::TypeMap is nil\n",
	    __FILE__, __LINE__);
    exit(1);
  }
  gov::cca::Port p = self;      //  Babel required casting
  if(p._is_nil()) {
    fprintf(stderr, "Error:: %s:%d: Error casting self to gov::cca::Port \n",
	    __FILE__, __LINE__);
    exit(1);
  }
  
  // Provides port
  services.addProvidesPort(p,
			   "TOPS.Structured.Solver",
			   "TOPS.Structured.Solver", tm);
  
  // Uses ports
  services.registerUsesPort("TOPS.System.Initialize.Once",
			    "TOPS.System.Initialize.Once", tm);

  services.registerUsesPort("TOPS.System.Initialize.EverySolve",
			    "TOPS.System.Initialize.EverySolve", tm);

  services.registerUsesPort("TOPS.System.Compute.InitialGuess",
			    "TOPS.System.Compute.InitialGuess", tm);

  services.registerUsesPort("TOPS.System.Compute.Matrix",
			    "TOPS.System.Compute.Matrix", tm);

  services.registerUsesPort("TOPS.System.Compute.RightHandSide",
			    "TOPS.System.Compute.RightHandSide", tm);

  services.registerUsesPort("TOPS.System.Compute.Residual",
			    "TOPS.System.Compute.Residual", tm);

  return;
  // DO-NOT-DELETE splicer.end(TOPS.StructuredSolver.setServices)
}


// DO-NOT-DELETE splicer.begin(TOPS.StructuredSolver._misc)
// Insert-Code-Here {TOPS.StructuredSolver._misc} (miscellaneous code)
// DO-NOT-DELETE splicer.end(TOPS.StructuredSolver._misc)

