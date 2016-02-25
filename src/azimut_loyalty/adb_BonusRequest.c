

        /**
         * adb_BonusRequest.c
         *
         * This file was auto-generated from WSDL
         * by the Apache Axis2/C version: SNAPSHOT  Built on : Mar 10, 2008 (08:35:52 GMT+00:00)
         */

        #include "adb_BonusRequest.h"
        
                /*
                 * This type was generated from the piece of schema that had
                 * name = BonusRequest
                 * Namespace URI = http://ffpServer.aeronavigator.ru/xsd
                 * Namespace Prefix = ns1
                 */
           


        struct adb_BonusRequest
        {
            axis2_char_t *property_Type;

            axis2_char_t* property_arrival;

                
                axis2_bool_t is_valid_arrival;
            int property_count;

                
                axis2_bool_t is_valid_count;
            axis2_char_t* property_departure;

                
                axis2_bool_t is_valid_departure;
            axis2_char_t* property_ffpCardNumber;

                
                axis2_bool_t is_valid_ffpCardNumber;
            axutil_date_time_t* property_flightDate;

                
                axis2_bool_t is_valid_flightDate;
            axis2_char_t* property_subClass;

                
                axis2_bool_t is_valid_subClass;
            
        };


       /************************* Private Function prototypes ********************************/
        


       /************************* Function Implmentations ********************************/
        adb_BonusRequest_t* AXIS2_CALL
        adb_BonusRequest_create(
            const axutil_env_t *env)
        {
            adb_BonusRequest_t *_BonusRequest = NULL;
            
            AXIS2_ENV_CHECK(env, NULL);

            _BonusRequest = (adb_BonusRequest_t *) AXIS2_MALLOC(env->
                allocator, sizeof(adb_BonusRequest_t));

            if(NULL == _BonusRequest)
            {
                AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
                return NULL;
            }

            memset(_BonusRequest, 0, sizeof(adb_BonusRequest_t));

            _BonusRequest->property_Type = axutil_strdup(env, "adb_BonusRequest");
            _BonusRequest->property_arrival  = NULL;
                  _BonusRequest->is_valid_arrival  = AXIS2_FALSE;
            _BonusRequest->is_valid_count  = AXIS2_FALSE;
            _BonusRequest->property_departure  = NULL;
                  _BonusRequest->is_valid_departure  = AXIS2_FALSE;
            _BonusRequest->property_ffpCardNumber  = NULL;
                  _BonusRequest->is_valid_ffpCardNumber  = AXIS2_FALSE;
            _BonusRequest->property_flightDate  = NULL;
                  _BonusRequest->is_valid_flightDate  = AXIS2_FALSE;
            _BonusRequest->property_subClass  = NULL;
                  _BonusRequest->is_valid_subClass  = AXIS2_FALSE;
            

            return _BonusRequest;
        }

        adb_BonusRequest_t* AXIS2_CALL
        adb_BonusRequest_create_with_values(
            const axutil_env_t *env,
                axis2_char_t* _arrival,
                int _count,
                axis2_char_t* _departure,
                axis2_char_t* _ffpCardNumber,
                axutil_date_time_t* _flightDate,
                axis2_char_t* _subClass)
        {
            adb_BonusRequest_t* adb_obj = NULL;
            axis2_status_t status = AXIS2_SUCCESS;

            adb_obj = adb_BonusRequest_create(env);

            
              status = adb_BonusRequest_set_arrival(
                                     adb_obj,
                                     env,
                                     _arrival);
              if(status == AXIS2_FAILURE) {
                  adb_BonusRequest_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_BonusRequest_set_count(
                                     adb_obj,
                                     env,
                                     _count);
              if(status == AXIS2_FAILURE) {
                  adb_BonusRequest_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_BonusRequest_set_departure(
                                     adb_obj,
                                     env,
                                     _departure);
              if(status == AXIS2_FAILURE) {
                  adb_BonusRequest_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_BonusRequest_set_ffpCardNumber(
                                     adb_obj,
                                     env,
                                     _ffpCardNumber);
              if(status == AXIS2_FAILURE) {
                  adb_BonusRequest_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_BonusRequest_set_flightDate(
                                     adb_obj,
                                     env,
                                     _flightDate);
              if(status == AXIS2_FAILURE) {
                  adb_BonusRequest_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_BonusRequest_set_subClass(
                                     adb_obj,
                                     env,
                                     _subClass);
              if(status == AXIS2_FAILURE) {
                  adb_BonusRequest_free (adb_obj, env);
                  return NULL;
              }
            

            return adb_obj;
        }
      
        axis2_char_t* AXIS2_CALL
                adb_BonusRequest_free_popping_value(
                        adb_BonusRequest_t* _BonusRequest,
                        const axutil_env_t *env)
                {
                    axis2_char_t* value;

                    
                    
                    value = _BonusRequest->property_arrival;

                    _BonusRequest->property_arrival = (axis2_char_t*)NULL;
                    adb_BonusRequest_free(_BonusRequest, env);

                    return value;
                }
            

        axis2_status_t AXIS2_CALL
        adb_BonusRequest_free(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env)
        {
            
            
            return axis2_extension_mapper_free(
                (adb_type_t*) _BonusRequest,
                env,
                "adb_BonusRequest");
            
        }

        axis2_status_t AXIS2_CALL
        adb_BonusRequest_free_obj(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env)
        {
            

            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);

            if (_BonusRequest->property_Type != NULL)
            {
              AXIS2_FREE(env->allocator, _BonusRequest->property_Type);
            }

            adb_BonusRequest_reset_arrival(_BonusRequest, env);
            adb_BonusRequest_reset_count(_BonusRequest, env);
            adb_BonusRequest_reset_departure(_BonusRequest, env);
            adb_BonusRequest_reset_ffpCardNumber(_BonusRequest, env);
            adb_BonusRequest_reset_flightDate(_BonusRequest, env);
            adb_BonusRequest_reset_subClass(_BonusRequest, env);
            

            if(_BonusRequest)
            {
                AXIS2_FREE(env->allocator, _BonusRequest);
                _BonusRequest = NULL;
            }

            return AXIS2_SUCCESS;
        }


        

        axis2_status_t AXIS2_CALL
        adb_BonusRequest_deserialize(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env,
                axiom_node_t **dp_parent,
                axis2_bool_t *dp_is_early_node_valid,
                axis2_bool_t dont_care_minoccurs)
        {
            
            
            return axis2_extension_mapper_deserialize(
                (adb_type_t*) _BonusRequest,
                env,
                dp_parent,
                dp_is_early_node_valid,
                dont_care_minoccurs,
                "adb_BonusRequest");
            
        }

        axis2_status_t AXIS2_CALL
        adb_BonusRequest_deserialize_obj(
                adb_BonusRequest_t* _BonusRequest,
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
            AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);

            
              
              while(parent && axiom_node_get_node_type(parent, env) != AXIOM_ELEMENT)
              {
                  parent = axiom_node_get_next_sibling(parent, env);
              }
              if (NULL == parent)
              {
                /* This should be checked before everything */
                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                            "Failed in building adb object for BonusRequest : "
                            "NULL element can not be passed to deserialize");
                return AXIS2_FAILURE;
              }
              
                      
                      first_node = axiom_node_get_first_child(parent, env);
                      
                    

                     
                     /*
                      * building arrival element
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
                                   
                                 element_qname = axutil_qname_create(env, "arrival", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_BonusRequest_set_arrival(_BonusRequest, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for arrival ");
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
                      * building count element
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
                                 
                                 element_qname = axutil_qname_create(env, "count", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_BonusRequest_set_count(_BonusRequest, env,
                                                                   atoi(text_value));
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for count ");
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
                      * building departure element
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
                                 
                                 element_qname = axutil_qname_create(env, "departure", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_BonusRequest_set_departure(_BonusRequest, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for departure ");
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
                      * building ffpCardNumber element
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
                                 
                                 element_qname = axutil_qname_create(env, "ffpCardNumber", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_BonusRequest_set_ffpCardNumber(_BonusRequest, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for ffpCardNumber ");
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
                      * building flightDate element
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
                                 
                                 element_qname = axutil_qname_create(env, "flightDate", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                              AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in building element flightDate ");
                                          }
                                          else
                                          {
                                            status = adb_BonusRequest_set_flightDate(_BonusRequest, env,
                                                                       (axutil_date_time_t*)element);
                                          }
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for flightDate ");
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
                      * building subClass element
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
                                 
                                 element_qname = axutil_qname_create(env, "subClass", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_BonusRequest_set_subClass(_BonusRequest, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for subClass ");
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
          adb_BonusRequest_is_particle()
          {
            
                 return AXIS2_FALSE;
              
          }


          void AXIS2_CALL
          adb_BonusRequest_declare_parent_namespaces(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index)
          {
            
                  /* Here this is an empty function, Nothing to declare */
                 
          }

        
        
        axiom_node_t* AXIS2_CALL
        adb_BonusRequest_serialize(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env, axiom_node_t *parent, axiom_element_t *parent_element, int parent_tag_closed, axutil_hash_t *namespaces, int *next_ns_index)
        {
            
            
            if (_BonusRequest == NULL)
            {
                return adb_BonusRequest_serialize_obj(
                    _BonusRequest, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index);
            }
            else
            {
                return axis2_extension_mapper_serialize(
                    (adb_type_t*) _BonusRequest, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index, "adb_BonusRequest");
            }
            
        }

        axiom_node_t* AXIS2_CALL
        adb_BonusRequest_serialize_obj(
                adb_BonusRequest_t* _BonusRequest,
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
            
                    axis2_char_t *text_value_1;
                    axis2_char_t *text_value_1_temp;
                    
                    axis2_char_t text_value_2[ADB_DEFAULT_DIGIT_LIMIT];
                    
                    axis2_char_t *text_value_3;
                    axis2_char_t *text_value_3_temp;
                    
                    axis2_char_t *text_value_4;
                    axis2_char_t *text_value_4_temp;
                    
                    axis2_char_t *text_value_5;
                    axis2_char_t *text_value_5_temp;
                    
                    axis2_char_t *text_value_6;
                    axis2_char_t *text_value_6_temp;
                    
               axis2_char_t *start_input_str = NULL;
               axis2_char_t *end_input_str = NULL;
               unsigned int start_input_str_len = 0;
               unsigned int end_input_str_len = 0;
            
            
               axiom_data_source_t *data_source = NULL;
               axutil_stream_t *stream = NULL;

            

            AXIS2_ENV_CHECK(env, NULL);
            AXIS2_PARAM_CHECK(env->error, _BonusRequest, NULL);
            
            
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
              type_attrib = axutil_strcat(env, " ", xsi_prefix, ":type=\"BonusRequest\"", NULL);
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
            xsi_type_attri = axiom_attribute_create (env, "type", "BonusRequest", xsi_ns);
            
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
                      

                   if (!_BonusRequest->is_valid_arrival)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("arrival"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("arrival")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing arrival element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sarrival>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sarrival>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_1 = _BonusRequest->property_arrival;
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                            
                           text_value_1_temp = axutil_xml_quote_string(env, text_value_1, AXIS2_TRUE);
                           if (text_value_1_temp)
                           {
                               axutil_stream_write(stream, env, text_value_1_temp, axutil_strlen(text_value_1_temp));
                               AXIS2_FREE(env->allocator, text_value_1_temp);
                           }
                           else
                           {
                               axutil_stream_write(stream, env, text_value_1, axutil_strlen(text_value_1));
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
                      

                   if (!_BonusRequest->is_valid_count)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("count"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("count")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing count element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%scount>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%scount>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                               sprintf (text_value_2, AXIS2_PRINTF_INT32_FORMAT_SPECIFIER, _BonusRequest->property_count);
                             
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
                      

                   if (!_BonusRequest->is_valid_departure)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("departure"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("departure")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing departure element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sdeparture>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sdeparture>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_3 = _BonusRequest->property_departure;
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                            
                           text_value_3_temp = axutil_xml_quote_string(env, text_value_3, AXIS2_TRUE);
                           if (text_value_3_temp)
                           {
                               axutil_stream_write(stream, env, text_value_3_temp, axutil_strlen(text_value_3_temp));
                               AXIS2_FREE(env->allocator, text_value_3_temp);
                           }
                           else
                           {
                               axutil_stream_write(stream, env, text_value_3, axutil_strlen(text_value_3));
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
                      

                   if (!_BonusRequest->is_valid_ffpCardNumber)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("ffpCardNumber"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("ffpCardNumber")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing ffpCardNumber element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sffpCardNumber>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sffpCardNumber>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_4 = _BonusRequest->property_ffpCardNumber;
                           
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
                      

                   if (!_BonusRequest->is_valid_flightDate)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("flightDate"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("flightDate")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing flightDate element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sflightDate>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sflightDate>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                          text_value_5 = axutil_date_time_serialize_date_time(_BonusRequest->property_flightDate, env);
                           
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                           axutil_stream_write(stream, env, text_value_5, axutil_strlen(text_value_5));
                           
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
                      

                   if (!_BonusRequest->is_valid_subClass)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("subClass"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("subClass")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing subClass element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%ssubClass>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%ssubClass>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_6 = _BonusRequest->property_subClass;
                           
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

                 

            return parent;
        }


        

            /**
             * Getter for arrival by  Property Number 1
             */
            axis2_char_t* AXIS2_CALL
            adb_BonusRequest_get_property1(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env)
            {
                return adb_BonusRequest_get_arrival(_BonusRequest,
                                             env);
            }

            /**
             * getter for arrival.
             */
            axis2_char_t* AXIS2_CALL
            adb_BonusRequest_get_arrival(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _BonusRequest, NULL);
                  

                return _BonusRequest->property_arrival;
             }

            /**
             * setter for arrival
             */
            axis2_status_t AXIS2_CALL
            adb_BonusRequest_set_arrival(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_arrival)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
                
                if(_BonusRequest->is_valid_arrival &&
                        arg_arrival == _BonusRequest->property_arrival)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_BonusRequest_reset_arrival(_BonusRequest, env);

                
                if(NULL == arg_arrival)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _BonusRequest->property_arrival = (axis2_char_t *)axutil_strdup(env, arg_arrival);
                        if(NULL == _BonusRequest->property_arrival)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for arrival");
                            return AXIS2_FAILURE;
                        }
                        _BonusRequest->is_valid_arrival = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for arrival
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_reset_arrival(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
               

               
            
                
                if(_BonusRequest->property_arrival != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _BonusRequest->property_arrival);
                     _BonusRequest->property_arrival = NULL;
                }
            
                
                
                _BonusRequest->is_valid_arrival = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether arrival is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_BonusRequest_is_arrival_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_TRUE);
               
               return !_BonusRequest->is_valid_arrival;
           }

           /**
            * Set arrival to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_set_arrival_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               return adb_BonusRequest_reset_arrival(_BonusRequest, env);
           }

           

            /**
             * Getter for count by  Property Number 2
             */
            int AXIS2_CALL
            adb_BonusRequest_get_property2(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env)
            {
                return adb_BonusRequest_get_count(_BonusRequest,
                                             env);
            }

            /**
             * getter for count.
             */
            int AXIS2_CALL
            adb_BonusRequest_get_count(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (int)0);
                    AXIS2_PARAM_CHECK(env->error, _BonusRequest, (int)0);
                  

                return _BonusRequest->property_count;
             }

            /**
             * setter for count
             */
            axis2_status_t AXIS2_CALL
            adb_BonusRequest_set_count(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env,
                    const int  arg_count)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
                
                if(_BonusRequest->is_valid_count &&
                        arg_count == _BonusRequest->property_count)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_BonusRequest_reset_count(_BonusRequest, env);

                _BonusRequest->property_count = arg_count;
                        _BonusRequest->is_valid_count = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for count
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_reset_count(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
               

               _BonusRequest->is_valid_count = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether count is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_BonusRequest_is_count_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_TRUE);
               
               return !_BonusRequest->is_valid_count;
           }

           /**
            * Set count to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_set_count_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               return adb_BonusRequest_reset_count(_BonusRequest, env);
           }

           

            /**
             * Getter for departure by  Property Number 3
             */
            axis2_char_t* AXIS2_CALL
            adb_BonusRequest_get_property3(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env)
            {
                return adb_BonusRequest_get_departure(_BonusRequest,
                                             env);
            }

            /**
             * getter for departure.
             */
            axis2_char_t* AXIS2_CALL
            adb_BonusRequest_get_departure(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _BonusRequest, NULL);
                  

                return _BonusRequest->property_departure;
             }

            /**
             * setter for departure
             */
            axis2_status_t AXIS2_CALL
            adb_BonusRequest_set_departure(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_departure)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
                
                if(_BonusRequest->is_valid_departure &&
                        arg_departure == _BonusRequest->property_departure)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_BonusRequest_reset_departure(_BonusRequest, env);

                
                if(NULL == arg_departure)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _BonusRequest->property_departure = (axis2_char_t *)axutil_strdup(env, arg_departure);
                        if(NULL == _BonusRequest->property_departure)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for departure");
                            return AXIS2_FAILURE;
                        }
                        _BonusRequest->is_valid_departure = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for departure
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_reset_departure(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
               

               
            
                
                if(_BonusRequest->property_departure != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _BonusRequest->property_departure);
                     _BonusRequest->property_departure = NULL;
                }
            
                
                
                _BonusRequest->is_valid_departure = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether departure is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_BonusRequest_is_departure_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_TRUE);
               
               return !_BonusRequest->is_valid_departure;
           }

           /**
            * Set departure to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_set_departure_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               return adb_BonusRequest_reset_departure(_BonusRequest, env);
           }

           

            /**
             * Getter for ffpCardNumber by  Property Number 4
             */
            axis2_char_t* AXIS2_CALL
            adb_BonusRequest_get_property4(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env)
            {
                return adb_BonusRequest_get_ffpCardNumber(_BonusRequest,
                                             env);
            }

            /**
             * getter for ffpCardNumber.
             */
            axis2_char_t* AXIS2_CALL
            adb_BonusRequest_get_ffpCardNumber(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _BonusRequest, NULL);
                  

                return _BonusRequest->property_ffpCardNumber;
             }

            /**
             * setter for ffpCardNumber
             */
            axis2_status_t AXIS2_CALL
            adb_BonusRequest_set_ffpCardNumber(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_ffpCardNumber)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
                
                if(_BonusRequest->is_valid_ffpCardNumber &&
                        arg_ffpCardNumber == _BonusRequest->property_ffpCardNumber)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_BonusRequest_reset_ffpCardNumber(_BonusRequest, env);

                
                if(NULL == arg_ffpCardNumber)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _BonusRequest->property_ffpCardNumber = (axis2_char_t *)axutil_strdup(env, arg_ffpCardNumber);
                        if(NULL == _BonusRequest->property_ffpCardNumber)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for ffpCardNumber");
                            return AXIS2_FAILURE;
                        }
                        _BonusRequest->is_valid_ffpCardNumber = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for ffpCardNumber
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_reset_ffpCardNumber(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
               

               
            
                
                if(_BonusRequest->property_ffpCardNumber != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _BonusRequest->property_ffpCardNumber);
                     _BonusRequest->property_ffpCardNumber = NULL;
                }
            
                
                
                _BonusRequest->is_valid_ffpCardNumber = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether ffpCardNumber is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_BonusRequest_is_ffpCardNumber_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_TRUE);
               
               return !_BonusRequest->is_valid_ffpCardNumber;
           }

           /**
            * Set ffpCardNumber to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_set_ffpCardNumber_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               return adb_BonusRequest_reset_ffpCardNumber(_BonusRequest, env);
           }

           

            /**
             * Getter for flightDate by  Property Number 5
             */
            axutil_date_time_t* AXIS2_CALL
            adb_BonusRequest_get_property5(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env)
            {
                return adb_BonusRequest_get_flightDate(_BonusRequest,
                                             env);
            }

            /**
             * getter for flightDate.
             */
            axutil_date_time_t* AXIS2_CALL
            adb_BonusRequest_get_flightDate(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _BonusRequest, NULL);
                  

                return _BonusRequest->property_flightDate;
             }

            /**
             * setter for flightDate
             */
            axis2_status_t AXIS2_CALL
            adb_BonusRequest_set_flightDate(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env,
                    axutil_date_time_t*  arg_flightDate)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
                
                if(_BonusRequest->is_valid_flightDate &&
                        arg_flightDate == _BonusRequest->property_flightDate)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_BonusRequest_reset_flightDate(_BonusRequest, env);

                
                if(NULL == arg_flightDate)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _BonusRequest->property_flightDate = arg_flightDate;
                        _BonusRequest->is_valid_flightDate = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for flightDate
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_reset_flightDate(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
               

               
            
                
                if(_BonusRequest->property_flightDate != NULL)
                {
                   
                   
                      axutil_date_time_free(_BonusRequest->property_flightDate, env);
                     _BonusRequest->property_flightDate = NULL;
                }
            
                
                
                _BonusRequest->is_valid_flightDate = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether flightDate is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_BonusRequest_is_flightDate_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_TRUE);
               
               return !_BonusRequest->is_valid_flightDate;
           }

           /**
            * Set flightDate to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_set_flightDate_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               return adb_BonusRequest_reset_flightDate(_BonusRequest, env);
           }

           

            /**
             * Getter for subClass by  Property Number 6
             */
            axis2_char_t* AXIS2_CALL
            adb_BonusRequest_get_property6(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env)
            {
                return adb_BonusRequest_get_subClass(_BonusRequest,
                                             env);
            }

            /**
             * getter for subClass.
             */
            axis2_char_t* AXIS2_CALL
            adb_BonusRequest_get_subClass(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _BonusRequest, NULL);
                  

                return _BonusRequest->property_subClass;
             }

            /**
             * setter for subClass
             */
            axis2_status_t AXIS2_CALL
            adb_BonusRequest_set_subClass(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_subClass)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
                
                if(_BonusRequest->is_valid_subClass &&
                        arg_subClass == _BonusRequest->property_subClass)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_BonusRequest_reset_subClass(_BonusRequest, env);

                
                if(NULL == arg_subClass)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _BonusRequest->property_subClass = (axis2_char_t *)axutil_strdup(env, arg_subClass);
                        if(NULL == _BonusRequest->property_subClass)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for subClass");
                            return AXIS2_FAILURE;
                        }
                        _BonusRequest->is_valid_subClass = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for subClass
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_reset_subClass(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_FAILURE);
               

               
            
                
                if(_BonusRequest->property_subClass != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _BonusRequest->property_subClass);
                     _BonusRequest->property_subClass = NULL;
                }
            
                
                
                _BonusRequest->is_valid_subClass = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether subClass is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_BonusRequest_is_subClass_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _BonusRequest, AXIS2_TRUE);
               
               return !_BonusRequest->is_valid_subClass;
           }

           /**
            * Set subClass to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_BonusRequest_set_subClass_nil(
                   adb_BonusRequest_t* _BonusRequest,
                   const axutil_env_t *env)
           {
               return adb_BonusRequest_reset_subClass(_BonusRequest, env);
           }

           

