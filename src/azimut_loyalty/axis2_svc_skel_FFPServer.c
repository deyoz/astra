

        /**
         * axis2_svc_skel_FFPServer.c
         *
         * This file was auto-generated from WSDL for "FFPServer|http://ffpServer.aeronavigator.ru" service
         * by the Apache Axis2 version: 1.7.0  Built on : Jan 18, 2016 (09:41:27 GMT)
         *  axis2_svc_skel_FFPServer
         */

        #include "axis2_skel_FFPServer.h"
        #include <axis2_svc_skeleton.h>
        #include <stdio.h>
        #include <axis2_svc.h>

        #ifdef __cplusplus
        extern "C" {
        #endif

        /**
         * creating a custom structure to wrap the axis2_svc_skeleton class
         */
        typedef struct {
            axis2_svc_skeleton_t svc_skeleton;

            /* union to keep all the exception objects */
          
        }axis2_svc_skel_FFPServer_t;
       
        /**
         * functions prototypes
         */
        /* On fault, handle the fault */
        axiom_node_t* AXIS2_CALL
        axis2_svc_skel_FFPServer_on_fault(axis2_svc_skeleton_t *svc_skeleton,
                  const axutil_env_t *env, axiom_node_t *node);

        /* Free the service */
        int AXIS2_CALL
        axis2_svc_skel_FFPServer_free(axis2_svc_skeleton_t *svc_skeleton,
                  const axutil_env_t *env);

        /* This method invokes the right service method */
        axiom_node_t* AXIS2_CALL
        axis2_svc_skel_FFPServer_invoke(axis2_svc_skeleton_t *svc_skeleton,
                    const axutil_env_t *env,
                    axiom_node_t *node,
                    axis2_msg_ctx_t *msg_ctx);

        /* Initializing the environment  */
        int AXIS2_CALL
        axis2_svc_skel_FFPServer_init(axis2_svc_skeleton_t *svc_skeleton,
                        const axutil_env_t *env);

        /* Create the service  */
        axis2_svc_skeleton_t* AXIS2_CALL
        axis2_svc_skel_FFPServer_create(const axutil_env_t *env);

        static const axis2_svc_skeleton_ops_t axis2_svc_skel_FFPServer_svc_skeleton_ops_var = {
            axis2_svc_skel_FFPServer_init,
            axis2_svc_skel_FFPServer_invoke,
            axis2_svc_skel_FFPServer_on_fault,
            axis2_svc_skel_FFPServer_free
        };


        /**
         * Following block distinguish the exposed part of the dll.
         * create the instance
         */

        AXIS2_EXTERN int
        axis2_get_instance(struct axis2_svc_skeleton **inst,
                                const axutil_env_t *env);

        AXIS2_EXTERN int 
        axis2_remove_instance(axis2_svc_skeleton_t *inst,
                                const axutil_env_t *env);


         /**
          * function to free any soap input headers
          */
         


        #ifdef __cplusplus
        }
        #endif


        /**
         * Implementations for the functions
         */

	axis2_svc_skeleton_t* AXIS2_CALL
	axis2_svc_skel_FFPServer_create(const axutil_env_t *env)
	{
	    axis2_svc_skel_FFPServer_t *svc_skeleton_wrapper = NULL;
	    axis2_svc_skeleton_t *svc_skeleton = NULL;
        
        /* Allocate memory for the structs */
        svc_skeleton_wrapper = (axis2_svc_skel_FFPServer_t *)AXIS2_MALLOC(env->allocator,
            sizeof(axis2_svc_skel_FFPServer_t));

        svc_skeleton = (axis2_svc_skeleton_t*)svc_skeleton_wrapper;

        svc_skeleton->ops = &axis2_svc_skel_FFPServer_svc_skeleton_ops_var;

	    return svc_skeleton;
	}


	int AXIS2_CALL
	axis2_svc_skel_FFPServer_init(axis2_svc_skeleton_t *svc_skeleton,
	                        const axutil_env_t *env)
	{
	    /* Nothing special in initialization  axis2_skel_FFPServer */
	    return AXIS2_SUCCESS;
	}

	int AXIS2_CALL
	axis2_svc_skel_FFPServer_free(axis2_svc_skeleton_t *svc_skeleton,
				 const axutil_env_t *env)
	{

        /* Free the service skeleton */
        if (svc_skeleton)
        {
            AXIS2_FREE(env->allocator, svc_skeleton);
            svc_skeleton = NULL;
        }

        return AXIS2_SUCCESS;
	}



     




	/*
	 * This method invokes the right service method
	 */
	axiom_node_t* AXIS2_CALL
	axis2_svc_skel_FFPServer_invoke(axis2_svc_skeleton_t *svc_skeleton,
				const axutil_env_t *env,
				axiom_node_t *content_node,
				axis2_msg_ctx_t *msg_ctx)
	{
         /* depending on the function name invoke the
          * corresponding  method
          */

          axis2_op_ctx_t *operation_ctx = NULL;
          axis2_op_t *operation = NULL;
          axutil_qname_t *op_qname = NULL;
          axis2_char_t *op_name = NULL;
          axis2_msg_ctx_t *in_msg_ctx = NULL;
          
          axiom_soap_envelope_t *req_soap_env = NULL;
          axiom_soap_header_t *req_soap_header = NULL;
          axiom_soap_envelope_t *res_soap_env = NULL;
          axiom_soap_header_t *res_soap_header = NULL;

          axiom_node_t *ret_node = NULL;
          axiom_node_t *input_header = NULL;
          axiom_node_t *output_header = NULL;
          axiom_node_t *header_base_node = NULL;
	    
          axis2_svc_skel_FFPServer_t *svc_skeleton_wrapper = NULL;

          
            adb_loginResponseE3_t* ret_val1;
            adb_login_t* input_val1;
            
            adb_payMilesResponseE1_t* ret_val2;
            adb_payMiles_t* input_val2;
            
            adb_getPriceResponse_t* ret_val3;
            adb_getPrice_t* input_val3;
            
            adb_getBonusResponse_t* ret_val4;
            adb_getBonus_t* input_val4;
            
            adb_getReserveStatusResponse_t* ret_val5;
            adb_getReserveStatus_t* input_val5;
            
            adb_cancelReserveResponseE2_t* ret_val6;
            adb_cancelReserve_t* input_val6;
            
            adb_getAvailableMilesResponse_t* ret_val7;
            adb_getAvailableMiles_t* input_val7;
            
            adb_reserveMilesResponseE0_t* ret_val8;
            adb_reserveMiles_t* input_val8;
            

          svc_skeleton_wrapper = (axis2_svc_skel_FFPServer_t*)svc_skeleton;
          operation_ctx = axis2_msg_ctx_get_op_ctx(msg_ctx, env);
          operation = axis2_op_ctx_get_op(operation_ctx, env);
          op_qname = (axutil_qname_t *)axis2_op_get_qname(operation, env);
          op_name = axutil_qname_get_localpart(op_qname, env);

          if (op_name)
          {
               

                if ( axutil_strcmp(op_name, "login") == 0 )
                {

                    
                    input_val1 = adb_login_create( env);
                        if( AXIS2_FAILURE == adb_login_deserialize(input_val1, env, &content_node, NULL, AXIS2_FALSE))
                        {
                            adb_login_free(input_val1, env);
                      
                            AXIS2_ERROR_SET(env->error, AXIS2_ERROR_DATA_ELEMENT_IS_NULL, AXIS2_FAILURE);
                            AXIS2_LOG_ERROR( env->log, AXIS2_LOG_SI, "NULL returnted from the adb_login_deserialize: "
                                        "This should be due to an invalid XML");
                            return NULL;      
                        }
                        
                        ret_val1 =  axis2_skel_FFPServer_login(env, msg_ctx,input_val1);
                    
                        if ( NULL == ret_val1 )
                        {
                            adb_login_free(input_val1, env);
                            
                            return NULL; 
                        }
                        ret_node = adb_loginResponseE3_serialize(ret_val1, env, NULL, NULL, AXIS2_TRUE, NULL, NULL);
                                   adb_loginResponseE3_free(ret_val1, env);
                                   adb_login_free(input_val1, env);
                                   

                        return ret_node;
                    

                    /* since this has no output params it just returns NULL */                    
                    

                }
             

                if ( axutil_strcmp(op_name, "payMiles") == 0 )
                {

                    
                    input_val2 = adb_payMiles_create( env);
                        if( AXIS2_FAILURE == adb_payMiles_deserialize(input_val2, env, &content_node, NULL, AXIS2_FALSE))
                        {
                            adb_payMiles_free(input_val2, env);
                      
                            AXIS2_ERROR_SET(env->error, AXIS2_ERROR_DATA_ELEMENT_IS_NULL, AXIS2_FAILURE);
                            AXIS2_LOG_ERROR( env->log, AXIS2_LOG_SI, "NULL returnted from the adb_payMiles_deserialize: "
                                        "This should be due to an invalid XML");
                            return NULL;      
                        }
                        
                        ret_val2 =  axis2_skel_FFPServer_payMiles(env, msg_ctx,input_val2);
                    
                        if ( NULL == ret_val2 )
                        {
                            adb_payMiles_free(input_val2, env);
                            
                            return NULL; 
                        }
                        ret_node = adb_payMilesResponseE1_serialize(ret_val2, env, NULL, NULL, AXIS2_TRUE, NULL, NULL);
                                   adb_payMilesResponseE1_free(ret_val2, env);
                                   adb_payMiles_free(input_val2, env);
                                   

                        return ret_node;
                    

                    /* since this has no output params it just returns NULL */                    
                    

                }
             

                if ( axutil_strcmp(op_name, "getPrice") == 0 )
                {

                    
                    input_val3 = adb_getPrice_create( env);
                        if( AXIS2_FAILURE == adb_getPrice_deserialize(input_val3, env, &content_node, NULL, AXIS2_FALSE))
                        {
                            adb_getPrice_free(input_val3, env);
                      
                            AXIS2_ERROR_SET(env->error, AXIS2_ERROR_DATA_ELEMENT_IS_NULL, AXIS2_FAILURE);
                            AXIS2_LOG_ERROR( env->log, AXIS2_LOG_SI, "NULL returnted from the adb_getPrice_deserialize: "
                                        "This should be due to an invalid XML");
                            return NULL;      
                        }
                        
                        ret_val3 =  axis2_skel_FFPServer_getPrice(env, msg_ctx,input_val3);
                    
                        if ( NULL == ret_val3 )
                        {
                            adb_getPrice_free(input_val3, env);
                            
                            return NULL; 
                        }
                        ret_node = adb_getPriceResponse_serialize(ret_val3, env, NULL, NULL, AXIS2_TRUE, NULL, NULL);
                                   adb_getPriceResponse_free(ret_val3, env);
                                   adb_getPrice_free(input_val3, env);
                                   

                        return ret_node;
                    

                    /* since this has no output params it just returns NULL */                    
                    

                }
             

                if ( axutil_strcmp(op_name, "getBonus") == 0 )
                {

                    
                    input_val4 = adb_getBonus_create( env);
                        if( AXIS2_FAILURE == adb_getBonus_deserialize(input_val4, env, &content_node, NULL, AXIS2_FALSE))
                        {
                            adb_getBonus_free(input_val4, env);
                      
                            AXIS2_ERROR_SET(env->error, AXIS2_ERROR_DATA_ELEMENT_IS_NULL, AXIS2_FAILURE);
                            AXIS2_LOG_ERROR( env->log, AXIS2_LOG_SI, "NULL returnted from the adb_getBonus_deserialize: "
                                        "This should be due to an invalid XML");
                            return NULL;      
                        }
                        
                        ret_val4 =  axis2_skel_FFPServer_getBonus(env, msg_ctx,input_val4);
                    
                        if ( NULL == ret_val4 )
                        {
                            adb_getBonus_free(input_val4, env);
                            
                            return NULL; 
                        }
                        ret_node = adb_getBonusResponse_serialize(ret_val4, env, NULL, NULL, AXIS2_TRUE, NULL, NULL);
                                   adb_getBonusResponse_free(ret_val4, env);
                                   adb_getBonus_free(input_val4, env);
                                   

                        return ret_node;
                    

                    /* since this has no output params it just returns NULL */                    
                    

                }
             

                if ( axutil_strcmp(op_name, "getReserveStatus") == 0 )
                {

                    
                    input_val5 = adb_getReserveStatus_create( env);
                        if( AXIS2_FAILURE == adb_getReserveStatus_deserialize(input_val5, env, &content_node, NULL, AXIS2_FALSE))
                        {
                            adb_getReserveStatus_free(input_val5, env);
                      
                            AXIS2_ERROR_SET(env->error, AXIS2_ERROR_DATA_ELEMENT_IS_NULL, AXIS2_FAILURE);
                            AXIS2_LOG_ERROR( env->log, AXIS2_LOG_SI, "NULL returnted from the adb_getReserveStatus_deserialize: "
                                        "This should be due to an invalid XML");
                            return NULL;      
                        }
                        
                        ret_val5 =  axis2_skel_FFPServer_getReserveStatus(env, msg_ctx,input_val5);
                    
                        if ( NULL == ret_val5 )
                        {
                            adb_getReserveStatus_free(input_val5, env);
                            
                            return NULL; 
                        }
                        ret_node = adb_getReserveStatusResponse_serialize(ret_val5, env, NULL, NULL, AXIS2_TRUE, NULL, NULL);
                                   adb_getReserveStatusResponse_free(ret_val5, env);
                                   adb_getReserveStatus_free(input_val5, env);
                                   

                        return ret_node;
                    

                    /* since this has no output params it just returns NULL */                    
                    

                }
             

                if ( axutil_strcmp(op_name, "cancelReserve") == 0 )
                {

                    
                    input_val6 = adb_cancelReserve_create( env);
                        if( AXIS2_FAILURE == adb_cancelReserve_deserialize(input_val6, env, &content_node, NULL, AXIS2_FALSE))
                        {
                            adb_cancelReserve_free(input_val6, env);
                      
                            AXIS2_ERROR_SET(env->error, AXIS2_ERROR_DATA_ELEMENT_IS_NULL, AXIS2_FAILURE);
                            AXIS2_LOG_ERROR( env->log, AXIS2_LOG_SI, "NULL returnted from the adb_cancelReserve_deserialize: "
                                        "This should be due to an invalid XML");
                            return NULL;      
                        }
                        
                        ret_val6 =  axis2_skel_FFPServer_cancelReserve(env, msg_ctx,input_val6);
                    
                        if ( NULL == ret_val6 )
                        {
                            adb_cancelReserve_free(input_val6, env);
                            
                            return NULL; 
                        }
                        ret_node = adb_cancelReserveResponseE2_serialize(ret_val6, env, NULL, NULL, AXIS2_TRUE, NULL, NULL);
                                   adb_cancelReserveResponseE2_free(ret_val6, env);
                                   adb_cancelReserve_free(input_val6, env);
                                   

                        return ret_node;
                    

                    /* since this has no output params it just returns NULL */                    
                    

                }
             

                if ( axutil_strcmp(op_name, "getAvailableMiles") == 0 )
                {

                    
                    input_val7 = adb_getAvailableMiles_create( env);
                        if( AXIS2_FAILURE == adb_getAvailableMiles_deserialize(input_val7, env, &content_node, NULL, AXIS2_FALSE))
                        {
                            adb_getAvailableMiles_free(input_val7, env);
                      
                            AXIS2_ERROR_SET(env->error, AXIS2_ERROR_DATA_ELEMENT_IS_NULL, AXIS2_FAILURE);
                            AXIS2_LOG_ERROR( env->log, AXIS2_LOG_SI, "NULL returnted from the adb_getAvailableMiles_deserialize: "
                                        "This should be due to an invalid XML");
                            return NULL;      
                        }
                        
                        ret_val7 =  axis2_skel_FFPServer_getAvailableMiles(env, msg_ctx,input_val7);
                    
                        if ( NULL == ret_val7 )
                        {
                            adb_getAvailableMiles_free(input_val7, env);
                            
                            return NULL; 
                        }
                        ret_node = adb_getAvailableMilesResponse_serialize(ret_val7, env, NULL, NULL, AXIS2_TRUE, NULL, NULL);
                                   adb_getAvailableMilesResponse_free(ret_val7, env);
                                   adb_getAvailableMiles_free(input_val7, env);
                                   

                        return ret_node;
                    

                    /* since this has no output params it just returns NULL */                    
                    

                }
             

                if ( axutil_strcmp(op_name, "reserveMiles") == 0 )
                {

                    
                    input_val8 = adb_reserveMiles_create( env);
                        if( AXIS2_FAILURE == adb_reserveMiles_deserialize(input_val8, env, &content_node, NULL, AXIS2_FALSE))
                        {
                            adb_reserveMiles_free(input_val8, env);
                      
                            AXIS2_ERROR_SET(env->error, AXIS2_ERROR_DATA_ELEMENT_IS_NULL, AXIS2_FAILURE);
                            AXIS2_LOG_ERROR( env->log, AXIS2_LOG_SI, "NULL returnted from the adb_reserveMiles_deserialize: "
                                        "This should be due to an invalid XML");
                            return NULL;      
                        }
                        
                        ret_val8 =  axis2_skel_FFPServer_reserveMiles(env, msg_ctx,input_val8);
                    
                        if ( NULL == ret_val8 )
                        {
                            adb_reserveMiles_free(input_val8, env);
                            
                            return NULL; 
                        }
                        ret_node = adb_reserveMilesResponseE0_serialize(ret_val8, env, NULL, NULL, AXIS2_TRUE, NULL, NULL);
                                   adb_reserveMilesResponseE0_free(ret_val8, env);
                                   adb_reserveMiles_free(input_val8, env);
                                   

                        return ret_node;
                    

                    /* since this has no output params it just returns NULL */                    
                    

                }
             
             }
            
          AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "axis2_svc_skel_FFPServer service ERROR: invalid OM parameters in request\n");
          return NULL;
    }

    axiom_node_t* AXIS2_CALL
    axis2_svc_skel_FFPServer_on_fault(axis2_svc_skeleton_t *svc_skeleton,
                  const axutil_env_t *env, axiom_node_t *node)
	{
		axiom_node_t *error_node = NULL;
		axiom_element_t *error_ele = NULL;
        axutil_error_codes_t error_code;
        axis2_svc_skel_FFPServer_t *svc_skeleton_wrapper = NULL;

        svc_skeleton_wrapper = (axis2_svc_skel_FFPServer_t*)svc_skeleton;

        error_code = env->error->error_number;

        if(error_code <= AXIS2_SKEL_FFPSERVER_ERROR_NONE ||
                error_code >= AXIS2_SKEL_FFPSERVER_ERROR_LAST )
        {
            error_ele = axiom_element_create(env, node, "fault", NULL,
                            &error_node);
            axiom_element_set_text(error_ele, env, "FFPServer|http://ffpServer.aeronavigator.ru failed",
                            error_node);
        }
        

		return error_node;
	}


	/**
	 * Following block distinguish the exposed part of the dll.
 	 */

    AXIS2_EXTERN int
    axis2_get_instance(struct axis2_svc_skeleton **inst,
	                        const axutil_env_t *env)
	{
		*inst = axis2_svc_skel_FFPServer_create(env);

        if(!(*inst))
        {
            return AXIS2_FAILURE;
        }

  		return AXIS2_SUCCESS;
	}

	AXIS2_EXTERN int 
    axis2_remove_instance(axis2_svc_skeleton_t *inst,
                            const axutil_env_t *env)
	{
        axis2_status_t status = AXIS2_FAILURE;
       	if (inst)
        {
            status = AXIS2_SVC_SKELETON_FREE(inst, env);
        }
    	return status;
	}


    

