

        /**
         * adb_BonusResponse.c
         *
         * This file was auto-generated from WSDL
         * by the Apache Axis2/C version: SNAPSHOT  Built on : Mar 10, 2008 (08:35:52 GMT+00:00)
         */

        #include "adb_BonusResponse.h"
        
                /*
                 * This type was generated from the piece of schema that had
                 * name = BonusResponse
                 * Namespace URI = http://ffpServer.aeronavigator.ru/xsd
                 * Namespace Prefix = ns1
                 */
           


        struct adb_BonusResponse
        {
            axis2_char_t *property_Type;

            axutil_array_list_t* property_errors;

                
                axis2_bool_t is_valid_errors;
            axutil_array_list_t* property_income;

                
                axis2_bool_t is_valid_income;
            
        };


       /************************* Private Function prototypes ********************************/
        


       /************************* Function Implmentations ********************************/
        adb_BonusResponse_t* AXIS2_CALL
        adb_BonusResponse_create(
            const axutil_env_t *env)
        {
            adb_BonusResponse_t *_BonusResponse = NULL;
            
            AXIS2_ENV_CHECK(env, NULL);

            _BonusResponse = (adb_BonusResponse_t *) AXIS2_MALLOC(env->
                allocator, sizeof(adb_BonusResponse_t));

            if(NULL == _BonusResponse)
            {
                AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
                return NULL;
            }

            memset(_BonusResponse, 0, sizeof(adb_BonusResponse_t));

            _BonusResponse->property_Type = axutil_strdup(env, "adb_BonusResponse");
            _BonusResponse->property_errors  = NULL;
                  _BonusResponse->is_valid_errors  = AXIS2_FALSE;
            _BonusResponse->property_income  = NULL;
                  _BonusResponse->is_valid_income  = AXIS2_FALSE;
            

            return _BonusResponse;
        }

        adb_BonusResponse_t* AXIS2_CALL
        adb_BonusResponse_create_with_values(
            const axutil_env_t *env,
                axutil_array_list_t* _errors,
                axutil_array_list_t* _income)
        {
            adb_BonusResponse_t* adb_obj = NULL;
            axis2_status_t status = AXIS2_SUCCESS;

            adb_obj = adb_BonusResponse_create(env);

            
              status = adb_BonusResponse_set_errors(
                                     adb_obj,
                                     env,
                                     _errors);
              if(status == AXIS2_FAILURE) {
                  adb_BonusResponse_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_BonusResponse_set_income(
                                     adb_obj,
                                     env,
                                     _income);
              if(status == AXIS2_FAILURE) {
                  adb_BonusResponse_free (adb_obj, env);
                  return NULL;
              }
            

            return adb_obj;
        }
      
        axutil_array_list_t* AXIS2_CALL
                adb_BonusResponse_free_popping_value(
                        adb_BonusResponse_t* _BonusResponse,
                        const axutil_env_t *env)
                {
                    axutil_array_list_t* value;

                    
                    
                    value = _BonusResponse->property_errors;

                    _BonusResponse->property_errors = (axutil_array_list_t*)NULL;
                    adb_BonusResponse_free(_BonusResponse, env);

                    return value;
                }
            

        axis2_status_t AXIS2_CALL
        adb_BonusResponse_free(
                adb_BonusResponse_t* _BonusResponse,
                const axutil_env_t *env)
        {
            
            
            return axis2_extension_mapper_free(
                (adb_type_t*) _BonusResponse,
                env,
                "adb_BonusResponse");
            
        }

        axis2_status_t AXIS2_CALL
        adb_BonusResponse_free_obj(
                adb_BonusResponse_t* _BonusResponse,
                const axutil_env_t *env)
        {
            
                int i = 0;
                int count = 0;
                void *element = NULL;
            

            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);

            if (_BonusResponse->property_Type != NULL)
            {
              AXIS2_FREE(env->allocator, _BonusResponse->property_Type);
            }

            adb_BonusResponse_reset_errors(_BonusResponse, env);
            adb_BonusResponse_reset_income(_BonusResponse, env);
            

            if(_BonusResponse)
            {
                AXIS2_FREE(env->allocator, _BonusResponse);
                _BonusResponse = NULL;
            }

            return AXIS2_SUCCESS;
        }


        

        axis2_status_t AXIS2_CALL
        adb_BonusResponse_deserialize(
                adb_BonusResponse_t* _BonusResponse,
                const axutil_env_t *env,
                axiom_node_t **dp_parent,
                axis2_bool_t *dp_is_early_node_valid,
                axis2_bool_t dont_care_minoccurs)
        {
            
            
            return axis2_extension_mapper_deserialize(
                (adb_type_t*) _BonusResponse,
                env,
                dp_parent,
                dp_is_early_node_valid,
                dont_care_minoccurs,
                "adb_BonusResponse");
            
        }

        axis2_status_t AXIS2_CALL
        adb_BonusResponse_deserialize_obj(
                adb_BonusResponse_t* _BonusResponse,
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
            AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);

            
              
              while(parent && axiom_node_get_node_type(parent, env) != AXIOM_ELEMENT)
              {
                  parent = axiom_node_get_next_sibling(parent, env);
              }
              if (NULL == parent)
              {
                /* This should be checked before everything */
                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                            "Failed in building adb object for BonusResponse : "
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
                                    status = adb_BonusResponse_set_errors(_BonusResponse, env,
                                                                   arr_list);
                               }

                             
                  if(element_qname)
                  {
                     axutil_qname_free(element_qname, env);
                     element_qname = NULL;
                  }
                 
                    /*
                     * building income array
                     */
                       arr_list = axutil_array_list_create(env, 10);
                   

                     
                     /*
                      * building income element
                      */
                     
                     
                     
                                    element_qname = axutil_qname_create(env, "income", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                  
                               
                               for (i = 0, sequence_broken = 0, current_node = (is_early_node_valid?axiom_node_get_next_sibling(current_node, env):current_node); !sequence_broken && current_node != NULL;) 
                                             
                               {
                                  if(axiom_node_get_node_type(current_node, env) != AXIOM_ELEMENT)
                                  {
                                     current_node =axiom_node_get_next_sibling(current_node, env);
                                     is_early_node_valid = AXIS2_FALSE;
                                     continue;
                                  }
                                  
                                  current_element = (axiom_element_t *)axiom_node_get_data_element(current_node, env);
                                  qname = axiom_element_get_qname(current_element, env, current_node);

                                  if (adb_ClassMilesIncome_is_particle() ||  
                                    (current_node && current_element && (axutil_qname_equals(element_qname, env, qname))))
                                  {
                                  
                                      if( current_node && current_element && (axutil_qname_equals(element_qname, env, qname)))
                                      {
                                          is_early_node_valid = AXIS2_TRUE;
                                      }
                                      
                                     
                                          element = (void*)axis2_extension_mapper_create_from_node(env, &current_node, "adb_ClassMilesIncome");
                                          
                                          status =  adb_ClassMilesIncome_deserialize((adb_ClassMilesIncome_t*)element, env,
                                                                                 &current_node, &is_early_node_valid, AXIS2_FALSE);
                                          
                                          if(AXIS2_FAILURE ==  status)
                                          {
                                              AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in building element income ");
                                          }
                                          else
                                          {
                                            axutil_array_list_add_at(arr_list, env, i, element);
                                          }
                                        
                                     if(AXIS2_FAILURE ==  status)
                                     {
                                         AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for income ");
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
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "income (@minOccurs = '0') only have %d elements", i);
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
                                    status = adb_BonusResponse_set_income(_BonusResponse, env,
                                                                   arr_list);
                               }

                             
                  if(element_qname)
                  {
                     axutil_qname_free(element_qname, env);
                     element_qname = NULL;
                  }
                 
          return status;
       }

          axis2_bool_t AXIS2_CALL
          adb_BonusResponse_is_particle()
          {
            
                 return AXIS2_FALSE;
              
          }


          void AXIS2_CALL
          adb_BonusResponse_declare_parent_namespaces(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index)
          {
            
                  /* Here this is an empty function, Nothing to declare */
                 
          }

        
        
        axiom_node_t* AXIS2_CALL
        adb_BonusResponse_serialize(
                adb_BonusResponse_t* _BonusResponse,
                const axutil_env_t *env, axiom_node_t *parent, axiom_element_t *parent_element, int parent_tag_closed, axutil_hash_t *namespaces, int *next_ns_index)
        {
            
            
            if (_BonusResponse == NULL)
            {
                return adb_BonusResponse_serialize_obj(
                    _BonusResponse, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index);
            }
            else
            {
                return axis2_extension_mapper_serialize(
                    (adb_type_t*) _BonusResponse, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index, "adb_BonusResponse");
            }
            
        }

        axiom_node_t* AXIS2_CALL
        adb_BonusResponse_serialize_obj(
                adb_BonusResponse_t* _BonusResponse,
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
                    
               axis2_char_t *start_input_str = NULL;
               axis2_char_t *end_input_str = NULL;
               unsigned int start_input_str_len = 0;
               unsigned int end_input_str_len = 0;
            
            
               axiom_data_source_t *data_source = NULL;
               axutil_stream_t *stream = NULL;

            

            AXIS2_ENV_CHECK(env, NULL);
            AXIS2_PARAM_CHECK(env->error, _BonusResponse, NULL);
            
            
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
              type_attrib = axutil_strcat(env, " ", xsi_prefix, ":type=\"BonusResponse\"", NULL);
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
            xsi_type_attri = axiom_attribute_create (env, "type", "BonusResponse", xsi_ns);
            
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
                      

                   if (!_BonusResponse->is_valid_errors)
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
                     if (_BonusResponse->property_errors != NULL)
                     {
                        

                            sprintf(start_input_str, "<%s%serrors",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                         start_input_str_len = axutil_strlen(start_input_str);

                         sprintf(end_input_str, "</%s%serrors>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                         end_input_str_len = axutil_strlen(end_input_str);

                         count = axutil_array_list_size(_BonusResponse->property_errors, env);
                         for(i = 0; i < count; i ++)
                         {
                            element = axutil_array_list_get(_BonusResponse->property_errors, env, i);

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
                      

                   if (!_BonusResponse->is_valid_income)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("income"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("income")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     /*
                      * Parsing income array
                      */
                     if (_BonusResponse->property_income != NULL)
                     {
                        

                            sprintf(start_input_str, "<%s%sincome",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                         start_input_str_len = axutil_strlen(start_input_str);

                         sprintf(end_input_str, "</%s%sincome>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                         end_input_str_len = axutil_strlen(end_input_str);

                         count = axutil_array_list_size(_BonusResponse->property_income, env);
                         for(i = 0; i < count; i ++)
                         {
                            element = axutil_array_list_get(_BonusResponse->property_income, env, i);

                            if(NULL == element) 
                            {
                                continue;
                            }
                    
                     
                     /*
                      * parsing income element
                      */

                    
                     
                            if(!adb_ClassMilesIncome_is_particle())
                            {
                                axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                            }
                            adb_ClassMilesIncome_serialize((adb_ClassMilesIncome_t*)element, 
                                                                                 env, current_node, parent_element,
                                                                                 adb_ClassMilesIncome_is_particle() || AXIS2_FALSE, namespaces, next_ns_index);
                            
                            if(!adb_ClassMilesIncome_is_particle())
                            {
                                axutil_stream_write(stream, env, end_input_str, end_input_str_len);
                            }
                            
                         }
                     }
                   
                     
                     AXIS2_FREE(env->allocator,start_input_str);
                     AXIS2_FREE(env->allocator,end_input_str);
                 } 

                 

            return parent;
        }


        

            /**
             * Getter for errors by  Property Number 1
             */
            axutil_array_list_t* AXIS2_CALL
            adb_BonusResponse_get_property1(
                adb_BonusResponse_t* _BonusResponse,
                const axutil_env_t *env)
            {
                return adb_BonusResponse_get_errors(_BonusResponse,
                                             env);
            }

            /**
             * getter for errors.
             */
            axutil_array_list_t* AXIS2_CALL
            adb_BonusResponse_get_errors(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _BonusResponse, NULL);
                  

                return _BonusResponse->property_errors;
             }

            /**
             * setter for errors
             */
            axis2_status_t AXIS2_CALL
            adb_BonusResponse_set_errors(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env,
                    axutil_array_list_t*  arg_errors)
             {
                
                 int size = 0;
                 int i = 0;
                 axis2_bool_t non_nil_exists = AXIS2_FALSE;
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);
                
                if(_BonusResponse->is_valid_errors &&
                        arg_errors == _BonusResponse->property_errors)
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

                 adb_BonusResponse_reset_errors(_BonusResponse, env);

                
                if(NULL == arg_errors)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _BonusResponse->property_errors = arg_errors;
                        if(non_nil_exists)
                        {
                            _BonusResponse->is_valid_errors = AXIS2_TRUE;
                        }
                        
                    
                return AXIS2_SUCCESS;
             }

            
            /**
             * Get ith element of errors.
             */
            adb_ErrorGroup_t* AXIS2_CALL
            adb_BonusResponse_get_errors_at(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env, int i)
            {
                adb_ErrorGroup_t* ret_val;

                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _BonusResponse, NULL);
                  

                if(_BonusResponse->property_errors == NULL)
                {
                    return (adb_ErrorGroup_t*)0;
                }
                ret_val = (adb_ErrorGroup_t*)axutil_array_list_get(_BonusResponse->property_errors, env, i);
                
                    return ret_val;
                  
            }

            /**
             * Set the ith element of errors.
             */
            axis2_status_t AXIS2_CALL
            adb_BonusResponse_set_errors_at(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env, int i,
                    adb_ErrorGroup_t* arg_errors)
            {
                void *element = NULL;
                int size = 0;
                int j;
                int non_nil_count;
                axis2_bool_t non_nil_exists = AXIS2_FALSE;

                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);
                
                if( _BonusResponse->is_valid_errors &&
                    _BonusResponse->property_errors &&
                
                    arg_errors == (adb_ErrorGroup_t*)axutil_array_list_get(_BonusResponse->property_errors, env, i))
                  
                {
                    
                    return AXIS2_SUCCESS; 
                }

                
                    if(NULL != arg_errors)
                    {
                        non_nil_exists = AXIS2_TRUE;
                    }
                    else {
                        if(_BonusResponse->property_errors != NULL)
                        {
                            size = axutil_array_list_size(_BonusResponse->property_errors, env);
                            for(j = 0, non_nil_count = 0; j < size; j ++ )
                            {
                                if(i == j) continue; 
                                if(NULL != axutil_array_list_get(_BonusResponse->property_errors, env, i))
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
                  

                if(_BonusResponse->property_errors == NULL)
                {
                    _BonusResponse->property_errors = axutil_array_list_create(env, 10);
                }
                
                /* check whether there already exist an element */
                element = axutil_array_list_get(_BonusResponse->property_errors, env, i);
                if(NULL != element)
                {
                  
                  
                  adb_ErrorGroup_free((adb_ErrorGroup_t*)element, env);
                     
                }

                
                    if(!non_nil_exists)
                    {
                        
                        _BonusResponse->is_valid_errors = AXIS2_FALSE;
                        axutil_array_list_set(_BonusResponse->property_errors , env, i, NULL);
                        
                        return AXIS2_SUCCESS;
                    }
                
                   axutil_array_list_set(_BonusResponse->property_errors , env, i, arg_errors);
                  _BonusResponse->is_valid_errors = AXIS2_TRUE;
                
                return AXIS2_SUCCESS;
            }

            /**
             * Add to errors.
             */
            axis2_status_t AXIS2_CALL
            adb_BonusResponse_add_errors(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env,
                    adb_ErrorGroup_t* arg_errors)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);

                
                    if(NULL == arg_errors)
                    {
                      
                           return AXIS2_SUCCESS; 
                        
                    }
                  

                if(_BonusResponse->property_errors == NULL)
                {
                    _BonusResponse->property_errors = axutil_array_list_create(env, 10);
                }
                if(_BonusResponse->property_errors == NULL)
                {
                    AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Failed in allocatting memory for errors");
                    return AXIS2_FAILURE;
                    
                }
                
                   axutil_array_list_add(_BonusResponse->property_errors , env, arg_errors);
                  _BonusResponse->is_valid_errors = AXIS2_TRUE;
                return AXIS2_SUCCESS;
             }

            /**
             * Get the size of the errors array.
             */
            int AXIS2_CALL
            adb_BonusResponse_sizeof_errors(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env)
            {
                AXIS2_ENV_CHECK(env, -1);
                AXIS2_PARAM_CHECK(env->error, _BonusResponse, -1);
                if(_BonusResponse->property_errors == NULL)
                {
                    return 0;
                }
                return axutil_array_list_size(_BonusResponse->property_errors, env);
            }

            /**
             * remove the ith element, same as set_nil_at.
             */
            axis2_status_t AXIS2_CALL
            adb_BonusResponse_remove_errors_at(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env, int i)
            {
                return adb_BonusResponse_set_errors_nil_at(_BonusResponse, env, i);
            }

            

           /**
            * resetter for errors
            */
           axis2_status_t AXIS2_CALL
           adb_BonusResponse_reset_errors(
                   adb_BonusResponse_t* _BonusResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);
               

               
                  if (_BonusResponse->property_errors != NULL)
                  {
                      count = axutil_array_list_size(_BonusResponse->property_errors, env);
                      for(i = 0; i < count; i ++)
                      {
                         element = axutil_array_list_get(_BonusResponse->property_errors, env, i);
                
            
                
                if(element != NULL)
                {
                   
                   adb_ErrorGroup_free((adb_ErrorGroup_t*)element, env);
                     element = NULL;
                }
            
                
                
                
                      }
                      axutil_array_list_free(_BonusResponse->property_errors, env);
                  }
                _BonusResponse->is_valid_errors = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether errors is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_BonusResponse_is_errors_nil(
                   adb_BonusResponse_t* _BonusResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_TRUE);
               
               return !_BonusResponse->is_valid_errors;
           }

           /**
            * Set errors to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_BonusResponse_set_errors_nil(
                   adb_BonusResponse_t* _BonusResponse,
                   const axutil_env_t *env)
           {
               return adb_BonusResponse_reset_errors(_BonusResponse, env);
           }

           
           /**
            * Check whether errors is nill at i
            */
           axis2_bool_t AXIS2_CALL
           adb_BonusResponse_is_errors_nil_at(
                   adb_BonusResponse_t* _BonusResponse,
                   const axutil_env_t *env, int i)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_TRUE);
               
               return (_BonusResponse->is_valid_errors == AXIS2_FALSE ||
                        NULL == _BonusResponse->property_errors || 
                        NULL == axutil_array_list_get(_BonusResponse->property_errors, env, i));
           }

           /**
            * Set errors to nill at i
            */
           axis2_status_t AXIS2_CALL
           adb_BonusResponse_set_errors_nil_at(
                   adb_BonusResponse_t* _BonusResponse,
                   const axutil_env_t *env, int i)
           {
                void *element = NULL;
                int size = 0;
                int j;
                axis2_bool_t non_nil_exists = AXIS2_FALSE;

                int k = 0;

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);

                if(_BonusResponse->property_errors == NULL ||
                            _BonusResponse->is_valid_errors == AXIS2_FALSE)
                {
                    
                    non_nil_exists = AXIS2_FALSE;
                }
                else
                {
                    size = axutil_array_list_size(_BonusResponse->property_errors, env);
                    for(j = 0, k = 0; j < size; j ++ )
                    {
                        if(i == j) continue; 
                        if(NULL != axutil_array_list_get(_BonusResponse->property_errors, env, i))
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
 
                if(_BonusResponse->property_errors == NULL)
                {
                    _BonusResponse->is_valid_errors = AXIS2_FALSE;
                    
                    return AXIS2_SUCCESS;
                }

                /* check whether there already exist an element */
                element = axutil_array_list_get(_BonusResponse->property_errors, env, i);
                if(NULL != element)
                {
                  
                  
                  adb_ErrorGroup_free((adb_ErrorGroup_t*)element, env);
                     
                }

                
                    if(!non_nil_exists)
                    {
                        
                        _BonusResponse->is_valid_errors = AXIS2_FALSE;
                        axutil_array_list_set(_BonusResponse->property_errors , env, i, NULL);
                        return AXIS2_SUCCESS;
                    }
                

                
                axutil_array_list_set(_BonusResponse->property_errors , env, i, NULL);
                
                return AXIS2_SUCCESS;

           }

           

            /**
             * Getter for income by  Property Number 2
             */
            axutil_array_list_t* AXIS2_CALL
            adb_BonusResponse_get_property2(
                adb_BonusResponse_t* _BonusResponse,
                const axutil_env_t *env)
            {
                return adb_BonusResponse_get_income(_BonusResponse,
                                             env);
            }

            /**
             * getter for income.
             */
            axutil_array_list_t* AXIS2_CALL
            adb_BonusResponse_get_income(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _BonusResponse, NULL);
                  

                return _BonusResponse->property_income;
             }

            /**
             * setter for income
             */
            axis2_status_t AXIS2_CALL
            adb_BonusResponse_set_income(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env,
                    axutil_array_list_t*  arg_income)
             {
                
                 int size = 0;
                 int i = 0;
                 axis2_bool_t non_nil_exists = AXIS2_FALSE;
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);
                
                if(_BonusResponse->is_valid_income &&
                        arg_income == _BonusResponse->property_income)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                
                 size = axutil_array_list_size(arg_income, env);
                 
                 if (size < 0)
                 {
                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "income has less than minOccurs(0)");
                     return AXIS2_FAILURE;
                 }
                 for(i = 0; i < size; i ++ )
                 {
                     if(NULL != axutil_array_list_get(arg_income, env, i))
                     {
                         non_nil_exists = AXIS2_TRUE;
                         break;
                     }
                 }

                 adb_BonusResponse_reset_income(_BonusResponse, env);

                
                if(NULL == arg_income)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _BonusResponse->property_income = arg_income;
                        if(non_nil_exists)
                        {
                            _BonusResponse->is_valid_income = AXIS2_TRUE;
                        }
                        
                    
                return AXIS2_SUCCESS;
             }

            
            /**
             * Get ith element of income.
             */
            adb_ClassMilesIncome_t* AXIS2_CALL
            adb_BonusResponse_get_income_at(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env, int i)
            {
                adb_ClassMilesIncome_t* ret_val;

                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _BonusResponse, NULL);
                  

                if(_BonusResponse->property_income == NULL)
                {
                    return (adb_ClassMilesIncome_t*)0;
                }
                ret_val = (adb_ClassMilesIncome_t*)axutil_array_list_get(_BonusResponse->property_income, env, i);
                
                    return ret_val;
                  
            }

            /**
             * Set the ith element of income.
             */
            axis2_status_t AXIS2_CALL
            adb_BonusResponse_set_income_at(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env, int i,
                    adb_ClassMilesIncome_t* arg_income)
            {
                void *element = NULL;
                int size = 0;
                int j;
                int non_nil_count;
                axis2_bool_t non_nil_exists = AXIS2_FALSE;

                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);
                
                if( _BonusResponse->is_valid_income &&
                    _BonusResponse->property_income &&
                
                    arg_income == (adb_ClassMilesIncome_t*)axutil_array_list_get(_BonusResponse->property_income, env, i))
                  
                {
                    
                    return AXIS2_SUCCESS; 
                }

                
                    if(NULL != arg_income)
                    {
                        non_nil_exists = AXIS2_TRUE;
                    }
                    else {
                        if(_BonusResponse->property_income != NULL)
                        {
                            size = axutil_array_list_size(_BonusResponse->property_income, env);
                            for(j = 0, non_nil_count = 0; j < size; j ++ )
                            {
                                if(i == j) continue; 
                                if(NULL != axutil_array_list_get(_BonusResponse->property_income, env, i))
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
                  

                if(_BonusResponse->property_income == NULL)
                {
                    _BonusResponse->property_income = axutil_array_list_create(env, 10);
                }
                
                /* check whether there already exist an element */
                element = axutil_array_list_get(_BonusResponse->property_income, env, i);
                if(NULL != element)
                {
                  
                  
                  adb_ClassMilesIncome_free((adb_ClassMilesIncome_t*)element, env);
                     
                }

                
                    if(!non_nil_exists)
                    {
                        
                        _BonusResponse->is_valid_income = AXIS2_FALSE;
                        axutil_array_list_set(_BonusResponse->property_income , env, i, NULL);
                        
                        return AXIS2_SUCCESS;
                    }
                
                   axutil_array_list_set(_BonusResponse->property_income , env, i, arg_income);
                  _BonusResponse->is_valid_income = AXIS2_TRUE;
                
                return AXIS2_SUCCESS;
            }

            /**
             * Add to income.
             */
            axis2_status_t AXIS2_CALL
            adb_BonusResponse_add_income(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env,
                    adb_ClassMilesIncome_t* arg_income)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);

                
                    if(NULL == arg_income)
                    {
                      
                           return AXIS2_SUCCESS; 
                        
                    }
                  

                if(_BonusResponse->property_income == NULL)
                {
                    _BonusResponse->property_income = axutil_array_list_create(env, 10);
                }
                if(_BonusResponse->property_income == NULL)
                {
                    AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Failed in allocatting memory for income");
                    return AXIS2_FAILURE;
                    
                }
                
                   axutil_array_list_add(_BonusResponse->property_income , env, arg_income);
                  _BonusResponse->is_valid_income = AXIS2_TRUE;
                return AXIS2_SUCCESS;
             }

            /**
             * Get the size of the income array.
             */
            int AXIS2_CALL
            adb_BonusResponse_sizeof_income(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env)
            {
                AXIS2_ENV_CHECK(env, -1);
                AXIS2_PARAM_CHECK(env->error, _BonusResponse, -1);
                if(_BonusResponse->property_income == NULL)
                {
                    return 0;
                }
                return axutil_array_list_size(_BonusResponse->property_income, env);
            }

            /**
             * remove the ith element, same as set_nil_at.
             */
            axis2_status_t AXIS2_CALL
            adb_BonusResponse_remove_income_at(
                    adb_BonusResponse_t* _BonusResponse,
                    const axutil_env_t *env, int i)
            {
                return adb_BonusResponse_set_income_nil_at(_BonusResponse, env, i);
            }

            

           /**
            * resetter for income
            */
           axis2_status_t AXIS2_CALL
           adb_BonusResponse_reset_income(
                   adb_BonusResponse_t* _BonusResponse,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);
               

               
                  if (_BonusResponse->property_income != NULL)
                  {
                      count = axutil_array_list_size(_BonusResponse->property_income, env);
                      for(i = 0; i < count; i ++)
                      {
                         element = axutil_array_list_get(_BonusResponse->property_income, env, i);
                
            
                
                if(element != NULL)
                {
                   
                   adb_ClassMilesIncome_free((adb_ClassMilesIncome_t*)element, env);
                     element = NULL;
                }
            
                
                
                
                      }
                      axutil_array_list_free(_BonusResponse->property_income, env);
                  }
                _BonusResponse->is_valid_income = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether income is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_BonusResponse_is_income_nil(
                   adb_BonusResponse_t* _BonusResponse,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_TRUE);
               
               return !_BonusResponse->is_valid_income;
           }

           /**
            * Set income to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_BonusResponse_set_income_nil(
                   adb_BonusResponse_t* _BonusResponse,
                   const axutil_env_t *env)
           {
               return adb_BonusResponse_reset_income(_BonusResponse, env);
           }

           
           /**
            * Check whether income is nill at i
            */
           axis2_bool_t AXIS2_CALL
           adb_BonusResponse_is_income_nil_at(
                   adb_BonusResponse_t* _BonusResponse,
                   const axutil_env_t *env, int i)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_TRUE);
               
               return (_BonusResponse->is_valid_income == AXIS2_FALSE ||
                        NULL == _BonusResponse->property_income || 
                        NULL == axutil_array_list_get(_BonusResponse->property_income, env, i));
           }

           /**
            * Set income to nill at i
            */
           axis2_status_t AXIS2_CALL
           adb_BonusResponse_set_income_nil_at(
                   adb_BonusResponse_t* _BonusResponse,
                   const axutil_env_t *env, int i)
           {
                void *element = NULL;
                int size = 0;
                int j;
                axis2_bool_t non_nil_exists = AXIS2_FALSE;

                int k = 0;

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusResponse, AXIS2_FAILURE);

                if(_BonusResponse->property_income == NULL ||
                            _BonusResponse->is_valid_income == AXIS2_FALSE)
                {
                    
                    non_nil_exists = AXIS2_FALSE;
                }
                else
                {
                    size = axutil_array_list_size(_BonusResponse->property_income, env);
                    for(j = 0, k = 0; j < size; j ++ )
                    {
                        if(i == j) continue; 
                        if(NULL != axutil_array_list_get(_BonusResponse->property_income, env, i))
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
                       AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Size of the array of income is beinng set to be smaller than the specificed number of minOccurs(0)");
                       return AXIS2_FAILURE;
                }
 
                if(_BonusResponse->property_income == NULL)
                {
                    _BonusResponse->is_valid_income = AXIS2_FALSE;
                    
                    return AXIS2_SUCCESS;
                }

                /* check whether there already exist an element */
                element = axutil_array_list_get(_BonusResponse->property_income, env, i);
                if(NULL != element)
                {
                  
                  
                  adb_ClassMilesIncome_free((adb_ClassMilesIncome_t*)element, env);
                     
                }

                
                    if(!non_nil_exists)
                    {
                        
                        _BonusResponse->is_valid_income = AXIS2_FALSE;
                        axutil_array_list_set(_BonusResponse->property_income , env, i, NULL);
                        return AXIS2_SUCCESS;
                    }
                

                
                axutil_array_list_set(_BonusResponse->property_income , env, i, NULL);
                
                return AXIS2_SUCCESS;

           }

           

