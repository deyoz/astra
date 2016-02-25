

        /**
         * adb_ReserveMilesResponse.c
         *
         * This file was auto-generated from WSDL
         * by the Apache Axis2/C version: SNAPSHOT  Built on : Mar 10, 2008 (08:35:52 GMT+00:00)
         */

        #include "adb_ReserveMilesResponse.h"
        
                /*
                 * This type was generated from the piece of schema that had
                 * name = ReserveMilesResponse
                 * Namespace URI = http://ffpServer.aeronavigator.ru/xsd
                 * Namespace Prefix = ns1
                 */
           


        struct adb_ReserveMilesResponse
        {
            axis2_char_t *property_Type;

            axutil_array_list_t* property_errors;

                
                axis2_bool_t is_valid_errors;
            axis2_char_t* property_dateValidBeforeUtc;

                
                axis2_bool_t is_valid_dateValidBeforeUtc;
            int property_token;

                
                axis2_bool_t is_valid_token;
            
        };


       /************************* Private Function prototypes ********************************/
        


       /************************* Function Implmentations ********************************/
        adb_ReserveMilesResponse_t* AXIS2_CALL
        adb_ReserveMilesResponse_create(
            const axutil_env_t *env)
        {
            adb_ReserveMilesResponse_t *_ReserveMilesResponse = NULL;
            
            AXIS2_ENV_CHECK(env, NULL);

            _ReserveMilesResponse = (adb_ReserveMilesResponse_t *) AXIS2_MALLOC(env->
                allocator, sizeof(adb_ReserveMilesResponse_t));

            if(NULL == _ReserveMilesResponse)
            {
                AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
                return NULL;
            }

            memset(_ReserveMilesResponse, 0, sizeof(adb_ReserveMilesResponse_t));

            _ReserveMilesResponse->property_Type = axutil_strdup(env, "adb_ReserveMilesResponse");
            _ReserveMilesResponse->property_errors  = NULL;
                  _ReserveMilesResponse->is_valid_errors  = AXIS2_FALSE;
            _ReserveMilesResponse->property_dateValidBeforeUtc  = NULL;
                  _ReserveMilesResponse->is_valid_dateValidBeforeUtc  = AXIS2_FALSE;
            _ReserveMilesResponse->is_valid_token  = AXIS2_FALSE;
            

            return _ReserveMilesResponse;
        }

        adb_ReserveMilesResponse_t* AXIS2_CALL
        adb_ReserveMilesResponse_create_with_values(
            const axutil_env_t *env,
                axutil_array_list_t* _errors,
                axis2_char_t* _dateValidBeforeUtc,
                int _token)
        {
            adb_ReserveMilesResponse_t* adb_obj = NULL;
            axis2_status_t status = AXIS2_SUCCESS;

            adb_obj = adb_ReserveMilesResponse_create(env);

            
              status = adb_ReserveMilesResponse_set_errors(
                                     adb_obj,
                                     env,
                                     _errors);
              if(status == AXIS2_FAILURE) {
                  adb_ReserveMilesResponse_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ReserveMilesResponse_set_dateValidBeforeUtc(
                                     adb_obj,
                                     env,
                                     _dateValidBeforeUtc);
              if(status == AXIS2_FAILURE) {
                  adb_ReserveMilesResponse_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ReserveMilesResponse_set_token(
                                     adb_obj,
                                     env,
                                     _token);
              if(status == AXIS2_FAILURE) {
                  adb_ReserveMilesResponse_free (adb_obj, env);
                  return NULL;
              }
            

            return adb_obj;
        }
      
        axutil_array_list_t* AXIS2_CALL
                adb_ReserveMilesResponse_free_popping_value(
                        adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                        const axutil_env_t *env)
                {
                    axutil_array_list_t* value;

                    
                    
                    value = _ReserveMilesResponse->property_errors;

                    _ReserveMilesResponse->property_errors = (axutil_array_list_t*)NULL;
                    adb_ReserveMilesResponse_free(_ReserveMilesResponse, env);

                    return value;
                }
            

        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_free(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env)
        {
            
            
            return axis2_extension_mapper_free(
                (adb_type_t*) _ReserveMilesResponse,
                env,
                "adb_ReserveMilesResponse");
            
        }

        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_free_obj(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env)
        {
            
                int i = 0;
                int count = 0;
                void *element = NULL;
            

            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);

            if (_ReserveMilesResponse->property_Type != NULL)
            {
              AXIS2_FREE(env->allocator, _ReserveMilesResponse->property_Type);
            }

            adb_ReserveMilesResponse_reset_errors(_ReserveMilesResponse, env);
            adb_ReserveMilesResponse_reset_dateValidBeforeUtc(_ReserveMilesResponse, env);
            adb_ReserveMilesResponse_reset_token(_ReserveMilesResponse, env);
            

            if(_ReserveMilesResponse)
            {
                AXIS2_FREE(env->allocator, _ReserveMilesResponse);
                _ReserveMilesResponse = NULL;
            }

            return AXIS2_SUCCESS;
        }


        

        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_deserialize(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env,
                axiom_node_t **dp_parent,
                axis2_bool_t *dp_is_early_node_valid,
                axis2_bool_t dont_care_minoccurs)
        {
            
            
            return axis2_extension_mapper_deserialize(
                (adb_type_t*) _ReserveMilesResponse,
                env,
                dp_parent,
                dp_is_early_node_valid,
                dont_care_minoccurs,
                "adb_ReserveMilesResponse");
            
        }

        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_deserialize_obj(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
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
            AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);

            
              
              while(parent && axiom_node_get_node_type(parent, env) != AXIOM_ELEMENT)
              {
                  parent = axiom_node_get_next_sibling(parent, env);
              }
              if (NULL == parent)
              {
                /* This should be checked before everything */
                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                            "Failed in building adb object for ReserveMilesResponse : "
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
                                    status = adb_ReserveMilesResponse_set_errors(_ReserveMilesResponse, env,
                                                                   arr_list);
                               }

                             
                  if(element_qname)
                  {
                     axutil_qname_free(element_qname, env);
                     element_qname = NULL;
                  }
                 

                     
                     /*
                      * building dateValidBeforeUtc element
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
                                 
                                 element_qname = axutil_qname_create(env, "dateValidBeforeUtc", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ReserveMilesResponse_set_dateValidBeforeUtc(_ReserveMilesResponse, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for dateValidBeforeUtc ");
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
                      * building token element
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
                                 
                                 element_qname = axutil_qname_create(env, "token", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ReserveMilesResponse_set_token(_ReserveMilesResponse, env,
                                                                   atoi(text_value));
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for token ");
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
          adb_ReserveMilesResponse_is_particle()
          {
            
                 return AXIS2_FALSE;
              
          }


          void AXIS2_CALL
          adb_ReserveMilesResponse_declare_parent_namespaces(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index)
          {
            
                  /* Here this is an empty function, Nothing to declare */
                 
          }

        
        
        axiom_node_t* AXIS2_CALL
        adb_ReserveMilesResponse_serialize(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env, axiom_node_t *parent, axiom_element_t *parent_element, int parent_tag_closed, axutil_hash_t *namespaces, int *next_ns_index)
        {
            
            
            if (_ReserveMilesResponse == NULL)
            {
                return adb_ReserveMilesResponse_serialize_obj(
                    _ReserveMilesResponse, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index);
            }
            else
            {
                return axis2_extension_mapper_serialize(
                    (adb_type_t*) _ReserveMilesResponse, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index, "adb_ReserveMilesResponse");
            }
            
        }

        axiom_node_t* AXIS2_CALL
        adb_ReserveMilesResponse_serialize_obj(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
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
                    
                    axis2_char_t *text_value_2;
                    axis2_char_t *text_value_2_temp;
                    
                    axis2_char_t text_value_3[ADB_DEFAULT_DIGIT_LIMIT];
                    
               axis2_char_t *start_input_str = NULL;
               axis2_char_t *end_input_str = NULL;
               unsigned int start_input_str_len = 0;
               unsigned int end_input_str_len = 0;
            
            
               axiom_data_source_t *data_source = NULL;
               axutil_stream_t *stream = NULL;

            

            AXIS2_ENV_CHECK(env, NULL);
            AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, NULL);
            
            
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
              type_attrib = axutil_strcat(env, " ", xsi_prefix, ":type=\"ReserveMilesResponse\"", NULL);
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
            xsi_type_attri = axiom_attribute_create (env, "type", "ReserveMilesResponse", xsi_ns);
            
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
                      

                   if (!_ReserveMilesResponse->is_valid_errors)
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
                     if (_ReserveMilesResponse->property_errors != NULL)
                     {
                        

                            sprintf(start_input_str, "<%s%serrors",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                         start_input_str_len = axutil_strlen(start_input_str);

                         sprintf(end_input_str, "</%s%serrors>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                         end_input_str_len = axutil_strlen(end_input_str);

                         count = axutil_array_list_size(_ReserveMilesResponse->property_errors, env);
                         for(i = 0; i < count; i ++)
                         {
                            element = axutil_array_list_get(_ReserveMilesResponse->property_errors, env, i);

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
                      

                   if (!_ReserveMilesResponse->is_valid_dateValidBeforeUtc)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("dateValidBeforeUtc"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("dateValidBeforeUtc")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing dateValidBeforeUtc element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sdateValidBeforeUtc>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sdateValidBeforeUtc>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_2 = _ReserveMilesResponse->property_dateValidBeforeUtc;
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                            
                           text_value_2_temp = axutil_xml_quote_string(env, text_value_2, AXIS2_TRUE);
                           if (text_value_2_temp)
                           {
                               axutil_stream_write(stream, env, text_value_2_temp, axutil_strlen(text_value_2_temp));
                               AXIS2_FREE(env->allocator, text_value_2_temp);
                           }
                           else
                           {
                               axutil_stream_write(stream, env, text_value_2, axutil_strlen(text_value_2));
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
                      

                   if (!_ReserveMilesResponse->is_valid_token)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("token"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("token")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing token element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%stoken>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%stoken>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                               sprintf (text_value_3, AXIS2_PRINTF_INT32_FORMAT_SPECIFIER, _ReserveMilesResponse->property_token);
                             
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                           axutil_stream_write(stream, env, text_value_3, axutil_strlen(text_value_3));
                           
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
            adb_ReserveMilesResponse_get_property1(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env)
            {
                return adb_ReserveMilesResponse_get_errors(_ReserveMilesResponse,
                                             env);
            }

            /**
             * getter for errors.
             */
            axutil_array_list_t* AXIS2_CALL
            adb_ReserveMilesResponse_get_errors(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, NULL);
                  

                return _ReserveMilesResponse->property_errors;
             }

            /**
             * setter for errors
             */
            axis2_status_t AXIS2_CALL
            adb_ReserveMilesResponse_set_errors(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env,
                    axutil_array_list_t*  arg_errors)
             {
                
                 int size = 0;
                 int i = 0;
                 axis2_bool_t non_nil_exists = AXIS2_FALSE;
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);
                
                if(_ReserveMilesResponse->is_valid_errors &&
                        arg_errors == _ReserveMilesResponse->property_errors)
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

                 adb_ReserveMilesResponse_reset_errors(_ReserveMilesResponse, env);

                
                if(NULL == arg_errors)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ReserveMilesResponse->property_errors = arg_errors;
                        if(non_nil_exists)
                        {
                            _ReserveMilesResponse->is_valid_errors = AXIS2_TRUE;
                        }
                        
                    
                return AXIS2_SUCCESS;
             }

            
            /**
             * Get ith element of errors.
             */
            adb_ErrorGroup_t* AXIS2_CALL
            adb_ReserveMilesResponse_get_errors_at(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env, int i)
            {
                adb_ErrorGroup_t* ret_val;

                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, NULL);
                  

                if(_ReserveMilesResponse->property_errors == NULL)
                {
                    return (adb_ErrorGroup_t*)0;
                }
                ret_val = (adb_ErrorGroup_t*)axutil_array_list_get(_ReserveMilesResponse->property_errors, env, i);
                
                    return ret_val;
                  
            }

            /**
             * Set the ith element of errors.
             */
            axis2_status_t AXIS2_CALL
            adb_ReserveMilesResponse_set_errors_at(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env, int i,
                    adb_ErrorGroup_t* arg_errors)
            {
                void *element = NULL;
                int size = 0;
                int j;
                int non_nil_count;
                axis2_bool_t non_nil_exists = AXIS2_FALSE;

                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);
                
                if( _ReserveMilesResponse->is_valid_errors &&
                    _ReserveMilesResponse->property_errors &&
                
                    arg_errors == (adb_ErrorGroup_t*)axutil_array_list_get(_ReserveMilesResponse->property_errors, env, i))
                  
                {
                    
                    return AXIS2_SUCCESS; 
                }

                
                    if(NULL != arg_errors)
                    {
                        non_nil_exists = AXIS2_TRUE;
                    }
                    else {
                        if(_ReserveMilesResponse->property_errors != NULL)
                        {
                            size = axutil_array_list_size(_ReserveMilesResponse->property_errors, env);
                            for(j = 0, non_nil_count = 0; j < size; j ++ )
                            {
                                if(i == j) continue; 
                                if(NULL != axutil_array_list_get(_ReserveMilesResponse->property_errors, env, i))
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
                  

                if(_ReserveMilesResponse->property_errors == NULL)
                {
                    _ReserveMilesResponse->property_errors = axutil_array_list_create(env, 10);
                }
                
                /* check whether there already exist an element */
                element = axutil_array_list_get(_ReserveMilesResponse->property_errors, env, i);
                if(NULL != element)
                {
                  
                  
                  adb_ErrorGroup_free((adb_ErrorGroup_t*)element, env);
                     
                }

                
                    if(!non_nil_exists)
                    {
                        
                        _ReserveMilesResponse->is_valid_errors = AXIS2_FALSE;
                        axutil_array_list_set(_ReserveMilesResponse->property_errors , env, i, NULL);
                        
                        return AXIS2_SUCCESS;
                    }
                
                   axutil_array_list_set(_ReserveMilesResponse->property_errors , env, i, arg_errors);
                  _ReserveMilesResponse->is_valid_errors = AXIS2_TRUE;
                
                return AXIS2_SUCCESS;
            }

            /**
             * Add to errors.
             */
            axis2_status_t AXIS2_CALL
            adb_ReserveMilesResponse_add_errors(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env,
                    adb_ErrorGroup_t* arg_errors)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);

                
                    if(NULL == arg_errors)
                    {
                      
                           return AXIS2_SUCCESS; 
                        
                    }
                  

                if(_ReserveMilesResponse->property_errors == NULL)
                {
                    _ReserveMilesResponse->property_errors = axutil_array_list_create(env, 10);
                }
                if(_ReserveMilesResponse->property_errors == NULL)
                {
                    AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Failed in allocatting memory for errors");
                    return AXIS2_FAILURE;
                    
                }
                
                   axutil_array_list_add(_ReserveMilesResponse->property_errors , env, arg_errors);
                  _ReserveMilesResponse->is_valid_errors = AXIS2_TRUE;
                return AXIS2_SUCCESS;
             }

            /**
             * Get the size of the errors array.
             */
            int AXIS2_CALL
            adb_ReserveMilesResponse_sizeof_errors(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env)
            {
                AXIS2_ENV_CHECK(env, -1);
                AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, -1);
                if(_ReserveMilesResponse->property_errors == NULL)
                {
                    return 0;
                }
                return axutil_array_list_size(_ReserveMilesResponse->property_errors, env);
            }

            /**
             * remove the ith element, same as set_nil_at.
             */
            axis2_status_t AXIS2_CALL
            adb_ReserveMilesResponse_remove_errors_at(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env, int i)
            {
                return adb_ReserveMilesResponse_set_errors_nil_at(_ReserveMilesResponse, env, i);
            }

            

           /**
            * resetter for errors
            */
           axis2_status_t AXIS2_CALL
           adb_ReserveMilesResponse_reset_errors(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);
               

               
                  if (_ReserveMilesResponse->property_errors != NULL)
                  {
                      count = axutil_array_list_size(_ReserveMilesResponse->property_errors, env);
                      for(i = 0; i < count; i ++)
                      {
                         element = axutil_array_list_get(_ReserveMilesResponse->property_errors, env, i);
                
            
                
                if(element != NULL)
                {
                   
                   adb_ErrorGroup_free((adb_ErrorGroup_t*)element, env);
                     element = NULL;
                }
            
                
                
                
                      }
                      axutil_array_list_free(_ReserveMilesResponse->property_errors, env);
                  }
                _ReserveMilesResponse->is_valid_errors = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether errors is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ReserveMilesResponse_is_errors_nil(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_TRUE);
               
               return !_ReserveMilesResponse->is_valid_errors;
           }

           /**
            * Set errors to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ReserveMilesResponse_set_errors_nil(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env)
           {
               return adb_ReserveMilesResponse_reset_errors(_ReserveMilesResponse, env);
           }

           
           /**
            * Check whether errors is nill at i
            */
           axis2_bool_t AXIS2_CALL
           adb_ReserveMilesResponse_is_errors_nil_at(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env, int i)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_TRUE);
               
               return (_ReserveMilesResponse->is_valid_errors == AXIS2_FALSE ||
                        NULL == _ReserveMilesResponse->property_errors || 
                        NULL == axutil_array_list_get(_ReserveMilesResponse->property_errors, env, i));
           }

           /**
            * Set errors to nill at i
            */
           axis2_status_t AXIS2_CALL
           adb_ReserveMilesResponse_set_errors_nil_at(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env, int i)
           {
                void *element = NULL;
                int size = 0;
                int j;
                axis2_bool_t non_nil_exists = AXIS2_FALSE;

                int k = 0;

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);

                if(_ReserveMilesResponse->property_errors == NULL ||
                            _ReserveMilesResponse->is_valid_errors == AXIS2_FALSE)
                {
                    
                    non_nil_exists = AXIS2_FALSE;
                }
                else
                {
                    size = axutil_array_list_size(_ReserveMilesResponse->property_errors, env);
                    for(j = 0, k = 0; j < size; j ++ )
                    {
                        if(i == j) continue; 
                        if(NULL != axutil_array_list_get(_ReserveMilesResponse->property_errors, env, i))
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
 
                if(_ReserveMilesResponse->property_errors == NULL)
                {
                    _ReserveMilesResponse->is_valid_errors = AXIS2_FALSE;
                    
                    return AXIS2_SUCCESS;
                }

                /* check whether there already exist an element */
                element = axutil_array_list_get(_ReserveMilesResponse->property_errors, env, i);
                if(NULL != element)
                {
                  
                  
                  adb_ErrorGroup_free((adb_ErrorGroup_t*)element, env);
                     
                }

                
                    if(!non_nil_exists)
                    {
                        
                        _ReserveMilesResponse->is_valid_errors = AXIS2_FALSE;
                        axutil_array_list_set(_ReserveMilesResponse->property_errors , env, i, NULL);
                        return AXIS2_SUCCESS;
                    }
                

                
                axutil_array_list_set(_ReserveMilesResponse->property_errors , env, i, NULL);
                
                return AXIS2_SUCCESS;

           }

           

            /**
             * Getter for dateValidBeforeUtc by  Property Number 2
             */
            axis2_char_t* AXIS2_CALL
            adb_ReserveMilesResponse_get_property2(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env)
            {
                return adb_ReserveMilesResponse_get_dateValidBeforeUtc(_ReserveMilesResponse,
                                             env);
            }

            /**
             * getter for dateValidBeforeUtc.
             */
            axis2_char_t* AXIS2_CALL
            adb_ReserveMilesResponse_get_dateValidBeforeUtc(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, NULL);
                  

                return _ReserveMilesResponse->property_dateValidBeforeUtc;
             }

            /**
             * setter for dateValidBeforeUtc
             */
            axis2_status_t AXIS2_CALL
            adb_ReserveMilesResponse_set_dateValidBeforeUtc(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_dateValidBeforeUtc)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);
                
                if(_ReserveMilesResponse->is_valid_dateValidBeforeUtc &&
                        arg_dateValidBeforeUtc == _ReserveMilesResponse->property_dateValidBeforeUtc)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ReserveMilesResponse_reset_dateValidBeforeUtc(_ReserveMilesResponse, env);

                
                if(NULL == arg_dateValidBeforeUtc)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ReserveMilesResponse->property_dateValidBeforeUtc = (axis2_char_t *)axutil_strdup(env, arg_dateValidBeforeUtc);
                        if(NULL == _ReserveMilesResponse->property_dateValidBeforeUtc)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for dateValidBeforeUtc");
                            return AXIS2_FAILURE;
                        }
                        _ReserveMilesResponse->is_valid_dateValidBeforeUtc = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for dateValidBeforeUtc
            */
           axis2_status_t AXIS2_CALL
           adb_ReserveMilesResponse_reset_dateValidBeforeUtc(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);
               

               
            
                
                if(_ReserveMilesResponse->property_dateValidBeforeUtc != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _ReserveMilesResponse->property_dateValidBeforeUtc);
                     _ReserveMilesResponse->property_dateValidBeforeUtc = NULL;
                }
            
                
                
                _ReserveMilesResponse->is_valid_dateValidBeforeUtc = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether dateValidBeforeUtc is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ReserveMilesResponse_is_dateValidBeforeUtc_nil(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_TRUE);
               
               return !_ReserveMilesResponse->is_valid_dateValidBeforeUtc;
           }

           /**
            * Set dateValidBeforeUtc to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ReserveMilesResponse_set_dateValidBeforeUtc_nil(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env)
           {
               return adb_ReserveMilesResponse_reset_dateValidBeforeUtc(_ReserveMilesResponse, env);
           }

           

            /**
             * Getter for token by  Property Number 3
             */
            int AXIS2_CALL
            adb_ReserveMilesResponse_get_property3(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env)
            {
                return adb_ReserveMilesResponse_get_token(_ReserveMilesResponse,
                                             env);
            }

            /**
             * getter for token.
             */
            int AXIS2_CALL
            adb_ReserveMilesResponse_get_token(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (int)0);
                    AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, (int)0);
                  

                return _ReserveMilesResponse->property_token;
             }

            /**
             * setter for token
             */
            axis2_status_t AXIS2_CALL
            adb_ReserveMilesResponse_set_token(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env,
                    const int  arg_token)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);
                
                if(_ReserveMilesResponse->is_valid_token &&
                        arg_token == _ReserveMilesResponse->property_token)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ReserveMilesResponse_reset_token(_ReserveMilesResponse, env);

                _ReserveMilesResponse->property_token = arg_token;
                        _ReserveMilesResponse->is_valid_token = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for token
            */
           axis2_status_t AXIS2_CALL
           adb_ReserveMilesResponse_reset_token(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_FAILURE);
               

               _ReserveMilesResponse->is_valid_token = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether token is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ReserveMilesResponse_is_token_nil(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ReserveMilesResponse, AXIS2_TRUE);
               
               return !_ReserveMilesResponse->is_valid_token;
           }

           /**
            * Set token to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ReserveMilesResponse_set_token_nil(
                   adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                   const axutil_env_t *env)
           {
               return adb_ReserveMilesResponse_reset_token(_ReserveMilesResponse, env);
           }

           

