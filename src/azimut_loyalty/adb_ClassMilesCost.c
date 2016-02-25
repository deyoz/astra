

        /**
         * adb_ClassMilesCost.c
         *
         * This file was auto-generated from WSDL
         * by the Apache Axis2/C version: SNAPSHOT  Built on : Mar 10, 2008 (08:35:52 GMT+00:00)
         */

        #include "adb_ClassMilesCost.h"
        
                /*
                 * This type was generated from the piece of schema that had
                 * name = ClassMilesCost
                 * Namespace URI = http://ffpServer.aeronavigator.ru/xsd
                 * Namespace Prefix = ns1
                 */
           


        struct adb_ClassMilesCost
        {
            axis2_char_t *property_Type;

            axis2_char_t* property_arrival;

                
                axis2_bool_t is_valid_arrival;
            axis2_char_t* property_arrivalRegion;

                
                axis2_bool_t is_valid_arrivalRegion;
            axis2_char_t* property_classDesc;

                
                axis2_bool_t is_valid_classDesc;
            axis2_char_t* property_className;

                
                axis2_bool_t is_valid_className;
            int property_count;

                
                axis2_bool_t is_valid_count;
            axis2_char_t* property_departure;

                
                axis2_bool_t is_valid_departure;
            axis2_char_t* property_departureRegion;

                
                axis2_bool_t is_valid_departureRegion;
            double property_milesOW;

                
                axis2_bool_t is_valid_milesOW;
            double property_milesRT;

                
                axis2_bool_t is_valid_milesRT;
            
        };


       /************************* Private Function prototypes ********************************/
        


       /************************* Function Implmentations ********************************/
        adb_ClassMilesCost_t* AXIS2_CALL
        adb_ClassMilesCost_create(
            const axutil_env_t *env)
        {
            adb_ClassMilesCost_t *_ClassMilesCost = NULL;
            
            AXIS2_ENV_CHECK(env, NULL);

            _ClassMilesCost = (adb_ClassMilesCost_t *) AXIS2_MALLOC(env->
                allocator, sizeof(adb_ClassMilesCost_t));

            if(NULL == _ClassMilesCost)
            {
                AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
                return NULL;
            }

            memset(_ClassMilesCost, 0, sizeof(adb_ClassMilesCost_t));

            _ClassMilesCost->property_Type = axutil_strdup(env, "adb_ClassMilesCost");
            _ClassMilesCost->property_arrival  = NULL;
                  _ClassMilesCost->is_valid_arrival  = AXIS2_FALSE;
            _ClassMilesCost->property_arrivalRegion  = NULL;
                  _ClassMilesCost->is_valid_arrivalRegion  = AXIS2_FALSE;
            _ClassMilesCost->property_classDesc  = NULL;
                  _ClassMilesCost->is_valid_classDesc  = AXIS2_FALSE;
            _ClassMilesCost->property_className  = NULL;
                  _ClassMilesCost->is_valid_className  = AXIS2_FALSE;
            _ClassMilesCost->is_valid_count  = AXIS2_FALSE;
            _ClassMilesCost->property_departure  = NULL;
                  _ClassMilesCost->is_valid_departure  = AXIS2_FALSE;
            _ClassMilesCost->property_departureRegion  = NULL;
                  _ClassMilesCost->is_valid_departureRegion  = AXIS2_FALSE;
            _ClassMilesCost->is_valid_milesOW  = AXIS2_FALSE;
            _ClassMilesCost->is_valid_milesRT  = AXIS2_FALSE;
            

            return _ClassMilesCost;
        }

        adb_ClassMilesCost_t* AXIS2_CALL
        adb_ClassMilesCost_create_with_values(
            const axutil_env_t *env,
                axis2_char_t* _arrival,
                axis2_char_t* _arrivalRegion,
                axis2_char_t* _classDesc,
                axis2_char_t* _className,
                int _count,
                axis2_char_t* _departure,
                axis2_char_t* _departureRegion,
                double _milesOW,
                double _milesRT)
        {
            adb_ClassMilesCost_t* adb_obj = NULL;
            axis2_status_t status = AXIS2_SUCCESS;

            adb_obj = adb_ClassMilesCost_create(env);

            
              status = adb_ClassMilesCost_set_arrival(
                                     adb_obj,
                                     env,
                                     _arrival);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesCost_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesCost_set_arrivalRegion(
                                     adb_obj,
                                     env,
                                     _arrivalRegion);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesCost_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesCost_set_classDesc(
                                     adb_obj,
                                     env,
                                     _classDesc);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesCost_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesCost_set_className(
                                     adb_obj,
                                     env,
                                     _className);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesCost_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesCost_set_count(
                                     adb_obj,
                                     env,
                                     _count);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesCost_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesCost_set_departure(
                                     adb_obj,
                                     env,
                                     _departure);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesCost_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesCost_set_departureRegion(
                                     adb_obj,
                                     env,
                                     _departureRegion);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesCost_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesCost_set_milesOW(
                                     adb_obj,
                                     env,
                                     _milesOW);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesCost_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesCost_set_milesRT(
                                     adb_obj,
                                     env,
                                     _milesRT);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesCost_free (adb_obj, env);
                  return NULL;
              }
            

            return adb_obj;
        }
      
        axis2_char_t* AXIS2_CALL
                adb_ClassMilesCost_free_popping_value(
                        adb_ClassMilesCost_t* _ClassMilesCost,
                        const axutil_env_t *env)
                {
                    axis2_char_t* value;

                    
                    
                    value = _ClassMilesCost->property_arrival;

                    _ClassMilesCost->property_arrival = (axis2_char_t*)NULL;
                    adb_ClassMilesCost_free(_ClassMilesCost, env);

                    return value;
                }
            

        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_free(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
        {
            
            
            return axis2_extension_mapper_free(
                (adb_type_t*) _ClassMilesCost,
                env,
                "adb_ClassMilesCost");
            
        }

        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_free_obj(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
        {
            

            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);

            if (_ClassMilesCost->property_Type != NULL)
            {
              AXIS2_FREE(env->allocator, _ClassMilesCost->property_Type);
            }

            adb_ClassMilesCost_reset_arrival(_ClassMilesCost, env);
            adb_ClassMilesCost_reset_arrivalRegion(_ClassMilesCost, env);
            adb_ClassMilesCost_reset_classDesc(_ClassMilesCost, env);
            adb_ClassMilesCost_reset_className(_ClassMilesCost, env);
            adb_ClassMilesCost_reset_count(_ClassMilesCost, env);
            adb_ClassMilesCost_reset_departure(_ClassMilesCost, env);
            adb_ClassMilesCost_reset_departureRegion(_ClassMilesCost, env);
            adb_ClassMilesCost_reset_milesOW(_ClassMilesCost, env);
            adb_ClassMilesCost_reset_milesRT(_ClassMilesCost, env);
            

            if(_ClassMilesCost)
            {
                AXIS2_FREE(env->allocator, _ClassMilesCost);
                _ClassMilesCost = NULL;
            }

            return AXIS2_SUCCESS;
        }


        

        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_deserialize(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env,
                axiom_node_t **dp_parent,
                axis2_bool_t *dp_is_early_node_valid,
                axis2_bool_t dont_care_minoccurs)
        {
            
            
            return axis2_extension_mapper_deserialize(
                (adb_type_t*) _ClassMilesCost,
                env,
                dp_parent,
                dp_is_early_node_valid,
                dont_care_minoccurs,
                "adb_ClassMilesCost");
            
        }

        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_deserialize_obj(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env,
                axiom_node_t **dp_parent,
                axis2_bool_t *dp_is_early_node_valid,
                axis2_bool_t dont_care_minoccurs)
        {
          axiom_node_t *parent = *dp_parent;
          
          axis2_status_t status = AXIS2_SUCCESS;
           
             const axis2_char_t* text_value = NULL;
             axutil_qname_t *qname = NULL;
          
            axutil_qname_t *element_qname = NULL; 
            
               axiom_node_t *first_node = NULL;
               axis2_bool_t is_early_node_valid = AXIS2_TRUE;
               axiom_node_t *current_node = NULL;
               axiom_element_t *current_element = NULL;
            
            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);

            
              
              while(parent && axiom_node_get_node_type(parent, env) != AXIOM_ELEMENT)
              {
                  parent = axiom_node_get_next_sibling(parent, env);
              }
              if (NULL == parent)
              {
                /* This should be checked before everything */
                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                            "Failed in building adb object for ClassMilesCost : "
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
                                            status = adb_ClassMilesCost_set_arrival(_ClassMilesCost, env,
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
                      * building arrivalRegion element
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
                                 
                                 element_qname = axutil_qname_create(env, "arrivalRegion", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesCost_set_arrivalRegion(_ClassMilesCost, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for arrivalRegion ");
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
                      * building classDesc element
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
                                 
                                 element_qname = axutil_qname_create(env, "classDesc", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesCost_set_classDesc(_ClassMilesCost, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for classDesc ");
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
                      * building className element
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
                                 
                                 element_qname = axutil_qname_create(env, "className", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesCost_set_className(_ClassMilesCost, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for className ");
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
                                            status = adb_ClassMilesCost_set_count(_ClassMilesCost, env,
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
                                            status = adb_ClassMilesCost_set_departure(_ClassMilesCost, env,
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
                      * building departureRegion element
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
                                 
                                 element_qname = axutil_qname_create(env, "departureRegion", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesCost_set_departureRegion(_ClassMilesCost, env,
                                                               text_value);
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for departureRegion ");
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
                      * building milesOW element
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
                                 
                                 element_qname = axutil_qname_create(env, "milesOW", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesCost_set_milesOW(_ClassMilesCost, env,
                                                                   atof(text_value));
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for milesOW ");
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
                      * building milesRT element
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
                                 
                                 element_qname = axutil_qname_create(env, "milesRT", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesCost_set_milesRT(_ClassMilesCost, env,
                                                                   atof(text_value));
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for milesRT ");
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
          adb_ClassMilesCost_is_particle()
          {
            
                 return AXIS2_FALSE;
              
          }


          void AXIS2_CALL
          adb_ClassMilesCost_declare_parent_namespaces(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index)
          {
            
                  /* Here this is an empty function, Nothing to declare */
                 
          }

        
        
        axiom_node_t* AXIS2_CALL
        adb_ClassMilesCost_serialize(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env, axiom_node_t *parent, axiom_element_t *parent_element, int parent_tag_closed, axutil_hash_t *namespaces, int *next_ns_index)
        {
            
            
            if (_ClassMilesCost == NULL)
            {
                return adb_ClassMilesCost_serialize_obj(
                    _ClassMilesCost, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index);
            }
            else
            {
                return axis2_extension_mapper_serialize(
                    (adb_type_t*) _ClassMilesCost, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index, "adb_ClassMilesCost");
            }
            
        }

        axiom_node_t* AXIS2_CALL
        adb_ClassMilesCost_serialize_obj(
                adb_ClassMilesCost_t* _ClassMilesCost,
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
                    
                    axis2_char_t *text_value_2;
                    axis2_char_t *text_value_2_temp;
                    
                    axis2_char_t *text_value_3;
                    axis2_char_t *text_value_3_temp;
                    
                    axis2_char_t *text_value_4;
                    axis2_char_t *text_value_4_temp;
                    
                    axis2_char_t text_value_5[ADB_DEFAULT_DIGIT_LIMIT];
                    
                    axis2_char_t *text_value_6;
                    axis2_char_t *text_value_6_temp;
                    
                    axis2_char_t *text_value_7;
                    axis2_char_t *text_value_7_temp;
                    
                    axis2_char_t text_value_8[ADB_DEFAULT_DIGIT_LIMIT];
                    
                    axis2_char_t text_value_9[ADB_DEFAULT_DIGIT_LIMIT];
                    
               axis2_char_t *start_input_str = NULL;
               axis2_char_t *end_input_str = NULL;
               unsigned int start_input_str_len = 0;
               unsigned int end_input_str_len = 0;
            
            
               axiom_data_source_t *data_source = NULL;
               axutil_stream_t *stream = NULL;

            

            AXIS2_ENV_CHECK(env, NULL);
            AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, NULL);
            
            
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
              type_attrib = axutil_strcat(env, " ", xsi_prefix, ":type=\"ClassMilesCost\"", NULL);
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
            xsi_type_attri = axiom_attribute_create (env, "type", "ClassMilesCost", xsi_ns);
            
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
                      

                   if (!_ClassMilesCost->is_valid_arrival)
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
                    
                           text_value_1 = _ClassMilesCost->property_arrival;
                           
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
                      

                   if (!_ClassMilesCost->is_valid_arrivalRegion)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("arrivalRegion"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("arrivalRegion")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing arrivalRegion element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sarrivalRegion>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sarrivalRegion>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_2 = _ClassMilesCost->property_arrivalRegion;
                           
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
                      

                   if (!_ClassMilesCost->is_valid_classDesc)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("classDesc"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("classDesc")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing classDesc element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sclassDesc>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sclassDesc>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_3 = _ClassMilesCost->property_classDesc;
                           
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
                      

                   if (!_ClassMilesCost->is_valid_className)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("className"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("className")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing className element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sclassName>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sclassName>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_4 = _ClassMilesCost->property_className;
                           
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
                      

                   if (!_ClassMilesCost->is_valid_count)
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
                    
                               sprintf (text_value_5, AXIS2_PRINTF_INT32_FORMAT_SPECIFIER, _ClassMilesCost->property_count);
                             
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
                      

                   if (!_ClassMilesCost->is_valid_departure)
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
                    
                           text_value_6 = _ClassMilesCost->property_departure;
                           
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
                      

                   if (!_ClassMilesCost->is_valid_departureRegion)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("departureRegion"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("departureRegion")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing departureRegion element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sdepartureRegion>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sdepartureRegion>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                           text_value_7 = _ClassMilesCost->property_departureRegion;
                           
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
                      

                   if (!_ClassMilesCost->is_valid_milesOW)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("milesOW"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("milesOW")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing milesOW element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%smilesOW>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%smilesOW>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                               sprintf (text_value_8, "%f", (double)_ClassMilesCost->property_milesOW);
                             
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                           axutil_stream_write(stream, env, text_value_8, axutil_strlen(text_value_8));
                           
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
                      

                   if (!_ClassMilesCost->is_valid_milesRT)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("milesRT"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("milesRT")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing milesRT element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%smilesRT>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%smilesRT>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                               sprintf (text_value_9, "%f", (double)_ClassMilesCost->property_milesRT);
                             
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                           axutil_stream_write(stream, env, text_value_9, axutil_strlen(text_value_9));
                           
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
            adb_ClassMilesCost_get_property1(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
            {
                return adb_ClassMilesCost_get_arrival(_ClassMilesCost,
                                             env);
            }

            /**
             * getter for arrival.
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_arrival(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, NULL);
                  

                return _ClassMilesCost->property_arrival;
             }

            /**
             * setter for arrival
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesCost_set_arrival(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_arrival)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
                
                if(_ClassMilesCost->is_valid_arrival &&
                        arg_arrival == _ClassMilesCost->property_arrival)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesCost_reset_arrival(_ClassMilesCost, env);

                
                if(NULL == arg_arrival)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ClassMilesCost->property_arrival = (axis2_char_t *)axutil_strdup(env, arg_arrival);
                        if(NULL == _ClassMilesCost->property_arrival)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for arrival");
                            return AXIS2_FAILURE;
                        }
                        _ClassMilesCost->is_valid_arrival = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for arrival
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_reset_arrival(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
               

               
            
                
                if(_ClassMilesCost->property_arrival != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _ClassMilesCost->property_arrival);
                     _ClassMilesCost->property_arrival = NULL;
                }
            
                
                
                _ClassMilesCost->is_valid_arrival = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether arrival is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesCost_is_arrival_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_TRUE);
               
               return !_ClassMilesCost->is_valid_arrival;
           }

           /**
            * Set arrival to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_set_arrival_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesCost_reset_arrival(_ClassMilesCost, env);
           }

           

            /**
             * Getter for arrivalRegion by  Property Number 2
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_property2(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
            {
                return adb_ClassMilesCost_get_arrivalRegion(_ClassMilesCost,
                                             env);
            }

            /**
             * getter for arrivalRegion.
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_arrivalRegion(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, NULL);
                  

                return _ClassMilesCost->property_arrivalRegion;
             }

            /**
             * setter for arrivalRegion
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesCost_set_arrivalRegion(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_arrivalRegion)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
                
                if(_ClassMilesCost->is_valid_arrivalRegion &&
                        arg_arrivalRegion == _ClassMilesCost->property_arrivalRegion)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesCost_reset_arrivalRegion(_ClassMilesCost, env);

                
                if(NULL == arg_arrivalRegion)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ClassMilesCost->property_arrivalRegion = (axis2_char_t *)axutil_strdup(env, arg_arrivalRegion);
                        if(NULL == _ClassMilesCost->property_arrivalRegion)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for arrivalRegion");
                            return AXIS2_FAILURE;
                        }
                        _ClassMilesCost->is_valid_arrivalRegion = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for arrivalRegion
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_reset_arrivalRegion(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
               

               
            
                
                if(_ClassMilesCost->property_arrivalRegion != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _ClassMilesCost->property_arrivalRegion);
                     _ClassMilesCost->property_arrivalRegion = NULL;
                }
            
                
                
                _ClassMilesCost->is_valid_arrivalRegion = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether arrivalRegion is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesCost_is_arrivalRegion_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_TRUE);
               
               return !_ClassMilesCost->is_valid_arrivalRegion;
           }

           /**
            * Set arrivalRegion to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_set_arrivalRegion_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesCost_reset_arrivalRegion(_ClassMilesCost, env);
           }

           

            /**
             * Getter for classDesc by  Property Number 3
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_property3(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
            {
                return adb_ClassMilesCost_get_classDesc(_ClassMilesCost,
                                             env);
            }

            /**
             * getter for classDesc.
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_classDesc(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, NULL);
                  

                return _ClassMilesCost->property_classDesc;
             }

            /**
             * setter for classDesc
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesCost_set_classDesc(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_classDesc)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
                
                if(_ClassMilesCost->is_valid_classDesc &&
                        arg_classDesc == _ClassMilesCost->property_classDesc)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesCost_reset_classDesc(_ClassMilesCost, env);

                
                if(NULL == arg_classDesc)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ClassMilesCost->property_classDesc = (axis2_char_t *)axutil_strdup(env, arg_classDesc);
                        if(NULL == _ClassMilesCost->property_classDesc)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for classDesc");
                            return AXIS2_FAILURE;
                        }
                        _ClassMilesCost->is_valid_classDesc = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for classDesc
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_reset_classDesc(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
               

               
            
                
                if(_ClassMilesCost->property_classDesc != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _ClassMilesCost->property_classDesc);
                     _ClassMilesCost->property_classDesc = NULL;
                }
            
                
                
                _ClassMilesCost->is_valid_classDesc = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether classDesc is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesCost_is_classDesc_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_TRUE);
               
               return !_ClassMilesCost->is_valid_classDesc;
           }

           /**
            * Set classDesc to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_set_classDesc_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesCost_reset_classDesc(_ClassMilesCost, env);
           }

           

            /**
             * Getter for className by  Property Number 4
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_property4(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
            {
                return adb_ClassMilesCost_get_className(_ClassMilesCost,
                                             env);
            }

            /**
             * getter for className.
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_className(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, NULL);
                  

                return _ClassMilesCost->property_className;
             }

            /**
             * setter for className
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesCost_set_className(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_className)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
                
                if(_ClassMilesCost->is_valid_className &&
                        arg_className == _ClassMilesCost->property_className)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesCost_reset_className(_ClassMilesCost, env);

                
                if(NULL == arg_className)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ClassMilesCost->property_className = (axis2_char_t *)axutil_strdup(env, arg_className);
                        if(NULL == _ClassMilesCost->property_className)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for className");
                            return AXIS2_FAILURE;
                        }
                        _ClassMilesCost->is_valid_className = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for className
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_reset_className(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
               

               
            
                
                if(_ClassMilesCost->property_className != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _ClassMilesCost->property_className);
                     _ClassMilesCost->property_className = NULL;
                }
            
                
                
                _ClassMilesCost->is_valid_className = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether className is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesCost_is_className_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_TRUE);
               
               return !_ClassMilesCost->is_valid_className;
           }

           /**
            * Set className to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_set_className_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesCost_reset_className(_ClassMilesCost, env);
           }

           

            /**
             * Getter for count by  Property Number 5
             */
            int AXIS2_CALL
            adb_ClassMilesCost_get_property5(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
            {
                return adb_ClassMilesCost_get_count(_ClassMilesCost,
                                             env);
            }

            /**
             * getter for count.
             */
            int AXIS2_CALL
            adb_ClassMilesCost_get_count(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (int)0);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, (int)0);
                  

                return _ClassMilesCost->property_count;
             }

            /**
             * setter for count
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesCost_set_count(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env,
                    const int  arg_count)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
                
                if(_ClassMilesCost->is_valid_count &&
                        arg_count == _ClassMilesCost->property_count)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesCost_reset_count(_ClassMilesCost, env);

                _ClassMilesCost->property_count = arg_count;
                        _ClassMilesCost->is_valid_count = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for count
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_reset_count(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
               

               _ClassMilesCost->is_valid_count = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether count is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesCost_is_count_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_TRUE);
               
               return !_ClassMilesCost->is_valid_count;
           }

           /**
            * Set count to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_set_count_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesCost_reset_count(_ClassMilesCost, env);
           }

           

            /**
             * Getter for departure by  Property Number 6
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_property6(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
            {
                return adb_ClassMilesCost_get_departure(_ClassMilesCost,
                                             env);
            }

            /**
             * getter for departure.
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_departure(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, NULL);
                  

                return _ClassMilesCost->property_departure;
             }

            /**
             * setter for departure
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesCost_set_departure(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_departure)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
                
                if(_ClassMilesCost->is_valid_departure &&
                        arg_departure == _ClassMilesCost->property_departure)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesCost_reset_departure(_ClassMilesCost, env);

                
                if(NULL == arg_departure)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ClassMilesCost->property_departure = (axis2_char_t *)axutil_strdup(env, arg_departure);
                        if(NULL == _ClassMilesCost->property_departure)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for departure");
                            return AXIS2_FAILURE;
                        }
                        _ClassMilesCost->is_valid_departure = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for departure
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_reset_departure(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
               

               
            
                
                if(_ClassMilesCost->property_departure != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _ClassMilesCost->property_departure);
                     _ClassMilesCost->property_departure = NULL;
                }
            
                
                
                _ClassMilesCost->is_valid_departure = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether departure is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesCost_is_departure_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_TRUE);
               
               return !_ClassMilesCost->is_valid_departure;
           }

           /**
            * Set departure to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_set_departure_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesCost_reset_departure(_ClassMilesCost, env);
           }

           

            /**
             * Getter for departureRegion by  Property Number 7
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_property7(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
            {
                return adb_ClassMilesCost_get_departureRegion(_ClassMilesCost,
                                             env);
            }

            /**
             * getter for departureRegion.
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesCost_get_departureRegion(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, NULL);
                  

                return _ClassMilesCost->property_departureRegion;
             }

            /**
             * setter for departureRegion
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesCost_set_departureRegion(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_departureRegion)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
                
                if(_ClassMilesCost->is_valid_departureRegion &&
                        arg_departureRegion == _ClassMilesCost->property_departureRegion)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesCost_reset_departureRegion(_ClassMilesCost, env);

                
                if(NULL == arg_departureRegion)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ClassMilesCost->property_departureRegion = (axis2_char_t *)axutil_strdup(env, arg_departureRegion);
                        if(NULL == _ClassMilesCost->property_departureRegion)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for departureRegion");
                            return AXIS2_FAILURE;
                        }
                        _ClassMilesCost->is_valid_departureRegion = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for departureRegion
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_reset_departureRegion(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
               

               
            
                
                if(_ClassMilesCost->property_departureRegion != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _ClassMilesCost->property_departureRegion);
                     _ClassMilesCost->property_departureRegion = NULL;
                }
            
                
                
                _ClassMilesCost->is_valid_departureRegion = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether departureRegion is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesCost_is_departureRegion_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_TRUE);
               
               return !_ClassMilesCost->is_valid_departureRegion;
           }

           /**
            * Set departureRegion to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_set_departureRegion_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesCost_reset_departureRegion(_ClassMilesCost, env);
           }

           

            /**
             * Getter for milesOW by  Property Number 8
             */
            double AXIS2_CALL
            adb_ClassMilesCost_get_property8(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
            {
                return adb_ClassMilesCost_get_milesOW(_ClassMilesCost,
                                             env);
            }

            /**
             * getter for milesOW.
             */
            double AXIS2_CALL
            adb_ClassMilesCost_get_milesOW(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (double)0);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, (double)0);
                  

                return _ClassMilesCost->property_milesOW;
             }

            /**
             * setter for milesOW
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesCost_set_milesOW(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env,
                    const double  arg_milesOW)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
                
                if(_ClassMilesCost->is_valid_milesOW &&
                        arg_milesOW == _ClassMilesCost->property_milesOW)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesCost_reset_milesOW(_ClassMilesCost, env);

                _ClassMilesCost->property_milesOW = arg_milesOW;
                        _ClassMilesCost->is_valid_milesOW = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for milesOW
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_reset_milesOW(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
               

               _ClassMilesCost->is_valid_milesOW = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether milesOW is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesCost_is_milesOW_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_TRUE);
               
               return !_ClassMilesCost->is_valid_milesOW;
           }

           /**
            * Set milesOW to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_set_milesOW_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesCost_reset_milesOW(_ClassMilesCost, env);
           }

           

            /**
             * Getter for milesRT by  Property Number 9
             */
            double AXIS2_CALL
            adb_ClassMilesCost_get_property9(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env)
            {
                return adb_ClassMilesCost_get_milesRT(_ClassMilesCost,
                                             env);
            }

            /**
             * getter for milesRT.
             */
            double AXIS2_CALL
            adb_ClassMilesCost_get_milesRT(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (double)0);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, (double)0);
                  

                return _ClassMilesCost->property_milesRT;
             }

            /**
             * setter for milesRT
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesCost_set_milesRT(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env,
                    const double  arg_milesRT)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
                
                if(_ClassMilesCost->is_valid_milesRT &&
                        arg_milesRT == _ClassMilesCost->property_milesRT)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesCost_reset_milesRT(_ClassMilesCost, env);

                _ClassMilesCost->property_milesRT = arg_milesRT;
                        _ClassMilesCost->is_valid_milesRT = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for milesRT
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_reset_milesRT(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_FAILURE);
               

               _ClassMilesCost->is_valid_milesRT = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether milesRT is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesCost_is_milesRT_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesCost, AXIS2_TRUE);
               
               return !_ClassMilesCost->is_valid_milesRT;
           }

           /**
            * Set milesRT to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesCost_set_milesRT_nil(
                   adb_ClassMilesCost_t* _ClassMilesCost,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesCost_reset_milesRT(_ClassMilesCost, env);
           }

           

