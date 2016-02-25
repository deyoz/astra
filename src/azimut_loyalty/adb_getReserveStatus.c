

        /**
         * adb_getReserveStatus.c
         *
         * This file was auto-generated from WSDL
         * by the Apache Axis2/C version: SNAPSHOT  Built on : Mar 10, 2008 (08:35:52 GMT+00:00)
         */

        #include "adb_getReserveStatus.h"
        
               /*
                * implmentation of the getReserveStatus|http://ffpServer.aeronavigator.ru element
                */
           


        struct adb_getReserveStatus
        {
            axis2_char_t *property_Type;

            
                axutil_qname_t* qname;
            adb_ReserveStatusRequest_t* property_request;

                
                axis2_bool_t is_valid_request;
            
        };


       /************************* Private Function prototypes ********************************/
        


       /************************* Function Implmentations ********************************/
        adb_getReserveStatus_t* AXIS2_CALL
        adb_getReserveStatus_create(
            const axutil_env_t *env)
        {
            adb_getReserveStatus_t *_getReserveStatus = NULL;
            
                axutil_qname_t* qname = NULL;
            
            AXIS2_ENV_CHECK(env, NULL);

            _getReserveStatus = (adb_getReserveStatus_t *) AXIS2_MALLOC(env->
                allocator, sizeof(adb_getReserveStatus_t));

            if(NULL == _getReserveStatus)
            {
                AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
                return NULL;
            }

            memset(_getReserveStatus, 0, sizeof(adb_getReserveStatus_t));

            _getReserveStatus->property_Type = axutil_strdup(env, "adb_getReserveStatus");
            _getReserveStatus->property_request  = NULL;
                  _getReserveStatus->is_valid_request  = AXIS2_FALSE;
            
                  qname =  axutil_qname_create (env,
                        "getReserveStatus",
                        "http://ffpServer.aeronavigator.ru",
                        NULL);
                _getReserveStatus->qname = qname;
            

            return _getReserveStatus;
        }

        adb_getReserveStatus_t* AXIS2_CALL
        adb_getReserveStatus_create_with_values(
            const axutil_env_t *env,
                adb_ReserveStatusRequest_t* _request)
        {
            adb_getReserveStatus_t* adb_obj = NULL;
            axis2_status_t status = AXIS2_SUCCESS;

            adb_obj = adb_getReserveStatus_create(env);

            
              status = adb_getReserveStatus_set_request(
                                     adb_obj,
                                     env,
                                     _request);
              if(status == AXIS2_FAILURE) {
                  adb_getReserveStatus_free (adb_obj, env);
                  return NULL;
              }
            

            return adb_obj;
        }
      
        adb_ReserveStatusRequest_t* AXIS2_CALL
                adb_getReserveStatus_free_popping_value(
                        adb_getReserveStatus_t* _getReserveStatus,
                        const axutil_env_t *env)
                {
                    adb_ReserveStatusRequest_t* value;

                    
                    
                    value = _getReserveStatus->property_request;

                    _getReserveStatus->property_request = (adb_ReserveStatusRequest_t*)NULL;
                    adb_getReserveStatus_free(_getReserveStatus, env);

                    return value;
                }
            

        axis2_status_t AXIS2_CALL
        adb_getReserveStatus_free(
                adb_getReserveStatus_t* _getReserveStatus,
                const axutil_env_t *env)
        {
            
            
            return adb_getReserveStatus_free_obj(
                _getReserveStatus,
                env);
            
        }

        axis2_status_t AXIS2_CALL
        adb_getReserveStatus_free_obj(
                adb_getReserveStatus_t* _getReserveStatus,
                const axutil_env_t *env)
        {
            

            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _getReserveStatus, AXIS2_FAILURE);

            if (_getReserveStatus->property_Type != NULL)
            {
              AXIS2_FREE(env->allocator, _getReserveStatus->property_Type);
            }

            adb_getReserveStatus_reset_request(_getReserveStatus, env);
            
              if(_getReserveStatus->qname)
              {
                  axutil_qname_free (_getReserveStatus->qname, env);
                  _getReserveStatus->qname = NULL;
              }
            

            if(_getReserveStatus)
            {
                AXIS2_FREE(env->allocator, _getReserveStatus);
                _getReserveStatus = NULL;
            }

            return AXIS2_SUCCESS;
        }


        

        axis2_status_t AXIS2_CALL
        adb_getReserveStatus_deserialize(
                adb_getReserveStatus_t* _getReserveStatus,
                const axutil_env_t *env,
                axiom_node_t **dp_parent,
                axis2_bool_t *dp_is_early_node_valid,
                axis2_bool_t dont_care_minoccurs)
        {
            
            
            return adb_getReserveStatus_deserialize_obj(
                _getReserveStatus,
                env,
                dp_parent,
                dp_is_early_node_valid,
                dont_care_minoccurs);
            
        }

        axis2_status_t AXIS2_CALL
        adb_getReserveStatus_deserialize_obj(
                adb_getReserveStatus_t* _getReserveStatus,
                const axutil_env_t *env,
                axiom_node_t **dp_parent,
                axis2_bool_t *dp_is_early_node_valid,
                axis2_bool_t dont_care_minoccurs)
        {
          axiom_node_t *parent = *dp_parent;
          
          axis2_status_t status = AXIS2_SUCCESS;
          
              void *element = NULL;
           
             const axis2_char_t* text_value = NULL;
             axutil_qname_t *qname = NULL;
          
            axutil_qname_t *element_qname = NULL; 
            
               axiom_node_t *first_node = NULL;
               axis2_bool_t is_early_node_valid = AXIS2_TRUE;
               axiom_node_t *current_node = NULL;
               axiom_element_t *current_element = NULL;
            
            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _getReserveStatus, AXIS2_FAILURE);

            
              
              while(parent && axiom_node_get_node_type(parent, env) != AXIOM_ELEMENT)
              {
                  parent = axiom_node_get_next_sibling(parent, env);
              }
              if (NULL == parent)
              {
                /* This should be checked before everything */
                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                            "Failed in building adb object for getReserveStatus : "
                            "NULL element can not be passed to deserialize");
                return AXIS2_FAILURE;
              }
              

                    current_element = (axiom_element_t *)axiom_node_get_data_element(parent, env);
                    qname = axiom_element_get_qname(current_element, env, parent);
                    if (axutil_qname_equals(qname, env, _getReserveStatus-> qname))
                    {
                        
                          first_node = axiom_node_get_first_child(parent, env);
                          
                    }
                    else
                    {
                        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                              "Failed in building adb object for getReserveStatus : "
                              "Expected %s but returned %s",
                              axutil_qname_to_string(_getReserveStatus-> qname, env),
                              axutil_qname_to_string(qname, env));
                        
                        return AXIS2_FAILURE;
                    }
                    

                     
                     /*
                      * building request element
                      */
                     
                     
                     
                                   current_node = first_node;
                                   is_early_node_valid = AXIS2_FALSE;
                                   
                                   
                                    while(current_node && axiom_node_get_node_type(current_node, env) != AXIOM_ELEMENT)
                                    {
                                        current_node = axiom_node_get_next_sibling(current_node, env);
                                    }
                                    if(current_node != NULL)
                                    {
                                        current_element = (axiom_element_t *)axiom_node_get_data_element(current_node, env);
                                        qname = axiom_element_get_qname(current_element, env, current_node);
                                    }
                                   
                                 element_qname = axutil_qname_create(env, "request", "http://ffpServer.aeronavigator.ru", NULL);
                                 

                           if (adb_ReserveStatusRequest_is_particle() ||  
                                (current_node   && current_element && (axutil_qname_equals(element_qname, env, qname))))
                           {
                              if( current_node   && current_element && (axutil_qname_equals(element_qname, env, qname)))
                              {
                                is_early_node_valid = AXIS2_TRUE;
                              }
                              
                                 
                                      element = (void*)axis2_extension_mapper_create_from_node(env, &current_node, "adb_ReserveStatusRequest");

                                      status =  adb_ReserveStatusRequest_deserialize((adb_ReserveStatusRequest_t*)element,
                                                                            env, &current_node, &is_early_node_valid, AXIS2_FALSE);
                                      if(AXIS2_FAILURE == status)
                                      {
                                          AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in building adb object for element request");
                                      }
                                      else
                                      {
                                          status = adb_getReserveStatus_set_request(_getReserveStatus, env,
                                                                   (adb_ReserveStatusRequest_t*)element);
                                      }
                                    
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for request ");
                                     if(element_qname)
                                     {
                                         axutil_qname_free(element_qname, env);
                                     }
                                     return AXIS2_FAILURE;
                                 }
                              }
                           
                  if(element_qname)
                  {
                     axutil_qname_free(element_qname, env);
                     element_qname = NULL;
                  }
                 
          return status;
       }

          axis2_bool_t AXIS2_CALL
          adb_getReserveStatus_is_particle()
          {
            
                 return AXIS2_FALSE;
              
          }


          void AXIS2_CALL
          adb_getReserveStatus_declare_parent_namespaces(
                    adb_getReserveStatus_t* _getReserveStatus,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index)
          {
            
                  /* Here this is an empty function, Nothing to declare */
                 
          }

        
        
        axiom_node_t* AXIS2_CALL
        adb_getReserveStatus_serialize(
                adb_getReserveStatus_t* _getReserveStatus,
                const axutil_env_t *env, axiom_node_t *parent, axiom_element_t *parent_element, int parent_tag_closed, axutil_hash_t *namespaces, int *next_ns_index)
        {
            
            
                return adb_getReserveStatus_serialize_obj(
                    _getReserveStatus, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index);
            
        }

        axiom_node_t* AXIS2_CALL
        adb_getReserveStatus_serialize_obj(
                adb_getReserveStatus_t* _getReserveStatus,
                const axutil_env_t *env, axiom_node_t *parent, axiom_element_t *parent_element, int parent_tag_closed, axutil_hash_t *namespaces, int *next_ns_index)
        {
            
            
         
         axiom_node_t* current_node = NULL;
         int tag_closed = 0;
         
                axiom_namespace_t *ns1 = NULL;

                axis2_char_t *qname_uri = NULL;
                axis2_char_t *qname_prefix = NULL;
                axis2_char_t *p_prefix = NULL;
                axis2_bool_t ns_already_defined;
            
                    axis2_char_t text_value_1[ADB_DEFAULT_DIGIT_LIMIT];
                    
               axis2_char_t *start_input_str = NULL;
               axis2_char_t *end_input_str = NULL;
               unsigned int start_input_str_len = 0;
               unsigned int end_input_str_len = 0;
            
            
               axiom_data_source_t *data_source = NULL;
               axutil_stream_t *stream = NULL;

             
                int next_ns_index_value = 0;
            

            AXIS2_ENV_CHECK(env, NULL);
            AXIS2_PARAM_CHECK(env->error, _getReserveStatus, NULL);
            
             
                    namespaces = axutil_hash_make(env);
                    next_ns_index = &next_ns_index_value;
                     
                           ns1 = axiom_namespace_create (env,
                                             "http://ffpServer.aeronavigator.ru",
                                             "n"); 
                           axutil_hash_set(namespaces, "http://ffpServer.aeronavigator.ru", AXIS2_HASH_KEY_STRING, axutil_strdup(env, "n"));
                       
                     
                    parent_element = axiom_element_create (env, NULL, "getReserveStatus", ns1 , &parent);
                    
                    
                    axiom_element_set_namespace(parent_element, env, ns1, parent);


            
                    data_source = axiom_data_source_create(env, parent, &current_node);
                    stream = axiom_data_source_get_stream(data_source, env);
                  
                       if(!(p_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://ffpServer.aeronavigator.ru", AXIS2_HASH_KEY_STRING)))
                       {
                           p_prefix = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof (axis2_char_t) * ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT);
                           sprintf(p_prefix, "n%d", (*next_ns_index)++);
                           axutil_hash_set(namespaces, "http://ffpServer.aeronavigator.ru", AXIS2_HASH_KEY_STRING, p_prefix);
                           
                           axiom_element_declare_namespace_assume_param_ownership(parent_element, env, axiom_namespace_create (env,
                                            "http://ffpServer.aeronavigator.ru",
                                            p_prefix));
                       }
                      

                   if (!_getReserveStatus->is_valid_request)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("request"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("request")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing request element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%srequest",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":""); 
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%srequest>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                     
                            if(!adb_ReserveStatusRequest_is_particle())
                            {
                                axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                            }
                            adb_ReserveStatusRequest_serialize(_getReserveStatus->property_request, 
                                                                                 env, current_node, parent_element,
                                                                                 adb_ReserveStatusRequest_is_particle() || AXIS2_FALSE, namespaces, next_ns_index);
                            
                            if(!adb_ReserveStatusRequest_is_particle())
                            {
                                axutil_stream_write(stream, env, end_input_str, end_input_str_len);
                            }
                            
                     
                     AXIS2_FREE(env->allocator,start_input_str);
                     AXIS2_FREE(env->allocator,end_input_str);
                 } 

                 
                   if(namespaces)
                   {
                       axutil_hash_index_t *hi;
                       void *val;
                       for (hi = axutil_hash_first(namespaces, env); hi; hi = axutil_hash_next(env, hi)) 
                       {
                           axutil_hash_this(hi, NULL, NULL, &val);
                           AXIS2_FREE(env->allocator, val);
                       }
                       axutil_hash_free(namespaces, env);
                   }
                

            return parent;
        }


        

            /**
             * Getter for request by  Property Number 1
             */
            adb_ReserveStatusRequest_t* AXIS2_CALL
            adb_getReserveStatus_get_property1(
                adb_getReserveStatus_t* _getReserveStatus,
                const axutil_env_t *env)
            {
                return adb_getReserveStatus_get_request(_getReserveStatus,
                                             env);
            }

            /**
             * getter for request.
             */
            adb_ReserveStatusRequest_t* AXIS2_CALL
            adb_getReserveStatus_get_request(
                    adb_getReserveStatus_t* _getReserveStatus,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _getReserveStatus, NULL);
                  

                return _getReserveStatus->property_request;
             }

            /**
             * setter for request
             */
            axis2_status_t AXIS2_CALL
            adb_getReserveStatus_set_request(
                    adb_getReserveStatus_t* _getReserveStatus,
                    const axutil_env_t *env,
                    adb_ReserveStatusRequest_t*  arg_request)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _getReserveStatus, AXIS2_FAILURE);
                
                if(_getReserveStatus->is_valid_request &&
                        arg_request == _getReserveStatus->property_request)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_getReserveStatus_reset_request(_getReserveStatus, env);

                
                if(NULL == arg_request)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _getReserveStatus->property_request = arg_request;
                        _getReserveStatus->is_valid_request = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for request
            */
           axis2_status_t AXIS2_CALL
           adb_getReserveStatus_reset_request(
                   adb_getReserveStatus_t* _getReserveStatus,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _getReserveStatus, AXIS2_FAILURE);
               

               
            
                
                if(_getReserveStatus->property_request != NULL)
                {
                   
                   adb_ReserveStatusRequest_free(_getReserveStatus->property_request, env);
                     _getReserveStatus->property_request = NULL;
                }
            
                
                
                _getReserveStatus->is_valid_request = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether request is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_getReserveStatus_is_request_nil(
                   adb_getReserveStatus_t* _getReserveStatus,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _getReserveStatus, AXIS2_TRUE);
               
               return !_getReserveStatus->is_valid_request;
           }

           /**
            * Set request to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_getReserveStatus_set_request_nil(
                   adb_getReserveStatus_t* _getReserveStatus,
                   const axutil_env_t *env)
           {
               return adb_getReserveStatus_reset_request(_getReserveStatus, env);
           }

           

