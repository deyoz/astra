

        /**
         * adb_LoginResponse.c
         *
         * This file was auto-generated from WSDL
         * by the Apache Axis2/C version: SNAPSHOT  Built on : Mar 10, 2008 (08:35:52 GMT+00:00)
         */

        #include "adb_LoginResponse.h"
        
                /*
                 * This type was generated from the piece of schema that had
                 * name = LoginResponse
                 * Namespace URI = http://ffpServer.aeronavigator.ru/xsd
                 * Namespace Prefix = ns1
                 */
           


        struct adb_LoginResponse
        {
            axis2_char_t *property_Type;

            axutil_array_list_t* property_errors;

                
                axis2_bool_t is_valid_errors;
            axis2_bool_t property_access;

                
                axis2_bool_t is_valid_access;
            axutil_date_time_t* property_dob;

                
                axis2_bool_t is_valid_dob;
            axis2_char_t* property_email;

                
                axis2_bool_t is_valid_email;
            axis2_char_t* property_lastName;

                
                axis2_bool_t is_valid_lastName;
            axis2_char_t* property_name;

                
                axis2_bool_t is_valid_name;
            axis2_char_t* property_secondName;

                
                axis2_bool_t is_valid_secondName;
            axis2_char_t* property_status;

                
                axis2_bool_t is_valid_status;
            
        };


       /************************* Private Function prototypes ********************************/
        


       /************************* Function Implmentations ********************************/
        adb_LoginResponse_t* AXIS2_CALL
        adb_LoginResponse_create(
            const axutil_env_t *env)
        {
            adb_LoginResponse_t *_LoginResponse = NULL;
            
            AXIS2_ENV_CHECK(env, NULL);

            _LoginResponse = (adb_LoginResponse_t *) AXIS2_MALLOC(env->
                allocator, sizeof(adb_LoginResponse_t));

            if(NULL == _LoginResponse)
            {
                AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
                return NULL;
            }

            memset(_LoginResponse, 0, sizeof(adb_LoginResponse_t));

            _LoginResponse->property_Type = axutil_strdup(env, "adb_LoginResponse");
            _LoginResponse->property_errors  = NULL;
                  _LoginResponse->is_valid_errors  = AXIS2_FALSE;
            _LoginResponse->is_valid_access  = AXIS2_FALSE;
            _LoginResponse->property_dob  = NULL;
                  _LoginResponse->is_valid_dob  = AXIS2_FALSE;
            _LoginResponse->property_email  = NULL;
                  _LoginResponse->is_valid_email  = AXIS2_FALSE;
            _LoginResponse->property_lastName  = NULL;
                  _LoginResponse->is_valid_lastName  = AXIS2_FALSE;
            _LoginResponse->property_name  = NULL;
                  _LoginResponse->is_valid_name  = AXIS2_FALSE;
            _LoginResponse->property_secondName  = NULL;
                  _LoginResponse->is_valid_secondName  = AXIS2_FALSE;
            _LoginResponse->property_status  = NULL;
                  _LoginResponse->is_valid_status  = AXIS2_FALSE;
            

            return _LoginResponse;
        }

        adb_LoginResponse_t* AXIS2_CALL
        adb_LoginResponse_create_with_values(
            const axutil_env_t *env,
                axutil_array_list_t* _errors,
                axis2_bool_t _access,
                axutil_date_time_t* _dob,
                axis2_char_t* _email,
                axis2_char_t* _lastName,
                axis2_char_t* _name,
                axis2_char_t* _secondName,
                axis2_char_t* _status)
        {
            adb_LoginResponse_t* adb_obj = NULL;
            axis2_status_t status = AXIS2_SUCCESS;

            adb_obj = adb_LoginResponse_create(env);

            
              status = adb_LoginResponse_set_errors(
                                     adb_obj,
                                     env,
                                     _errors);
              if(status == AXIS2_FAILURE) {
                  adb_LoginResponse_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_LoginResponse_set_access(
                                     adb_obj,
                                     env,
                                     _access);
              if(status == AXIS2_FAILURE) {
                  adb_LoginResponse_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_LoginResponse_set_dob(
                                     adb_obj,
                                     env,
                                     _dob);
              if(status == AXIS2_FAILURE) {
                  adb_LoginResponse_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_LoginResponse_set_email(
                                     adb_obj,
                                     env,
                                     _email);
              if(status == AXIS2_FAILURE) {
                  adb_LoginResponse_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_LoginResponse_set_lastName(
                                     adb_obj,
                                     env,
                                     _lastName);
              if(status == AXIS2_FAILURE) {
                  adb_LoginResponse_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_LoginResponse_set_name(
                                     adb_obj,
                                     env,
                                     _name);
              if(status == AXIS2_FAILURE) {
                  adb_LoginResponse_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_LoginResponse_set_secondName(
                                     adb_obj,
                                     env,
                                     _secondName);
              if(status == AXIS2_FAILURE) {
                  adb_LoginResponse_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_LoginResponse_set_status(
                                     adb_obj,
                                     env,
                                     _status);
              if(status == AXIS2_FAILURE) {
                  adb_LoginResponse_free (adb_obj, env);
                  return NULL;
              }
            

            return adb_obj;
        }
      
        axutil_array_list_t* AXIS2_CALL
                adb_LoginResponse_free_popping_value(
                        adb_LoginResponse_t* _LoginResponse,
                        const axutil_env_t *env)
                {
                    axutil_array_list_t* value;

                    
                    
                    value = _LoginResponse->property_errors;

                    _LoginResponse->property_errors = (axutil_array_list_t*)NULL;
                    adb_LoginResponse_free(_LoginResponse, env);

                    return value;
                }
            

        axis2_status_t AXIS2_CALL
        adb_LoginResponse_free(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env)
        {
            
            
            return axis2_extension_mapper_free(
                (adb_type_t*) _LoginResponse,
                env,
                "adb_LoginResponse");
            
        }

        axis2_status_t AXIS2_CALL
        adb_LoginResponse_free_obj(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env)
        {
            
                int i = 0;
                int count = 0;
                void *element = NULL;
            

            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);

            if (_LoginResponse->property_Type != NULL)
            {
              AXIS2_FREE(env->allocator, _LoginResponse->property_Type);
            }

            adb_LoginResponse_reset_errors(_LoginResponse, env);
            adb_LoginResponse_reset_access(_LoginResponse, env);
            adb_LoginResponse_reset_dob(_LoginResponse, env);
            adb_LoginResponse_reset_email(_LoginResponse, env);
            adb_LoginResponse_reset_lastName(_LoginResponse, env);
            adb_LoginResponse_reset_name(_LoginResponse, env);
            adb_LoginResponse_reset_secondName(_LoginResponse, env);
            adb_LoginResponse_reset_status(_LoginResponse, env);
            

            if(_LoginResponse)
            {
                AXIS2_FREE(env->allocator, _LoginResponse);
                _LoginResponse = NULL;
            }

            return AXIS2_SUCCESS;
        }


        

        axis2_status_t AXIS2_CALL
        adb_LoginResponse_deserialize(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env,
                axiom_node_t **dp_parent,
                axis2_bool_t *dp_is_early_node_valid,
                axis2_bool_t dont_care_minoccurs)
        {
            
            
            return axis2_extension_mapper_deserialize(
                (adb_type_t*) _LoginResponse,
                env,
                dp_parent,
                dp_is_early_node_valid,
                dont_care_minoccurs,
                "adb_LoginResponse");
            
        }

        axis2_status_t AXIS2_CALL
        adb_LoginResponse_deserialize_obj(
                adb_LoginResponse_t* _LoginResponse,
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
          
               int i = 0;
               axutil_array_list_t *arr_list = NULL;
            
               int sequence_broken = 0;
               axiom_node_t *tmp_node = NULL;
            
            axutil_qname_t *element_qname = NULL; 
            
               axiom_node_t *first_node = NULL;
               axis2_bool_t is_early_node_valid = AXIS2_TRUE;
               axiom_node_t *current_node = NULL;
               axiom_element_t *current_element = NULL;
            
            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);

            
              
              while(parent && axiom_node_get_node_type(parent, env) != AXIOM_ELEMENT)
              {
                  parent = axiom_node_get_next_sibling(parent, env);
              }
              if (NULL == parent)
              {
                /* This should be checked before everything */
                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                            "Failed in building adb object for LoginResponse : "
                            "NULL element can not be passed to deserialize");
                return AXIS2_FAILURE;
              }
              
                      
                      first_node = axiom_node_get_first_child(parent, env);
                      
                    
                    /*
                     * building errors array
                     */
                       arr_list = axutil_array_list_create(env, 10);
                   

                     
                     /*
                      * building errors element
                      */
                     
                     
                     
                                    element_qname = axutil_qname_create(env, "errors", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                  
                               
                               for (i = 0, sequence_broken = 0, current_node = first_node; !sequence_broken && current_node != NULL;) 
                                             
                               {
                                  if(axiom_node_get_node_type(current_node, env) != AXIOM_ELEMENT)
                                  {
                                     current_node =axiom_node_get_next_sibling(current_node, env);
                                     is_early_node_valid = AXIS2_FALSE;
                                     continue;
                                  }
                                  
                                  current_element = (axiom_element_t *)axiom_node_get_data_element(current_node, env);
                                  qname = axiom_element_get_qname(current_element, env, current_node);

                                  if (adb_ErrorGroup_is_particle() ||  
                                    (current_node && current_element && (axutil_qname_equals(element_qname, env, qname))))
                                  {
                                  
                                      if( current_node && current_element && (axutil_qname_equals(element_qname, env, qname)))
                                      {
                                          is_early_node_valid = AXIS2_TRUE;
                                      }
                                      
                                     
                                          element = (void*)axis2_extension_mapper_create_from_node(env, &current_node, "adb_ErrorGroup");
                                          
                                          status =  adb_ErrorGroup_deserialize((adb_ErrorGroup_t*)element, env,
                                                                                 &current_node, &is_early_node_valid, AXIS2_FALSE);
                                          
                                          if(AXIS2_FAILURE ==  status)
                                          {
                                              AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in building element errors ");
                                          }
                                          else
                                          {
                                            axutil_array_list_add_at(arr_list, env, i, element);
                                          }
                                        
                                     if(AXIS2_FAILURE ==  status)
                                     {
                                         AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for errors ");
                                         if(element_qname)
                                         {
                                            axutil_qname_free(element_qname, env);
                                         }
                                         if(arr_list)
                                         {
                                            axutil_array_list_free(arr_list, env);
                                         }
                                         return AXIS2_FAILURE;
                                     }

                                     i ++;
                                    current_node = axiom_node_get_next_sibling(current_node, env);
                                  }
                                  else
                                  {
                                      is_early_node_valid = AXIS2_FALSE;
                                      sequence_broken = 1;
                                  }
                                  
                               }

                               
                                   if (i < 0)
                                   {
                                     /* found element out of order */
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "errors (@minOccurs = '0') only have %d elements", i);
                                     if(element_qname)
                                     {
                                        axutil_qname_free(element_qname, env);
                                     }
                                     if(arr_list)
                                     {
                                        axutil_array_list_free(arr_list, env);
                                     }
                                     return AXIS2_FAILURE;
                                   }
                               

                               if(0 == axutil_array_list_size(arr_list,env))
                               {
                                    axutil_array_list_free(arr_list, env);
                               }
                               else
                               {
                                    status = adb_LoginResponse_set_errors(_LoginResponse, env,
                                                                   arr_list);
                               }

                             
                  if(element_qname)
                  {
                     axutil_qname_free(element_qname, env);
                     element_qname = NULL;
                  }
                 

                     
                     /*
                      * building access element
                      */
                     
                     
                     
                                    /*
                                     * because elements are ordered this works fine
                                     */
                                  
                                   
                                   if(current_node != NULL && is_early_node_valid)
                                   {
                                       current_node = axiom_node_get_next_sibling(current_node, env);
                                       
                                       
                                        while(current_node && axiom_node_get_node_type(current_node, env) != AXIOM_ELEMENT)
                                        {
                                            current_node = axiom_node_get_next_sibling(current_node, env);
                                        }
                                        if(current_node != NULL)
                                        {
                                            current_element = (axiom_element_t *)axiom_node_get_data_element(current_node, env);
                                            qname = axiom_element_get_qname(current_element, env, current_node);
                                        }
                                       
                                   }
                                   is_early_node_valid = AXIS2_FALSE;
                                 
                                 element_qname = axutil_qname_create(env, "access", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

                           if ( 
                                (current_node   && current_element && (axutil_qname_equals(element_qname, env, qname))))
                           {
                              if( current_node   && current_element && (axutil_qname_equals(element_qname, env, qname)))
                              {
                                is_early_node_valid = AXIS2_TRUE;
                              }
                              
                                 
                                      text_value = axiom_element_get_text(current_element, env, current_node);
                                      if(text_value != NULL)
                                      {
                                            if (!axutil_strcasecmp(text_value , "true"))
                                            {
                                                status = adb_LoginResponse_set_access(_LoginResponse, env,
                                                                 AXIS2_TRUE);
                                            }
                                            else
                                            {
                                                status = adb_LoginResponse_set_access(_LoginResponse, env,
                                                                      AXIS2_FALSE);
                                            }
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for access ");
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
                 

                     
                     /*
                      * building dob element
                      */
                     
                     
                     
                                    /*
                                     * because elements are ordered this works fine
                                     */
                                  
                                   
                                   if(current_node != NULL && is_early_node_valid)
                                   {
                                       current_node = axiom_node_get_next_sibling(current_node, env);
                                       
                                       
                                        while(current_node && axiom_node_get_node_type(current_node, env) != AXIOM_ELEMENT)
                                        {
                                            current_node = axiom_node_get_next_sibling(current_node, env);
                                        }
                                        if(current_node != NULL)
                                        {
                                            current_element = (axiom_element_t *)axiom_node_get_data_element(current_node, env);
                                            qname = axiom_element_get_qname(current_element, env, current_node);
                                        }
                                       
                                   }
                                   is_early_node_valid = AXIS2_FALSE;
                                 
                                 element_qname = axutil_qname_create(env, "dob", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

                           if ( 
                                (current_node   && current_element && (axutil_qname_equals(element_qname, env, qname))))
                           {
                              if( current_node   && current_element && (axutil_qname_equals(element_qname, env, qname)))
                              {
                                is_early_node_valid = AXIS2_TRUE;
                              }
                              
                                 
                                      text_value = axiom_element_get_text(current_element, env, current_node);
                                      if(text_value != NULL)
                                      {
                                          element = (void*)axutil_date_time_create(env);
                                          status = axutil_date_time_deserialize_date_time((axutil_date_time_t*)element, env,
                                                                          text_value);
                                          if(AXIS2_FAILURE ==  status)
                                          {
                                              if(element != NULL)
                                              {
                                                  axutil_date_time_free((axutil_date_time_t*)element, env);
                                              }
                                              AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in building element dob ");
                                          }
                                          else
                                          {
                                            status = adb_LoginResponse_set_dob(_LoginResponse, env,
                                                                       (axutil_date_time_t*)element);
                                          }
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for dob ");
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
                 

                     
                     /*
                      * building email element
                      */
                     
                     
                     
                                    /*
                                     * because elements are ordered this works fine
                                     */
                                  
                                   
                                   if(current_node != NULL && is_early_node_valid)
                                   {
                                       current_node = axiom_node_get_next_sibling(current_node, env);
                                       
                                       
                                        while(current_node && axiom_node_get_node_type(current_node, env) != AXIOM_ELEMENT)
                                        {
                                            current_node = axiom_node_get_next_sibling(current_node, env);
                                        }
                                        if(current_node != NULL)
                                        {
                                            current_element = (axiom_element_t *)axiom_node_get_data_element(current_node, env);
                                            qname = axiom_element_get_qname(current_element, env, current_node);
                                        }
                                       
                                   }
                                   is_early_node_valid = AXIS2_FALSE;
                                 
                                 element_qname = axutil_qname_create(env, "email", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

                           if ( 
                                (current_node   && current_element && (axutil_qname_equals(element_qname, env, qname))))
                           {
                              if( current_node   && current_element && (axutil_qname_equals(element_qname, env, qname)))
                              {
                                is_early_node_valid = AXIS2_TRUE;
                              }
                              
                                 
                                      text_value = axiom_element_get_text(current_element, env, current_node);
                                      if(text_value != NULL)
                                      {
                                            status = adb_LoginResponse_set_email(_LoginResponse, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for email ");
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
                 

                     
                     /*
                      * building lastName element
                      */
                     
                     
                     
                                    /*
                                     * because elements are ordered this works fine
                                     */
                                  
                                   
                                   if(current_node != NULL && is_early_node_valid)
                                   {
                                       current_node = axiom_node_get_next_sibling(current_node, env);
                                       
                                       
                                        while(current_node && axiom_node_get_node_type(current_node, env) != AXIOM_ELEMENT)
                                        {
                                            current_node = axiom_node_get_next_sibling(current_node, env);
                                        }
                                        if(current_node != NULL)
                                        {
                                            current_element = (axiom_element_t *)axiom_node_get_data_element(current_node, env);
                                            qname = axiom_element_get_qname(current_element, env, current_node);
                                        }
                                       
                                   }
                                   is_early_node_valid = AXIS2_FALSE;
                                 
                                 element_qname = axutil_qname_create(env, "lastName", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

                           if ( 
                                (current_node   && current_element && (axutil_qname_equals(element_qname, env, qname))))
                           {
                              if( current_node   && current_element && (axutil_qname_equals(element_qname, env, qname)))
                              {
                                is_early_node_valid = AXIS2_TRUE;
                              }
                              
                                 
                                      text_value = axiom_element_get_text(current_element, env, current_node);
                                      if(text_value != NULL)
                                      {
                                            status = adb_LoginResponse_set_lastName(_LoginResponse, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for lastName ");
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
                 

                     
                     /*
                      * building name element
                      */
                     
                     
                     
                                    /*
                                     * because elements are ordered this works fine
                                     */
                                  
                                   
                                   if(current_node != NULL && is_early_node_valid)
                                   {
                                       current_node = axiom_node_get_next_sibling(current_node, env);
                                       
                                       
                                        while(current_node && axiom_node_get_node_type(current_node, env) != AXIOM_ELEMENT)
                                        {
                                            current_node = axiom_node_get_next_sibling(current_node, env);
                                        }
                                        if(current_node != NULL)
                                        {
                                            current_element = (axiom_element_t *)axiom_node_get_data_element(current_node, env);
                                            qname = axiom_element_get_qname(current_element, env, current_node);
                                        }
                                       
                                   }
                                   is_early_node_valid = AXIS2_FALSE;
                                 
                                 element_qname = axutil_qname_create(env, "name", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

                           if ( 
                                (current_node   && current_element && (axutil_qname_equals(element_qname, env, qname))))
                           {
                              if( current_node   && current_element && (axutil_qname_equals(element_qname, env, qname)))
                              {
                                is_early_node_valid = AXIS2_TRUE;
                              }
                              
                                 
                                      text_value = axiom_element_get_text(current_element, env, current_node);
                                      if(text_value != NULL)
                                      {
                                            status = adb_LoginResponse_set_name(_LoginResponse, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for name ");
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
                 

                     
                     /*
                      * building secondName element
                      */
                     
                     
                     
                                    /*
                                     * because elements are ordered this works fine
                                     */
                                  
                                   
                                   if(current_node != NULL && is_early_node_valid)
                                   {
                                       current_node = axiom_node_get_next_sibling(current_node, env);
                                       
                                       
                                        while(current_node && axiom_node_get_node_type(current_node, env) != AXIOM_ELEMENT)
                                        {
                                            current_node = axiom_node_get_next_sibling(current_node, env);
                                        }
                                        if(current_node != NULL)
                                        {
                                            current_element = (axiom_element_t *)axiom_node_get_data_element(current_node, env);
                                            qname = axiom_element_get_qname(current_element, env, current_node);
                                        }
                                       
                                   }
                                   is_early_node_valid = AXIS2_FALSE;
                                 
                                 element_qname = axutil_qname_create(env, "secondName", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

                           if ( 
                                (current_node   && current_element && (axutil_qname_equals(element_qname, env, qname))))
                           {
                              if( current_node   && current_element && (axutil_qname_equals(element_qname, env, qname)))
                              {
                                is_early_node_valid = AXIS2_TRUE;
                              }
                              
                                 
                                      text_value = axiom_element_get_text(current_element, env, current_node);
                                      if(text_value != NULL)
                                      {
                                            status = adb_LoginResponse_set_secondName(_LoginResponse, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for secondName ");
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
                 

                     
                     /*
                      * building status element
                      */
                     
                     
                     
                                    /*
                                     * because elements are ordered this works fine
                                     */
                                  
                                   
                                   if(current_node != NULL && is_early_node_valid)
                                   {
                                       current_node = axiom_node_get_next_sibling(current_node, env);
                                       
                                       
                                        while(current_node && axiom_node_get_node_type(current_node, env) != AXIOM_ELEMENT)
                                        {
                                            current_node = axiom_node_get_next_sibling(current_node, env);
                                        }
                                        if(current_node != NULL)
                                        {
                                            current_element = (axiom_element_t *)axiom_node_get_data_element(current_node, env);
                                            qname = axiom_element_get_qname(current_element, env, current_node);
                                        }
                                       
                                   }
                                   is_early_node_valid = AXIS2_FALSE;
                                 
                                 element_qname = axutil_qname_create(env, "status", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

                           if ( 
                                (current_node   && current_element && (axutil_qname_equals(element_qname, env, qname))))
                           {
                              if( current_node   && current_element && (axutil_qname_equals(element_qname, env, qname)))
                              {
                                is_early_node_valid = AXIS2_TRUE;
                              }
                              
                                 
                                      text_value = axiom_element_get_text(current_element, env, current_node);
                                      if(text_value != NULL)
                                      {
                                            status = adb_LoginResponse_set_status(_LoginResponse, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for status ");
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
          adb_LoginResponse_is_particle()
          {
            
                 return AXIS2_FALSE;
              
          }


          void AXIS2_CALL
          adb_LoginResponse_declare_parent_namespaces(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index)
          {
            
                  /* Here this is an empty function, Nothing to declare */
                 
          }

        
        
        axiom_node_t* AXIS2_CALL
        adb_LoginResponse_serialize(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env, axiom_node_t *parent, axiom_element_t *parent_element, int parent_tag_closed, axutil_hash_t *namespaces, int *next_ns_index)
        {
            
            
            if (_LoginResponse == NULL)
            {
                return adb_LoginResponse_serialize_obj(
                    _LoginResponse, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index);
            }
            else
            {
                return axis2_extension_mapper_serialize(
                    (adb_type_t*) _LoginResponse, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index, "adb_LoginResponse");
            }
            
        }

        axiom_node_t* AXIS2_CALL
        adb_LoginResponse_serialize_obj(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env, axiom_node_t *parent, axiom_element_t *parent_element, int parent_tag_closed, axutil_hash_t *namespaces, int *next_ns_index)
        {
            
            
             axis2_char_t *string_to_stream;
            
         
         axiom_node_t* current_node = NULL;
         int tag_closed = 0;
         
         axis2_char_t* xsi_prefix = NULL;
         
         axis2_char_t* type_attrib = NULL;
         axiom_namespace_t* xsi_ns = NULL;
         axiom_attribute_t* xsi_type_attri = NULL;
         
                axiom_namespace_t *ns1 = NULL;

                axis2_char_t *qname_uri = NULL;
                axis2_char_t *qname_prefix = NULL;
                axis2_char_t *p_prefix = NULL;
                axis2_bool_t ns_already_defined;
            
               int i = 0;
               int count = 0;
               void *element = NULL;
             
                    axis2_char_t text_value_1[ADB_DEFAULT_DIGIT_LIMIT];
                    
                    axis2_char_t text_value_2[ADB_DEFAULT_DIGIT_LIMIT];
                    
                    axis2_char_t *text_value_3;
                    axis2_char_t *text_value_3_temp;
                    
                    axis2_char_t *text_value_4;
                    axis2_char_t *text_value_4_temp;
                    
                    axis2_char_t *text_value_5;
                    axis2_char_t *text_value_5_temp;
                    
                    axis2_char_t *text_value_6;
                    axis2_char_t *text_value_6_temp;
                    
                    axis2_char_t *text_value_7;
                    axis2_char_t *text_value_7_temp;
                    
                    axis2_char_t *text_value_8;
                    axis2_char_t *text_value_8_temp;
                    
               axis2_char_t *start_input_str = NULL;
               axis2_char_t *end_input_str = NULL;
               unsigned int start_input_str_len = 0;
               unsigned int end_input_str_len = 0;
            
            
               axiom_data_source_t *data_source = NULL;
               axutil_stream_t *stream = NULL;

            

            AXIS2_ENV_CHECK(env, NULL);
            AXIS2_PARAM_CHECK(env->error, _LoginResponse, NULL);
            
            
                    current_node = parent;
                    data_source = (axiom_data_source_t *)axiom_node_get_data_element(current_node, env);
                    if (!data_source)
                        return NULL;
                    stream = axiom_data_source_get_stream(data_source, env); /* assume parent is of type data source */
                    if (!stream)
                        return NULL;
                  
            if(!parent_tag_closed)
            {
            
              
 
              if(!(xsi_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://www.w3.org/2001/XMLSchema-instance", AXIS2_HASH_KEY_STRING)))
              {
                  /* it is better to stick with the standard prefix */
                  xsi_prefix = (axis2_char_t*)axutil_strdup(env, "xsi");
                  
                  axutil_hash_set(namespaces, "http://www.w3.org/2001/XMLSchema-instance", AXIS2_HASH_KEY_STRING, xsi_prefix);

                  if(parent_element)
                  {
                        axiom_namespace_t *element_ns = NULL;
                        element_ns = axiom_namespace_create(env, "http://www.w3.org/2001/XMLSchema-instance",
                                                            xsi_prefix);
                        axiom_element_declare_namespace_assume_param_ownership(parent_element, env, element_ns);
                  }
              }
              type_attrib = axutil_strcat(env, " ", xsi_prefix, ":type=\"LoginResponse\"", NULL);
              axutil_stream_write(stream, env, type_attrib, axutil_strlen(type_attrib));

              AXIS2_FREE(env->allocator, type_attrib);
                
              string_to_stream = ">"; 
              axutil_stream_write(stream, env, string_to_stream, axutil_strlen(string_to_stream));
              tag_closed = 1;
            
            }
            else {
              /* if the parent tag closed we would be able to declare the type directly on the parent element */ 
              if(!(xsi_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://www.w3.org/2001/XMLSchema-instance", AXIS2_HASH_KEY_STRING)))
              {
                  /* it is better to stick with the standard prefix */
                  xsi_prefix = (axis2_char_t*)axutil_strdup(env, "xsi");
                  
                  axutil_hash_set(namespaces, "http://www.w3.org/2001/XMLSchema-instance", AXIS2_HASH_KEY_STRING, xsi_prefix);

                  if(parent_element)
                  {
                        axiom_namespace_t *element_ns = NULL;
                        element_ns = axiom_namespace_create(env, "http://www.w3.org/2001/XMLSchema-instance",
                                                            xsi_prefix);
                        axiom_element_declare_namespace_assume_param_ownership(parent_element, env, element_ns);
                  }
              }
            }
            xsi_ns = axiom_namespace_create (env,
                                 "http://ffpServer.aeronavigator.ru/xsd",
                                 xsi_prefix);
            xsi_type_attri = axiom_attribute_create (env, "type", "LoginResponse", xsi_ns);
            
            axiom_element_add_attribute (parent_element, env, xsi_type_attri, parent);
        
                       if(!(p_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING)))
                       {
                           p_prefix = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof (axis2_char_t) * ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT);
                           sprintf(p_prefix, "n%d", (*next_ns_index)++);
                           axutil_hash_set(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING, p_prefix);
                           
                           axiom_element_declare_namespace_assume_param_ownership(parent_element, env, axiom_namespace_create (env,
                                            "http://ffpServer.aeronavigator.ru/xsd",
                                            p_prefix));
                       }
                      

                   if (!_LoginResponse->is_valid_errors)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("errors"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("errors")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     /*
                      * Parsing errors array
                      */
                     if (_LoginResponse->property_errors != NULL)
                     {
                        

                            sprintf(start_input_str, "<%s%serrors",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                         start_input_str_len = axutil_strlen(start_input_str);

                         sprintf(end_input_str, "</%s%serrors>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                         end_input_str_len = axutil_strlen(end_input_str);

                         count = axutil_array_list_size(_LoginResponse->property_errors, env);
                         for(i = 0; i < count; i ++)
                         {
                            element = axutil_array_list_get(_LoginResponse->property_errors, env, i);

                            if(NULL == element) 
                            {
                                continue;
                            }
                    
                     
                     /*
                      * parsing errors element
                      */

                    
                     
                            if(!adb_ErrorGroup_is_particle())
                            {
                                axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                            }
                            adb_ErrorGroup_serialize((adb_ErrorGroup_t*)element, 
                                                                                 env, current_node, parent_element,
                                                                                 adb_ErrorGroup_is_particle() || AXIS2_FALSE, namespaces, next_ns_index);
                            
                            if(!adb_ErrorGroup_is_particle())
                            {
                                axutil_stream_write(stream, env, end_input_str, end_input_str_len);
                            }
                            
                         }
                     }
                   
                     
                     AXIS2_FREE(env->allocator,start_input_str);
                     AXIS2_FREE(env->allocator,end_input_str);
                 } 

                 
                       if(!(p_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING)))
                       {
                           p_prefix = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof (axis2_char_t) * ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT);
                           sprintf(p_prefix, "n%d", (*next_ns_index)++);
                           axutil_hash_set(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING, p_prefix);
                           
                           axiom_element_declare_namespace_assume_param_ownership(parent_element, env, axiom_namespace_create (env,
                                            "http://ffpServer.aeronavigator.ru/xsd",
                                            p_prefix));
                       }
                      

                   if (!_LoginResponse->is_valid_access)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("access"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("access")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing access element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%saccess>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%saccess>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           strcpy(text_value_2, (_LoginResponse->property_access)?"true":"false");
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                           axutil_stream_write(stream, env, text_value_2, axutil_strlen(text_value_2));
                           
                           axutil_stream_write(stream, env, end_input_str, end_input_str_len);
                           
                     
                     AXIS2_FREE(env->allocator,start_input_str);
                     AXIS2_FREE(env->allocator,end_input_str);
                 } 

                 
                       if(!(p_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING)))
                       {
                           p_prefix = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof (axis2_char_t) * ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT);
                           sprintf(p_prefix, "n%d", (*next_ns_index)++);
                           axutil_hash_set(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING, p_prefix);
                           
                           axiom_element_declare_namespace_assume_param_ownership(parent_element, env, axiom_namespace_create (env,
                                            "http://ffpServer.aeronavigator.ru/xsd",
                                            p_prefix));
                       }
                      

                   if (!_LoginResponse->is_valid_dob)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("dob"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("dob")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing dob element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sdob>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sdob>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                          text_value_3 = axutil_date_time_serialize_date_time(_LoginResponse->property_dob, env);
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                           axutil_stream_write(stream, env, text_value_3, axutil_strlen(text_value_3));
                           
                           axutil_stream_write(stream, env, end_input_str, end_input_str_len);
                           
                     
                     AXIS2_FREE(env->allocator,start_input_str);
                     AXIS2_FREE(env->allocator,end_input_str);
                 } 

                 
                       if(!(p_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING)))
                       {
                           p_prefix = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof (axis2_char_t) * ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT);
                           sprintf(p_prefix, "n%d", (*next_ns_index)++);
                           axutil_hash_set(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING, p_prefix);
                           
                           axiom_element_declare_namespace_assume_param_ownership(parent_element, env, axiom_namespace_create (env,
                                            "http://ffpServer.aeronavigator.ru/xsd",
                                            p_prefix));
                       }
                      

                   if (!_LoginResponse->is_valid_email)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("email"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("email")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing email element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%semail>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%semail>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_4 = _LoginResponse->property_email;
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                            
                           text_value_4_temp = axutil_xml_quote_string(env, text_value_4, AXIS2_TRUE);
                           if (text_value_4_temp)
                           {
                               axutil_stream_write(stream, env, text_value_4_temp, axutil_strlen(text_value_4_temp));
                               AXIS2_FREE(env->allocator, text_value_4_temp);
                           }
                           else
                           {
                               axutil_stream_write(stream, env, text_value_4, axutil_strlen(text_value_4));
                           }
                           
                           axutil_stream_write(stream, env, end_input_str, end_input_str_len);
                           
                     
                     AXIS2_FREE(env->allocator,start_input_str);
                     AXIS2_FREE(env->allocator,end_input_str);
                 } 

                 
                       if(!(p_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING)))
                       {
                           p_prefix = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof (axis2_char_t) * ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT);
                           sprintf(p_prefix, "n%d", (*next_ns_index)++);
                           axutil_hash_set(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING, p_prefix);
                           
                           axiom_element_declare_namespace_assume_param_ownership(parent_element, env, axiom_namespace_create (env,
                                            "http://ffpServer.aeronavigator.ru/xsd",
                                            p_prefix));
                       }
                      

                   if (!_LoginResponse->is_valid_lastName)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("lastName"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("lastName")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing lastName element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%slastName>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%slastName>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_5 = _LoginResponse->property_lastName;
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                            
                           text_value_5_temp = axutil_xml_quote_string(env, text_value_5, AXIS2_TRUE);
                           if (text_value_5_temp)
                           {
                               axutil_stream_write(stream, env, text_value_5_temp, axutil_strlen(text_value_5_temp));
                               AXIS2_FREE(env->allocator, text_value_5_temp);
                           }
                           else
                           {
                               axutil_stream_write(stream, env, text_value_5, axutil_strlen(text_value_5));
                           }
                           
                           axutil_stream_write(stream, env, end_input_str, end_input_str_len);
                           
                     
                     AXIS2_FREE(env->allocator,start_input_str);
                     AXIS2_FREE(env->allocator,end_input_str);
                 } 

                 
                       if(!(p_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING)))
                       {
                           p_prefix = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof (axis2_char_t) * ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT);
                           sprintf(p_prefix, "n%d", (*next_ns_index)++);
                           axutil_hash_set(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING, p_prefix);
                           
                           axiom_element_declare_namespace_assume_param_ownership(parent_element, env, axiom_namespace_create (env,
                                            "http://ffpServer.aeronavigator.ru/xsd",
                                            p_prefix));
                       }
                      

                   if (!_LoginResponse->is_valid_name)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("name"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("name")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing name element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sname>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sname>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_6 = _LoginResponse->property_name;
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                            
                           text_value_6_temp = axutil_xml_quote_string(env, text_value_6, AXIS2_TRUE);
                           if (text_value_6_temp)
                           {
                               axutil_stream_write(stream, env, text_value_6_temp, axutil_strlen(text_value_6_temp));
                               AXIS2_FREE(env->allocator, text_value_6_temp);
                           }
                           else
                           {
                               axutil_stream_write(stream, env, text_value_6, axutil_strlen(text_value_6));
                           }
                           
                           axutil_stream_write(stream, env, end_input_str, end_input_str_len);
                           
                     
                     AXIS2_FREE(env->allocator,start_input_str);
                     AXIS2_FREE(env->allocator,end_input_str);
                 } 

                 
                       if(!(p_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING)))
                       {
                           p_prefix = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof (axis2_char_t) * ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT);
                           sprintf(p_prefix, "n%d", (*next_ns_index)++);
                           axutil_hash_set(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING, p_prefix);
                           
                           axiom_element_declare_namespace_assume_param_ownership(parent_element, env, axiom_namespace_create (env,
                                            "http://ffpServer.aeronavigator.ru/xsd",
                                            p_prefix));
                       }
                      

                   if (!_LoginResponse->is_valid_secondName)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("secondName"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("secondName")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing secondName element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%ssecondName>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%ssecondName>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_7 = _LoginResponse->property_secondName;
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                            
                           text_value_7_temp = axutil_xml_quote_string(env, text_value_7, AXIS2_TRUE);
                           if (text_value_7_temp)
                           {
                               axutil_stream_write(stream, env, text_value_7_temp, axutil_strlen(text_value_7_temp));
                               AXIS2_FREE(env->allocator, text_value_7_temp);
                           }
                           else
                           {
                               axutil_stream_write(stream, env, text_value_7, axutil_strlen(text_value_7));
                           }
                           
                           axutil_stream_write(stream, env, end_input_str, end_input_str_len);
                           
                     
                     AXIS2_FREE(env->allocator,start_input_str);
                     AXIS2_FREE(env->allocator,end_input_str);
                 } 

                 
                       if(!(p_prefix = (axis2_char_t*)axutil_hash_get(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING)))
                       {
                           p_prefix = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof (axis2_char_t) * ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT);
                           sprintf(p_prefix, "n%d", (*next_ns_index)++);
                           axutil_hash_set(namespaces, "http://ffpServer.aeronavigator.ru/xsd", AXIS2_HASH_KEY_STRING, p_prefix);
                           
                           axiom_element_declare_namespace_assume_param_ownership(parent_element, env, axiom_namespace_create (env,
                                            "http://ffpServer.aeronavigator.ru/xsd",
                                            p_prefix));
                       }
                      

                   if (!_LoginResponse->is_valid_status)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("status"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("status")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing status element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sstatus>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sstatus>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_8 = _LoginResponse->property_status;
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                            
                           text_value_8_temp = axutil_xml_quote_string(env, text_value_8, AXIS2_TRUE);
                           if (text_value_8_temp)
                           {
                               axutil_stream_write(stream, env, text_value_8_temp, axutil_strlen(text_value_8_temp));
                               AXIS2_FREE(env->allocator, text_value_8_temp);
                           }
                           else
                           {
                               axutil_stream_write(stream, env, text_value_8, axutil_strlen(text_value_8));
                           }
                           
                           axutil_stream_write(stream, env, end_input_str, end_input_str_len);
                           
                     
                     AXIS2_FREE(env->allocator,start_input_str);
                     AXIS2_FREE(env->allocator,end_input_str);
                 } 

                 

            return parent;
        }


        

            /**
             * Getter for errors by  Property Number 1
             */
            axutil_array_list_t* AXIS2_CALL
            adb_LoginResponse_get_property1(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env)
            {
                return adb_LoginResponse_get_errors(_LoginResponse,
                                             env);
            }

            /**
             * getter for errors.
             */
            axutil_array_list_t* AXIS2_CALL
            adb_LoginResponse_get_errors(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _LoginResponse, NULL);
                  

                return _LoginResponse->property_errors;
             }

            /**
             * setter for errors
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_set_errors(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env,
                    axutil_array_list_t*  arg_errors)
             {
                
                 int size = 0;
                 int i = 0;
                 axis2_bool_t non_nil_exists = AXIS2_FALSE;
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
                
                if(_LoginResponse->is_valid_errors &&
                        arg_errors == _LoginResponse->property_errors)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                
                 size = axutil_array_list_size(arg_errors, env);
                 
                 if (size < 0)
                 {
                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "errors has less than minOccurs(0)");
                     return AXIS2_FAILURE;
                 }
                 for(i = 0; i < size; i ++ )
                 {
                     if(NULL != axutil_array_list_get(arg_errors, env, i))
                     {
                         non_nil_exists = AXIS2_TRUE;
                         break;
                     }
                 }

                 adb_LoginResponse_reset_errors(_LoginResponse, env);

                
                if(NULL == arg_errors)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _LoginResponse->property_errors = arg_errors;
                        if(non_nil_exists)
                        {
                            _LoginResponse->is_valid_errors = AXIS2_TRUE;
                        }
                        
                    
                return AXIS2_SUCCESS;
             }

            
            /**
             * Get ith element of errors.
             */
            adb_ErrorGroup_t* AXIS2_CALL
            adb_LoginResponse_get_errors_at(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env, int i)
            {
                adb_ErrorGroup_t* ret_val;

                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _LoginResponse, NULL);
                  

                if(_LoginResponse->property_errors == NULL)
                {
                    return (adb_ErrorGroup_t*)0;
                }
                ret_val = (adb_ErrorGroup_t*)axutil_array_list_get(_LoginResponse->property_errors, env, i);
                
                    return ret_val;
                  
            }

            /**
             * Set the ith element of errors.
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_set_errors_at(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env, int i,
                    adb_ErrorGroup_t* arg_errors)
            {
                void *element = NULL;
                int size = 0;
                int j;
                int non_nil_count;
                axis2_bool_t non_nil_exists = AXIS2_FALSE;

                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
                
                if( _LoginResponse->is_valid_errors &&
                    _LoginResponse->property_errors &&
                
                    arg_errors == (adb_ErrorGroup_t*)axutil_array_list_get(_LoginResponse->property_errors, env, i))
                  
                {
                    
                    return AXIS2_SUCCESS; 
                }

                
                    if(NULL != arg_errors)
                    {
                        non_nil_exists = AXIS2_TRUE;
                    }
                    else {
                        if(_LoginResponse->property_errors != NULL)
                        {
                            size = axutil_array_list_size(_LoginResponse->property_errors, env);
                            for(j = 0, non_nil_count = 0; j < size; j ++ )
                            {
                                if(i == j) continue; 
                                if(NULL != axutil_array_list_get(_LoginResponse->property_errors, env, i))
                                {
                                    non_nil_count ++;
                                    non_nil_exists = AXIS2_TRUE;
                                    if(non_nil_count >= 0)
                                    {
                                        break;
                                    }
                                }
                            }

                        
                        }
                    }
                  

                if(_LoginResponse->property_errors == NULL)
                {
                    _LoginResponse->property_errors = axutil_array_list_create(env, 10);
                }
                
                /* check whether there already exist an element */
                element = axutil_array_list_get(_LoginResponse->property_errors, env, i);
                if(NULL != element)
                {
                  
                  
                  adb_ErrorGroup_free((adb_ErrorGroup_t*)element, env);
                     
                }

                
                    if(!non_nil_exists)
                    {
                        
                        _LoginResponse->is_valid_errors = AXIS2_FALSE;
                        axutil_array_list_set(_LoginResponse->property_errors , env, i, NULL);
                        
                        return AXIS2_SUCCESS;
                    }
                
                   axutil_array_list_set(_LoginResponse->property_errors , env, i, arg_errors);
                  _LoginResponse->is_valid_errors = AXIS2_TRUE;
                
                return AXIS2_SUCCESS;
            }

            /**
             * Add to errors.
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_add_errors(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env,
                    adb_ErrorGroup_t* arg_errors)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);

                
                    if(NULL == arg_errors)
                    {
                      
                           return AXIS2_SUCCESS; 
                        
                    }
                  

                if(_LoginResponse->property_errors == NULL)
                {
                    _LoginResponse->property_errors = axutil_array_list_create(env, 10);
                }
                if(_LoginResponse->property_errors == NULL)
                {
                    AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Failed in allocatting memory for errors");
                    return AXIS2_FAILURE;
                    
                }
                
                   axutil_array_list_add(_LoginResponse->property_errors , env, arg_errors);
                  _LoginResponse->is_valid_errors = AXIS2_TRUE;
                return AXIS2_SUCCESS;
             }

            /**
             * Get the size of the errors array.
             */
            int AXIS2_CALL
            adb_LoginResponse_sizeof_errors(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env)
            {
                AXIS2_ENV_CHECK(env, -1);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, -1);
                if(_LoginResponse->property_errors == NULL)
                {
                    return 0;
                }
                return axutil_array_list_size(_LoginResponse->property_errors, env);
            }

            /**
             * remove the ith element, same as set_nil_at.
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_remove_errors_at(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env, int i)
            {
                return adb_LoginResponse_set_errors_nil_at(_LoginResponse, env, i);
            }

            

           /**
            * resetter for errors
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_reset_errors(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
               

               
                  if (_LoginResponse->property_errors != NULL)
                  {
                      count = axutil_array_list_size(_LoginResponse->property_errors, env);
                      for(i = 0; i < count; i ++)
                      {
                         element = axutil_array_list_get(_LoginResponse->property_errors, env, i);
                
            
                
                if(element != NULL)
                {
                   
                   adb_ErrorGroup_free((adb_ErrorGroup_t*)element, env);
                     element = NULL;
                }
            
                
                
                
                      }
                      axutil_array_list_free(_LoginResponse->property_errors, env);
                  }
                _LoginResponse->is_valid_errors = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether errors is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_LoginResponse_is_errors_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_TRUE);
               
               return !_LoginResponse->is_valid_errors;
           }

           /**
            * Set errors to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_set_errors_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               return adb_LoginResponse_reset_errors(_LoginResponse, env);
           }

           
           /**
            * Check whether errors is nill at i
            */
           axis2_bool_t AXIS2_CALL
           adb_LoginResponse_is_errors_nil_at(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env, int i)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_TRUE);
               
               return (_LoginResponse->is_valid_errors == AXIS2_FALSE ||
                        NULL == _LoginResponse->property_errors || 
                        NULL == axutil_array_list_get(_LoginResponse->property_errors, env, i));
           }

           /**
            * Set errors to nill at i
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_set_errors_nil_at(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env, int i)
           {
                void *element = NULL;
                int size = 0;
                int j;
                axis2_bool_t non_nil_exists = AXIS2_FALSE;

                int k = 0;

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);

                if(_LoginResponse->property_errors == NULL ||
                            _LoginResponse->is_valid_errors == AXIS2_FALSE)
                {
                    
                    non_nil_exists = AXIS2_FALSE;
                }
                else
                {
                    size = axutil_array_list_size(_LoginResponse->property_errors, env);
                    for(j = 0, k = 0; j < size; j ++ )
                    {
                        if(i == j) continue; 
                        if(NULL != axutil_array_list_get(_LoginResponse->property_errors, env, i))
                        {
                            k ++;
                            non_nil_exists = AXIS2_TRUE;
                            if( k >= 0)
                            {
                                break;
                            }
                        }
                    }
                }
                

                if( k < 0)
                {
                       AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Size of the array of errors is beinng set to be smaller than the specificed number of minOccurs(0)");
                       return AXIS2_FAILURE;
                }
 
                if(_LoginResponse->property_errors == NULL)
                {
                    _LoginResponse->is_valid_errors = AXIS2_FALSE;
                    
                    return AXIS2_SUCCESS;
                }

                /* check whether there already exist an element */
                element = axutil_array_list_get(_LoginResponse->property_errors, env, i);
                if(NULL != element)
                {
                  
                  
                  adb_ErrorGroup_free((adb_ErrorGroup_t*)element, env);
                     
                }

                
                    if(!non_nil_exists)
                    {
                        
                        _LoginResponse->is_valid_errors = AXIS2_FALSE;
                        axutil_array_list_set(_LoginResponse->property_errors , env, i, NULL);
                        return AXIS2_SUCCESS;
                    }
                

                
                axutil_array_list_set(_LoginResponse->property_errors , env, i, NULL);
                
                return AXIS2_SUCCESS;

           }

           

            /**
             * Getter for access by  Property Number 2
             */
            axis2_bool_t AXIS2_CALL
            adb_LoginResponse_get_property2(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env)
            {
                return adb_LoginResponse_get_access(_LoginResponse,
                                             env);
            }

            /**
             * getter for access.
             */
            axis2_bool_t AXIS2_CALL
            adb_LoginResponse_get_access(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (axis2_bool_t)0);
                    AXIS2_PARAM_CHECK(env->error, _LoginResponse, (axis2_bool_t)0);
                  

                return _LoginResponse->property_access;
             }

            /**
             * setter for access
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_set_access(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env,
                    axis2_bool_t  arg_access)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
                
                if(_LoginResponse->is_valid_access &&
                        arg_access == _LoginResponse->property_access)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_LoginResponse_reset_access(_LoginResponse, env);

                _LoginResponse->property_access = arg_access;
                        _LoginResponse->is_valid_access = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for access
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_reset_access(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
               

               _LoginResponse->is_valid_access = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether access is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_LoginResponse_is_access_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_TRUE);
               
               return !_LoginResponse->is_valid_access;
           }

           /**
            * Set access to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_set_access_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               return adb_LoginResponse_reset_access(_LoginResponse, env);
           }

           

            /**
             * Getter for dob by  Property Number 3
             */
            axutil_date_time_t* AXIS2_CALL
            adb_LoginResponse_get_property3(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env)
            {
                return adb_LoginResponse_get_dob(_LoginResponse,
                                             env);
            }

            /**
             * getter for dob.
             */
            axutil_date_time_t* AXIS2_CALL
            adb_LoginResponse_get_dob(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _LoginResponse, NULL);
                  

                return _LoginResponse->property_dob;
             }

            /**
             * setter for dob
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_set_dob(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env,
                    axutil_date_time_t*  arg_dob)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
                
                if(_LoginResponse->is_valid_dob &&
                        arg_dob == _LoginResponse->property_dob)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_LoginResponse_reset_dob(_LoginResponse, env);

                
                if(NULL == arg_dob)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _LoginResponse->property_dob = arg_dob;
                        _LoginResponse->is_valid_dob = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for dob
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_reset_dob(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
               

               
            
                
                if(_LoginResponse->property_dob != NULL)
                {
                   
                   
                      axutil_date_time_free(_LoginResponse->property_dob, env);
                     _LoginResponse->property_dob = NULL;
                }
            
                
                
                _LoginResponse->is_valid_dob = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether dob is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_LoginResponse_is_dob_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_TRUE);
               
               return !_LoginResponse->is_valid_dob;
           }

           /**
            * Set dob to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_set_dob_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               return adb_LoginResponse_reset_dob(_LoginResponse, env);
           }

           

            /**
             * Getter for email by  Property Number 4
             */
            axis2_char_t* AXIS2_CALL
            adb_LoginResponse_get_property4(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env)
            {
                return adb_LoginResponse_get_email(_LoginResponse,
                                             env);
            }

            /**
             * getter for email.
             */
            axis2_char_t* AXIS2_CALL
            adb_LoginResponse_get_email(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _LoginResponse, NULL);
                  

                return _LoginResponse->property_email;
             }

            /**
             * setter for email
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_set_email(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_email)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
                
                if(_LoginResponse->is_valid_email &&
                        arg_email == _LoginResponse->property_email)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_LoginResponse_reset_email(_LoginResponse, env);

                
                if(NULL == arg_email)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _LoginResponse->property_email = (axis2_char_t *)axutil_strdup(env, arg_email);
                        if(NULL == _LoginResponse->property_email)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for email");
                            return AXIS2_FAILURE;
                        }
                        _LoginResponse->is_valid_email = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for email
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_reset_email(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
               

               
            
                
                if(_LoginResponse->property_email != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _LoginResponse->property_email);
                     _LoginResponse->property_email = NULL;
                }
            
                
                
                _LoginResponse->is_valid_email = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether email is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_LoginResponse_is_email_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_TRUE);
               
               return !_LoginResponse->is_valid_email;
           }

           /**
            * Set email to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_set_email_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               return adb_LoginResponse_reset_email(_LoginResponse, env);
           }

           

            /**
             * Getter for lastName by  Property Number 5
             */
            axis2_char_t* AXIS2_CALL
            adb_LoginResponse_get_property5(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env)
            {
                return adb_LoginResponse_get_lastName(_LoginResponse,
                                             env);
            }

            /**
             * getter for lastName.
             */
            axis2_char_t* AXIS2_CALL
            adb_LoginResponse_get_lastName(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _LoginResponse, NULL);
                  

                return _LoginResponse->property_lastName;
             }

            /**
             * setter for lastName
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_set_lastName(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_lastName)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
                
                if(_LoginResponse->is_valid_lastName &&
                        arg_lastName == _LoginResponse->property_lastName)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_LoginResponse_reset_lastName(_LoginResponse, env);

                
                if(NULL == arg_lastName)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _LoginResponse->property_lastName = (axis2_char_t *)axutil_strdup(env, arg_lastName);
                        if(NULL == _LoginResponse->property_lastName)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for lastName");
                            return AXIS2_FAILURE;
                        }
                        _LoginResponse->is_valid_lastName = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for lastName
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_reset_lastName(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
               

               
            
                
                if(_LoginResponse->property_lastName != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _LoginResponse->property_lastName);
                     _LoginResponse->property_lastName = NULL;
                }
            
                
                
                _LoginResponse->is_valid_lastName = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether lastName is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_LoginResponse_is_lastName_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_TRUE);
               
               return !_LoginResponse->is_valid_lastName;
           }

           /**
            * Set lastName to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_set_lastName_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               return adb_LoginResponse_reset_lastName(_LoginResponse, env);
           }

           

            /**
             * Getter for name by  Property Number 6
             */
            axis2_char_t* AXIS2_CALL
            adb_LoginResponse_get_property6(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env)
            {
                return adb_LoginResponse_get_name(_LoginResponse,
                                             env);
            }

            /**
             * getter for name.
             */
            axis2_char_t* AXIS2_CALL
            adb_LoginResponse_get_name(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _LoginResponse, NULL);
                  

                return _LoginResponse->property_name;
             }

            /**
             * setter for name
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_set_name(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_name)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
                
                if(_LoginResponse->is_valid_name &&
                        arg_name == _LoginResponse->property_name)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_LoginResponse_reset_name(_LoginResponse, env);

                
                if(NULL == arg_name)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _LoginResponse->property_name = (axis2_char_t *)axutil_strdup(env, arg_name);
                        if(NULL == _LoginResponse->property_name)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for name");
                            return AXIS2_FAILURE;
                        }
                        _LoginResponse->is_valid_name = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for name
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_reset_name(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
               

               
            
                
                if(_LoginResponse->property_name != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _LoginResponse->property_name);
                     _LoginResponse->property_name = NULL;
                }
            
                
                
                _LoginResponse->is_valid_name = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether name is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_LoginResponse_is_name_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_TRUE);
               
               return !_LoginResponse->is_valid_name;
           }

           /**
            * Set name to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_set_name_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               return adb_LoginResponse_reset_name(_LoginResponse, env);
           }

           

            /**
             * Getter for secondName by  Property Number 7
             */
            axis2_char_t* AXIS2_CALL
            adb_LoginResponse_get_property7(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env)
            {
                return adb_LoginResponse_get_secondName(_LoginResponse,
                                             env);
            }

            /**
             * getter for secondName.
             */
            axis2_char_t* AXIS2_CALL
            adb_LoginResponse_get_secondName(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _LoginResponse, NULL);
                  

                return _LoginResponse->property_secondName;
             }

            /**
             * setter for secondName
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_set_secondName(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_secondName)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
                
                if(_LoginResponse->is_valid_secondName &&
                        arg_secondName == _LoginResponse->property_secondName)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_LoginResponse_reset_secondName(_LoginResponse, env);

                
                if(NULL == arg_secondName)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _LoginResponse->property_secondName = (axis2_char_t *)axutil_strdup(env, arg_secondName);
                        if(NULL == _LoginResponse->property_secondName)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for secondName");
                            return AXIS2_FAILURE;
                        }
                        _LoginResponse->is_valid_secondName = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for secondName
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_reset_secondName(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
               

               
            
                
                if(_LoginResponse->property_secondName != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _LoginResponse->property_secondName);
                     _LoginResponse->property_secondName = NULL;
                }
            
                
                
                _LoginResponse->is_valid_secondName = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether secondName is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_LoginResponse_is_secondName_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_TRUE);
               
               return !_LoginResponse->is_valid_secondName;
           }

           /**
            * Set secondName to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_set_secondName_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               return adb_LoginResponse_reset_secondName(_LoginResponse, env);
           }

           

            /**
             * Getter for status by  Property Number 8
             */
            axis2_char_t* AXIS2_CALL
            adb_LoginResponse_get_property8(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env)
            {
                return adb_LoginResponse_get_status(_LoginResponse,
                                             env);
            }

            /**
             * getter for status.
             */
            axis2_char_t* AXIS2_CALL
            adb_LoginResponse_get_status(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _LoginResponse, NULL);
                  

                return _LoginResponse->property_status;
             }

            /**
             * setter for status
             */
            axis2_status_t AXIS2_CALL
            adb_LoginResponse_set_status(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_status)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
                
                if(_LoginResponse->is_valid_status &&
                        arg_status == _LoginResponse->property_status)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_LoginResponse_reset_status(_LoginResponse, env);

                
                if(NULL == arg_status)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _LoginResponse->property_status = (axis2_char_t *)axutil_strdup(env, arg_status);
                        if(NULL == _LoginResponse->property_status)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for status");
                            return AXIS2_FAILURE;
                        }
                        _LoginResponse->is_valid_status = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for status
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_reset_status(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_FAILURE);
               

               
            
                
                if(_LoginResponse->property_status != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _LoginResponse->property_status);
                     _LoginResponse->property_status = NULL;
                }
            
                
                
                _LoginResponse->is_valid_status = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether status is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_LoginResponse_is_status_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _LoginResponse, AXIS2_TRUE);
               
               return !_LoginResponse->is_valid_status;
           }

           /**
            * Set status to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_LoginResponse_set_status_nil(
                   adb_LoginResponse_t* _LoginResponse,
                   const axutil_env_t *env)
           {
               return adb_LoginResponse_reset_status(_LoginResponse, env);
           }

           

