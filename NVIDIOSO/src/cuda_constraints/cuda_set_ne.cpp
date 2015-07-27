//
//  cuda_set_ne.cpp
//  iNVIDIOSO
//
//  Created by Federico Campeotto on 07/07/14.
//  Copyright (c) 2014-2015 ___UDNMSU___. All rights reserved.
//

#include <stdio.h>
#include "cuda_set_ne.h"

#if CUDAON

__device__ CudaSetNe::CudaSetNe ( int n_id, int n_vars, int n_args,
                                  int* vars, int* args,
                                  int* domain_idx, uint* domain_states ) :
CudaConstraint ( n_id, n_vars, n_args, vars, args, domain_idx, domain_states ) {
}//CudaSetNe

__device__ CudaSetNe::~CudaSetNe ()
{
}

__device__ void
CudaSetNe::consistency ( int ref )
{
}//consistency

//! It checks if
__device__ bool
CudaSetNe::satisfied ()
{
  return true;
}//satisfied

//! Prints the semantic of this constraint
__device__ void
CudaSetNe::print() const
{
}//print_semantic
 
#endif


