//
//  cuda_cp_model_simple.cpp
//  iNVIDIOSO
//
//  Created by Federico Campeotto on 07/09/15.
//  Copyright (c) 2014-2015 ___UDNMSU___. All rights reserved.
//

#include "cuda_cp_model_simple.h"
#include "cuda_constraint_macro.h"

#if CUDAON
#include <cuda.h>
#include <cuda_runtime_api.h>
#endif 

using namespace std;

CudaCPModelSimple::CudaCPModelSimple () :
	CudaCPModel () {
	_dbg = "CudaCPModelSimple - ";
}//CudaCPModelSimple

CudaCPModelSimple::~CudaCPModelSimple () 
{
}//~CudaCPModelSimple 

bool
CudaCPModelSimple::alloc_variables () 
{

#if CUDAON
    
    // Calculate total size to allocate on device
    int var_id = 0;
    for ( auto var : _variables ) 
    {
        // Map Boolean variables
        if ( var->get_size() == 2 )
        {
            _bool_var_lookup.insert ( var->get_id() );

            // 2 Integers to represent Boolean domains on Device
            _domain_state_size += 2 * sizeof ( int );
        }
        else
        {
            _domain_state_size += ( (var->domain_iterator)->get_domain_status() ).first;
        }
    	_cuda_var_lookup[ var->get_id() ] = var_id++;
    }
    
    // Allocate space on host and device
    _h_domain_states = (uint*) malloc ( _domain_state_size );
    if ( logger.cuda_handle_error ( cudaMalloc( (void**)&_d_domain_states, _domain_state_size ) ) ) 
    {
    	return false;
    }
    
    // Set states on device
    int idx = 0;
    vector<int> map_vars_to_doms( _variables.size() );
    for ( auto var : _variables ) 
    {// Save the index where each variable domain starts at
    	map_vars_to_doms[ _cuda_var_lookup[ var->get_id() ] ] = idx;
    	idx += ( (var->domain_iterator)->get_domain_status() ).first / sizeof(int);
    }
  
    // Alloc lookup index for variables on device
    if ( logger.cuda_handle_error ( cudaMalloc( (void**)&_d_domain_index, _variables.size() * sizeof ( int ) ) ) ) 
    {
    	return false;
    }
    
    // Copy lookup index for variables on device
    if ( logger.cuda_handle_error ( cudaMemcpy (_d_domain_index, &map_vars_to_doms[ 0 ],
    	                                        _variables.size() * sizeof ( int ), cudaMemcpyHostToDevice ) ) ) 
    {
    	return false;
    }
  	
#endif
  
    return true;
}//alloc_variables

bool
CudaCPModelSimple::upload_device_state () 
{

#if CUDAON

    int idx = 0, value;
    bool singleton;
    const uint * var_domain;
    for ( auto var : _variables )
    {
        var_domain = static_cast<const uint *> ( ( (var->domain_iterator)->get_domain_status() ).second );
        singleton  = ( var_domain [ LB ] ==  var_domain [ UB ] );
        value      = var_domain [ LB ];

        // Differentiate between Boolean domains and standard domains
        if ( _bool_var_lookup.find ( var->get_id() ) != _bool_var_lookup.end () )
        {
            if ( singleton )
            {
                _h_domain_states [ idx ]     = SNG_EVT;
                _h_domain_states [ idx + 1 ] = value;
            }
            else
            { // Set Bool representation as event
                _h_domain_states [ idx ]   = BOL_EVT;
                _h_domain_states [ idx+1 ] = BOL_U;
            }

            idx += 2;
        }
        else
        {
            memcpy ( &_h_domain_states [ idx ], (uint*)( (var->domain_iterator)->get_domain_status() ).second,
                   ( (var->domain_iterator)->get_domain_status() ).first );
            if ( singleton )
            {
                _h_domain_states [ idx ] = SNG_EVT;
            }
            idx += ( (var->domain_iterator)->get_domain_status() ).first / sizeof(int);
        }
    }
    
    if ( logger.cuda_handle_error ( cudaMemcpy (_d_domain_states, &_h_domain_states[ 0 ],
                                                _domain_state_size, cudaMemcpyHostToDevice ) ) ) 
    {
        string err = _dbg + "Error updating device from host.\n";
        throw NvdException ( err.c_str(), __FILE__, __LINE__ );
    }
  
#endif

    return true;
}//upload_device_state

bool
CudaCPModelSimple::download_device_state () 
{

#if CUDAON

    // Download states from device
    if ( logger.cuda_handle_error ( cudaMemcpy (&_h_domain_states[ 0 ], _d_domain_states, 
                                            	_domain_state_size, cudaMemcpyDeviceToHost ) ) ) 
    {
    	string err = _dbg + "Error updating host from device.\n";
    	throw NvdException ( err.c_str(), __FILE__, __LINE__ );
    }
    
    // Set updated states on host
    int idx = 0;
    uint * var_domain = nullptr;
    for ( auto var : _variables ) 
    {
        // Exit asap if propagation failed
        if ( _h_domain_states [ idx ] == FAL_EVT )
        {
            return false;
        }
        
        // Differentiate between Boolean domains and standard domains
        if ( _bool_var_lookup.find ( var->get_id() ) != _bool_var_lookup.end () )
        {// Boolean domain representation
        	
            if ( _h_domain_states [ idx ] == SNG_EVT )
            {
            	if ( var_domain == nullptr )
            	{
            		var_domain = new uint[ ( (var->domain_iterator->get_domain_status)() ).first / sizeof(int)];
            	}
            	memcpy ( var_domain, 
            	( (var->domain_iterator->get_domain_status)() ).second , 
            	( (var->domain_iterator->get_domain_status)() ).first);
                var_domain [ EVT ] = SNG_EVT;
                var_domain [ LB ] = var_domain [ UB ] = _h_domain_states [ idx + 1 ];
                var_domain [ DSZ ] = 1;
                (var->domain_iterator->set_domain_status)( (void *) var_domain );
            }
            
            idx += 2;
        }
        else
        {//Standard domain representation
        	bool changed = (((uint*)( (var->domain_iterator)->get_domain_status() ).second)[ 0 ] != _h_domain_states[ idx ]);
            (var->domain_iterator)->set_domain_status( (void *) &_h_domain_states[ idx ] );
            if ( changed ) 
            {
            	var->notify_observers ();
            }
            idx += ( (var->domain_iterator)->get_domain_status() ).first / sizeof(int);
        }
    }
  	delete [] var_domain;
#endif

return true;
}//download_device_state



