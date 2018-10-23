
#include <../src/mat/impls/aij/seq/aij.h>
#include "../../../../vec/is/is/impls/general/general.h"
#include "../../../../vec/vec/impls/mpi/pvecimpl.h"
#include <petscviewerhdf5.h>

#if defined(PETSC_HAVE_HDF5)
PetscErrorCode MatLoad_AIJ_HDF5(Mat mat, PetscViewer viewer)
{
  PetscMPIInt     rank,size;
  hid_t           file_id,group_matrix_id,dset_id,dspace_id;
  hsize_t         h5_dims[4];

  PetscInt        count_data;
  PetscInt        *i = NULL,*j = NULL;
  PetscScalar     *a = NULL;
  const char      *a_name,*i_name,*j_name,*mat_name,*c_name;
  int             rdim;
  PetscInt        p,m,M,N;
  PetscInt        bs = mat->rmap->bs;
  PetscBool       flg;

  PetscErrorCode  ierr;
  MPI_Comm        comm;

  IS              is_i,is_j;
  Vec             vec_a;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)mat,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = PetscObjectGetName((PetscObject)mat,&mat_name);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetAIJNames(viewer,&i_name,&j_name,&a_name,&c_name);CHKERRQ(ierr);

  ierr = PetscOptionsBegin(comm,NULL,"Options for loading matrix from HDF5","Mat");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-matload_block_size","Set the blocksize used to store the matrix","MatLoad",bs,&bs,&flg);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  if (flg) {
    ierr = MatSetBlockSize(mat, bs);CHKERRQ(ierr);
  }

  ierr = PetscViewerHDF5PushGroup(viewer,mat_name);CHKERRQ(ierr);
  ierr = PetscViewerHDF5OpenGroup(viewer,&file_id,&group_matrix_id);CHKERRQ(ierr);

  ierr = PetscViewerHDF5ReadAttribute(viewer,mat_name,c_name,PETSC_INT,&N);CHKERRQ(ierr);

  PetscStackCallHDF5Return(dset_id,H5Dopen,(group_matrix_id,i_name,H5P_DEFAULT));
  PetscStackCallHDF5Return(dspace_id,H5Dget_space,(dset_id));
  PetscStackCallHDF5Return(rdim,H5Sget_simple_extent_dims,(dspace_id,h5_dims,NULL));
  M = (PetscInt) h5_dims[rdim-1] - 1;
  PetscStackCallHDF5(H5Sclose,(dspace_id));
  PetscStackCallHDF5(H5Dclose,(dset_id));

  /* If global sizes are set, check if they are consistent with that given in the file */
  if (mat->rmap->N >= 0 && mat->rmap->N != M) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"Inconsistent # of rows: Matrix in file has (%D) and input matrix has (%D)",mat->rmap->N,M);
  if (mat->cmap->N >= 0 && mat->cmap->N != N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"Inconsistent # of cols: Matrix in file has (%D) and input matrix has (%D)",mat->cmap->N,N);
  mat->rmap->N = M;
  mat->cmap->N = N;

  /* determine ownership of all (block) rows and columns */
  ierr = PetscLayoutSetUp(mat->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(mat->cmap);CHKERRQ(ierr);
  m = (rank == size - 1) ? mat->rmap->n + 1 : mat->rmap->n;
  M = M + 1;

  /* Read array i (array of row indices) */
  ierr = ISCreate(comm,&is_i);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject)is_i,i_name);CHKERRQ(ierr);
  ierr = PetscLayoutSetLocalSize(is_i->map,m);CHKERRQ(ierr);
  ierr = PetscLayoutSetSize(is_i->map,M);CHKERRQ(ierr);
  ierr = ISLoad(is_i,viewer);CHKERRQ(ierr);
  i    = ((IS_General*)is_i->data)->idx;

  /* Determine offset and count of elements for reading local part of array data*/
  if (rank > 0) {
    ierr = MPI_Isend(i,1,MPIU_INT,rank-1,0,comm,NULL);CHKERRQ(ierr);
  }

  if (rank < size - 1) {
    ierr = MPI_Irecv(&count_data,1,MPIU_INT,rank+1,0,comm,NULL);CHKERRQ(ierr);
    count_data = count_data - i[0];
  } else {
    count_data = i[m-1] - i[0];
  }

  /* Read array j (array of column indices) */
  ierr = ISCreate(comm,&is_j);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject)is_j,j_name);CHKERRQ(ierr);
  ierr = PetscLayoutSetLocalSize(is_j->map,count_data);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(is_j->map);CHKERRQ(ierr);
  ierr = ISLoad(is_j,viewer);CHKERRQ(ierr);
  j    = ((IS_General*)is_j->data)->idx;

  /* Read array a (array of values) */
  ierr = VecCreate(comm,&vec_a);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject)vec_a,a_name);CHKERRQ(ierr);
  ierr = VecSetSizes(vec_a,count_data,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = VecLoad(vec_a,viewer);CHKERRQ(ierr);
  a    = ((Vec_MPI*)vec_a->data)->array;

  /* close group */
  PetscStackCallHDF5(H5Gclose,(group_matrix_id));
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);

  /* Converting global to local indexing of rows */
  for (p=1; p<m; ++p) i[p] -= i[0];
  i[0] = 0;

  /* populate matrix */
  if (!((PetscObject)mat)->type_name) {
    ierr = MatSetType(mat,MATAIJ);CHKERRQ(ierr);
  }
  ierr = MatSeqAIJSetPreallocationCSR(mat,i,j,a);CHKERRQ(ierr);
  ierr = MatMPIAIJSetPreallocationCSR(mat,i,j,a);CHKERRQ(ierr);
  /*
  ierr = MatSeqBAIJSetPreallocationCSR(mat,bs,i,j,a);CHKERRQ(ierr);
  ierr = MatMPIBAIJSetPreallocationCSR(mat,bs,i,j,a);CHKERRQ(ierr);
  */

  ierr = ISDestroy(&is_i);CHKERRQ(ierr);
  ierr = ISDestroy(&is_j);CHKERRQ(ierr);
  ierr = VecDestroy(&vec_a);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#endif

