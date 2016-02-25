

        /**
         * adb_ClassMilesIncome.c
         *
         * This file was auto-generated from WSDL
         * by the Apache Axis2/C version: SNAPSHOT  Built on : Mar 10, 2008 (08:35:52 GMT+00:00)
         */

        #include "adb_ClassMilesIncome.h"
        
                /*
                 * This type was generated from the piece of schema that had
                 * name = ClassMilesIncome
                 * Namespace URI = http://ffpServer.aeronavigator.ru/xsd
                 * Namespace Prefix = ns1
                 */
           


        struct adb_ClassMilesIncome
        {
            axis2_char_t *property_Type;

            axis2_char_t* property_arrival;

                
                axis2_bool_t is_valid_arrival;
            double property_base;

                
                axis2_bool_t is_valid_base;
            axis2_char_t* property_className;

                
                axis2_bool_t is_valid_className;
            int property_count;

                
                axis2_bool_t is_valid_count;
            axis2_char_t* property_departure;

                
                axis2_bool_t is_valid_departure;
            double property_factor;

                
                axis2_bool_t is_valid_factor;
            double property_miles;

                
                axis2_bool_t is_valid_miles;
            double property_statCoef;

                
                axis2_bool_t is_valid_statCoef;
            double property_typeCoef;

                
                axis2_bool_t is_valid_typeCoef;
            
        };


       /************************* Private Function prototypes ********************************/
        


       /************************* Function Implmentations ********************************/
        adb_ClassMilesIncome_t* AXIS2_CALL
        adb_ClassMilesIncome_create(
            const axutil_env_t *env)
        {
            adb_ClassMilesIncome_t *_ClassMilesIncome = NULL;
            
            AXIS2_ENV_CHECK(env, NULL);

            _ClassMilesIncome = (adb_ClassMilesIncome_t *) AXIS2_MALLOC(env->
                allocator, sizeof(adb_ClassMilesIncome_t));

            if(NULL == _ClassMilesIncome)
            {
                AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
                return NULL;
            }

            memset(_ClassMilesIncome, 0, sizeof(adb_ClassMilesIncome_t));

            _ClassMilesIncome->property_Type = axutil_strdup(env, "adb_ClassMilesIncome");
            _ClassMilesIncome->property_arrival  = NULL;
                  _ClassMilesIncome->is_valid_arrival  = AXIS2_FALSE;
            _ClassMilesIncome->is_valid_base  = AXIS2_FALSE;
            _ClassMilesIncome->property_className  = NULL;
                  _ClassMilesIncome->is_valid_className  = AXIS2_FALSE;
            _ClassMilesIncome->is_valid_count  = AXIS2_FALSE;
            _ClassMilesIncome->property_departure  = NULL;
                  _ClassMilesIncome->is_valid_departure  = AXIS2_FALSE;
            _ClassMilesIncome->is_valid_factor  = AXIS2_FALSE;
            _ClassMilesIncome->is_valid_miles  = AXIS2_FALSE;
            _ClassMilesIncome->is_valid_statCoef  = AXIS2_FALSE;
            _ClassMilesIncome->is_valid_typeCoef  = AXIS2_FALSE;
            

            return _ClassMilesIncome;
        }

        adb_ClassMilesIncome_t* AXIS2_CALL
        adb_ClassMilesIncome_create_with_values(
            const axutil_env_t *env,
                axis2_char_t* _arrival,
                double _base,
                axis2_char_t* _className,
                int _count,
                axis2_char_t* _departure,
                double _factor,
                double _miles,
                double _statCoef,
                double _typeCoef)
        {
            adb_ClassMilesIncome_t* adb_obj = NULL;
            axis2_status_t status = AXIS2_SUCCESS;

            adb_obj = adb_ClassMilesIncome_create(env);

            
              status = adb_ClassMilesIncome_set_arrival(
                                     adb_obj,
                                     env,
                                     _arrival);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesIncome_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesIncome_set_base(
                                     adb_obj,
                                     env,
                                     _base);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesIncome_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesIncome_set_className(
                                     adb_obj,
                                     env,
                                     _className);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesIncome_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesIncome_set_count(
                                     adb_obj,
                                     env,
                                     _count);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesIncome_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesIncome_set_departure(
                                     adb_obj,
                                     env,
                                     _departure);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesIncome_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesIncome_set_factor(
                                     adb_obj,
                                     env,
                                     _factor);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesIncome_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesIncome_set_miles(
                                     adb_obj,
                                     env,
                                     _miles);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesIncome_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesIncome_set_statCoef(
                                     adb_obj,
                                     env,
                                     _statCoef);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesIncome_free (adb_obj, env);
                  return NULL;
              }
            
              status = adb_ClassMilesIncome_set_typeCoef(
                                     adb_obj,
                                     env,
                                     _typeCoef);
              if(status == AXIS2_FAILURE) {
                  adb_ClassMilesIncome_free (adb_obj, env);
                  return NULL;
              }
            

            return adb_obj;
        }
      
        axis2_char_t* AXIS2_CALL
                adb_ClassMilesIncome_free_popping_value(
                        adb_ClassMilesIncome_t* _ClassMilesIncome,
                        const axutil_env_t *env)
                {
                    axis2_char_t* value;

                    
                    
                    value = _ClassMilesIncome->property_arrival;

                    _ClassMilesIncome->property_arrival = (axis2_char_t*)NULL;
                    adb_ClassMilesIncome_free(_ClassMilesIncome, env);

                    return value;
                }
            

        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_free(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
        {
            
            
            return axis2_extension_mapper_free(
                (adb_type_t*) _ClassMilesIncome,
                env,
                "adb_ClassMilesIncome");
            
        }

        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_free_obj(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
        {
            

            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);

            if (_ClassMilesIncome->property_Type != NULL)
            {
              AXIS2_FREE(env->allocator, _ClassMilesIncome->property_Type);
            }

            adb_ClassMilesIncome_reset_arrival(_ClassMilesIncome, env);
            adb_ClassMilesIncome_reset_base(_ClassMilesIncome, env);
            adb_ClassMilesIncome_reset_className(_ClassMilesIncome, env);
            adb_ClassMilesIncome_reset_count(_ClassMilesIncome, env);
            adb_ClassMilesIncome_reset_departure(_ClassMilesIncome, env);
            adb_ClassMilesIncome_reset_factor(_ClassMilesIncome, env);
            adb_ClassMilesIncome_reset_miles(_ClassMilesIncome, env);
            adb_ClassMilesIncome_reset_statCoef(_ClassMilesIncome, env);
            adb_ClassMilesIncome_reset_typeCoef(_ClassMilesIncome, env);
            

            if(_ClassMilesIncome)
            {
                AXIS2_FREE(env->allocator, _ClassMilesIncome);
                _ClassMilesIncome = NULL;
            }

            return AXIS2_SUCCESS;
        }


        

        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_deserialize(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env,
                axiom_node_t **dp_parent,
                axis2_bool_t *dp_is_early_node_valid,
                axis2_bool_t dont_care_minoccurs)
        {
            
            
            return axis2_extension_mapper_deserialize(
                (adb_type_t*) _ClassMilesIncome,
                env,
                dp_parent,
                dp_is_early_node_valid,
                dont_care_minoccurs,
                "adb_ClassMilesIncome");
            
        }

        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_deserialize_obj(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
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
            AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);

            
              
              while(parent && axiom_node_get_node_type(parent, env) != AXIOM_ELEMENT)
              {
                  parent = axiom_node_get_next_sibling(parent, env);
              }
              if (NULL == parent)
              {
                /* This should be checked before everything */
                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                            "Failed in building adb object for ClassMilesIncome : "
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
                                            status = adb_ClassMilesIncome_set_arrival(_ClassMilesIncome, env,
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
                      * building base element
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
                                 
                                 element_qname = axutil_qname_create(env, "base", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesIncome_set_base(_ClassMilesIncome, env,
                                                                   atof(text_value));
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for base ");
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
                                            status = adb_ClassMilesIncome_set_className(_ClassMilesIncome, env,
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
                                            status = adb_ClassMilesIncome_set_count(_ClassMilesIncome, env,
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
                                            status = adb_ClassMilesIncome_set_departure(_ClassMilesIncome, env,
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
                      * building factor element
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
                                 
                                 element_qname = axutil_qname_create(env, "factor", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesIncome_set_factor(_ClassMilesIncome, env,
                                                                   atof(text_value));
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for factor ");
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
                      * building miles element
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
                                 
                                 element_qname = axutil_qname_create(env, "miles", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesIncome_set_miles(_ClassMilesIncome, env,
                                                                   atof(text_value));
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for miles ");
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
                      * building statCoef element
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
                                 
                                 element_qname = axutil_qname_create(env, "statCoef", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesIncome_set_statCoef(_ClassMilesIncome, env,
                                                                   atof(text_value));
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for statCoef ");
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
                      * building typeCoef element
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
                                 
                                 element_qname = axutil_qname_create(env, "typeCoef", "http://ffpServer.aeronavigator.ru/xsd", NULL);
                                 

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
                                            status = adb_ClassMilesIncome_set_typeCoef(_ClassMilesIncome, env,
                                                                   atof(text_value));
                                      }
                                      
                                 if(AXIS2_FAILURE ==  status)
                                 {
                                     AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "failed in setting the value for typeCoef ");
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
          adb_ClassMilesIncome_is_particle()
          {
            
                 return AXIS2_FALSE;
              
          }


          void AXIS2_CALL
          adb_ClassMilesIncome_declare_parent_namespaces(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index)
          {
            
                  /* Here this is an empty function, Nothing to declare */
                 
          }

        
        
        axiom_node_t* AXIS2_CALL
        adb_ClassMilesIncome_serialize(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env, axiom_node_t *parent, axiom_element_t *parent_element, int parent_tag_closed, axutil_hash_t *namespaces, int *next_ns_index)
        {
            
            
            if (_ClassMilesIncome == NULL)
            {
                return adb_ClassMilesIncome_serialize_obj(
                    _ClassMilesIncome, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index);
            }
            else
            {
                return axis2_extension_mapper_serialize(
                    (adb_type_t*) _ClassMilesIncome, env, parent, parent_element, parent_tag_closed, namespaces, next_ns_index, "adb_ClassMilesIncome");
            }
            
        }

        axiom_node_t* AXIS2_CALL
        adb_ClassMilesIncome_serialize_obj(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
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
                    
                    axis2_char_t text_value_4[ADB_DEFAULT_DIGIT_LIMIT];
                    
                    axis2_char_t *text_value_5;
                    axis2_char_t *text_value_5_temp;
                    
                    axis2_char_t text_value_6[ADB_DEFAULT_DIGIT_LIMIT];
                    
                    axis2_char_t text_value_7[ADB_DEFAULT_DIGIT_LIMIT];
                    
                    axis2_char_t text_value_8[ADB_DEFAULT_DIGIT_LIMIT];
                    
                    axis2_char_t text_value_9[ADB_DEFAULT_DIGIT_LIMIT];
                    
               axis2_char_t *start_input_str = NULL;
               axis2_char_t *end_input_str = NULL;
               unsigned int start_input_str_len = 0;
               unsigned int end_input_str_len = 0;
            
            
               axiom_data_source_t *data_source = NULL;
               axutil_stream_t *stream = NULL;

            

            AXIS2_ENV_CHECK(env, NULL);
            AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, NULL);
            
            
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
              type_attrib = axutil_strcat(env, " ", xsi_prefix, ":type=\"ClassMilesIncome\"", NULL);
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
            xsi_type_attri = axiom_attribute_create (env, "type", "ClassMilesIncome", xsi_ns);
            
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
                      

                   if (!_ClassMilesIncome->is_valid_arrival)
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
                    
                           text_value_1 = _ClassMilesIncome->property_arrival;
                           
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
                      

                   if (!_ClassMilesIncome->is_valid_base)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("base"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("base")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing base element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sbase>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sbase>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                               sprintf (text_value_2, "%f", (double)_ClassMilesIncome->property_base);
                             
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
                      

                   if (!_ClassMilesIncome->is_valid_className)
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
                    
                           text_value_3 = _ClassMilesIncome->property_className;
                           
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
                      

                   if (!_ClassMilesIncome->is_valid_count)
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
                    
                               sprintf (text_value_4, AXIS2_PRINTF_INT32_FORMAT_SPECIFIER, _ClassMilesIncome->property_count);
                             
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                           axutil_stream_write(stream, env, text_value_4, axutil_strlen(text_value_4));
                           
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
                      

                   if (!_ClassMilesIncome->is_valid_departure)
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
                    
                           text_value_5 = _ClassMilesIncome->property_departure;
                           
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
                      

                   if (!_ClassMilesIncome->is_valid_factor)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("factor"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("factor")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing factor element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sfactor>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sfactor>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                               sprintf (text_value_6, "%f", (double)_ClassMilesIncome->property_factor);
                             
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                           axutil_stream_write(stream, env, text_value_6, axutil_strlen(text_value_6));
                           
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
                      

                   if (!_ClassMilesIncome->is_valid_miles)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("miles"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("miles")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing miles element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%smiles>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%smiles>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                               sprintf (text_value_7, "%f", (double)_ClassMilesIncome->property_miles);
                             
                           axutil_stream_write(stream, env, start_input_str, start_input_str_len);
                           
                           axutil_stream_write(stream, env, text_value_7, axutil_strlen(text_value_7));
                           
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
                      

                   if (!_ClassMilesIncome->is_valid_statCoef)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("statCoef"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("statCoef")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing statCoef element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%sstatCoef>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%sstatCoef>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                               sprintf (text_value_8, "%f", (double)_ClassMilesIncome->property_statCoef);
                             
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
                      

                   if (!_ClassMilesIncome->is_valid_typeCoef)
                   {
                      
                           /* no need to complain for minoccurs=0 element */
                            
                          
                   }
                   else
                   {
                     start_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (4 + axutil_strlen(p_prefix) + 
                                  axutil_strlen("typeCoef"))); 
                                 
                                 /* axutil_strlen("<:>") + 1 = 4 */
                     end_input_str = (axis2_char_t*)AXIS2_MALLOC(env->allocator, sizeof(axis2_char_t) *
                                 (5 + axutil_strlen(p_prefix) + axutil_strlen("typeCoef")));
                                  /* axutil_strlen("</:>") + 1 = 5 */
                                  
                     

                   
                   
                     
                     /*
                      * parsing typeCoef element
                      */

                    
                    
                            sprintf(start_input_str, "<%s%stypeCoef>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                            
                        start_input_str_len = axutil_strlen(start_input_str);
                        sprintf(end_input_str, "</%s%stypeCoef>",
                                 p_prefix?p_prefix:"",
                                 (p_prefix && axutil_strcmp(p_prefix, ""))?":":"");
                        end_input_str_len = axutil_strlen(end_input_str);
                    
                               sprintf (text_value_9, "%f", (double)_ClassMilesIncome->property_typeCoef);
                             
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
            adb_ClassMilesIncome_get_property1(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
            {
                return adb_ClassMilesIncome_get_arrival(_ClassMilesIncome,
                                             env);
            }

            /**
             * getter for arrival.
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesIncome_get_arrival(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, NULL);
                  

                return _ClassMilesIncome->property_arrival;
             }

            /**
             * setter for arrival
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesIncome_set_arrival(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_arrival)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
                
                if(_ClassMilesIncome->is_valid_arrival &&
                        arg_arrival == _ClassMilesIncome->property_arrival)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesIncome_reset_arrival(_ClassMilesIncome, env);

                
                if(NULL == arg_arrival)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ClassMilesIncome->property_arrival = (axis2_char_t *)axutil_strdup(env, arg_arrival);
                        if(NULL == _ClassMilesIncome->property_arrival)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for arrival");
                            return AXIS2_FAILURE;
                        }
                        _ClassMilesIncome->is_valid_arrival = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for arrival
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_reset_arrival(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
               

               
            
                
                if(_ClassMilesIncome->property_arrival != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _ClassMilesIncome->property_arrival);
                     _ClassMilesIncome->property_arrival = NULL;
                }
            
                
                
                _ClassMilesIncome->is_valid_arrival = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether arrival is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesIncome_is_arrival_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_TRUE);
               
               return !_ClassMilesIncome->is_valid_arrival;
           }

           /**
            * Set arrival to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_set_arrival_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesIncome_reset_arrival(_ClassMilesIncome, env);
           }

           

            /**
             * Getter for base by  Property Number 2
             */
            double AXIS2_CALL
            adb_ClassMilesIncome_get_property2(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
            {
                return adb_ClassMilesIncome_get_base(_ClassMilesIncome,
                                             env);
            }

            /**
             * getter for base.
             */
            double AXIS2_CALL
            adb_ClassMilesIncome_get_base(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (double)0);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, (double)0);
                  

                return _ClassMilesIncome->property_base;
             }

            /**
             * setter for base
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesIncome_set_base(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env,
                    const double  arg_base)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
                
                if(_ClassMilesIncome->is_valid_base &&
                        arg_base == _ClassMilesIncome->property_base)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesIncome_reset_base(_ClassMilesIncome, env);

                _ClassMilesIncome->property_base = arg_base;
                        _ClassMilesIncome->is_valid_base = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for base
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_reset_base(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
               

               _ClassMilesIncome->is_valid_base = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether base is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesIncome_is_base_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_TRUE);
               
               return !_ClassMilesIncome->is_valid_base;
           }

           /**
            * Set base to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_set_base_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesIncome_reset_base(_ClassMilesIncome, env);
           }

           

            /**
             * Getter for className by  Property Number 3
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesIncome_get_property3(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
            {
                return adb_ClassMilesIncome_get_className(_ClassMilesIncome,
                                             env);
            }

            /**
             * getter for className.
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesIncome_get_className(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, NULL);
                  

                return _ClassMilesIncome->property_className;
             }

            /**
             * setter for className
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesIncome_set_className(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_className)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
                
                if(_ClassMilesIncome->is_valid_className &&
                        arg_className == _ClassMilesIncome->property_className)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesIncome_reset_className(_ClassMilesIncome, env);

                
                if(NULL == arg_className)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ClassMilesIncome->property_className = (axis2_char_t *)axutil_strdup(env, arg_className);
                        if(NULL == _ClassMilesIncome->property_className)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for className");
                            return AXIS2_FAILURE;
                        }
                        _ClassMilesIncome->is_valid_className = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for className
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_reset_className(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
               

               
            
                
                if(_ClassMilesIncome->property_className != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _ClassMilesIncome->property_className);
                     _ClassMilesIncome->property_className = NULL;
                }
            
                
                
                _ClassMilesIncome->is_valid_className = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether className is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesIncome_is_className_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_TRUE);
               
               return !_ClassMilesIncome->is_valid_className;
           }

           /**
            * Set className to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_set_className_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesIncome_reset_className(_ClassMilesIncome, env);
           }

           

            /**
             * Getter for count by  Property Number 4
             */
            int AXIS2_CALL
            adb_ClassMilesIncome_get_property4(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
            {
                return adb_ClassMilesIncome_get_count(_ClassMilesIncome,
                                             env);
            }

            /**
             * getter for count.
             */
            int AXIS2_CALL
            adb_ClassMilesIncome_get_count(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (int)0);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, (int)0);
                  

                return _ClassMilesIncome->property_count;
             }

            /**
             * setter for count
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesIncome_set_count(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env,
                    const int  arg_count)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
                
                if(_ClassMilesIncome->is_valid_count &&
                        arg_count == _ClassMilesIncome->property_count)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesIncome_reset_count(_ClassMilesIncome, env);

                _ClassMilesIncome->property_count = arg_count;
                        _ClassMilesIncome->is_valid_count = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for count
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_reset_count(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
               

               _ClassMilesIncome->is_valid_count = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether count is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesIncome_is_count_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_TRUE);
               
               return !_ClassMilesIncome->is_valid_count;
           }

           /**
            * Set count to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_set_count_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesIncome_reset_count(_ClassMilesIncome, env);
           }

           

            /**
             * Getter for departure by  Property Number 5
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesIncome_get_property5(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
            {
                return adb_ClassMilesIncome_get_departure(_ClassMilesIncome,
                                             env);
            }

            /**
             * getter for departure.
             */
            axis2_char_t* AXIS2_CALL
            adb_ClassMilesIncome_get_departure(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, NULL);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, NULL);
                  

                return _ClassMilesIncome->property_departure;
             }

            /**
             * setter for departure
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesIncome_set_departure(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env,
                    const axis2_char_t*  arg_departure)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
                
                if(_ClassMilesIncome->is_valid_departure &&
                        arg_departure == _ClassMilesIncome->property_departure)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesIncome_reset_departure(_ClassMilesIncome, env);

                
                if(NULL == arg_departure)
                {
                    /* We are already done */
                    return AXIS2_SUCCESS;
                }
                _ClassMilesIncome->property_departure = (axis2_char_t *)axutil_strdup(env, arg_departure);
                        if(NULL == _ClassMilesIncome->property_departure)
                        {
                            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Error allocating memeory for departure");
                            return AXIS2_FAILURE;
                        }
                        _ClassMilesIncome->is_valid_departure = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for departure
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_reset_departure(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
               

               
            
                
                if(_ClassMilesIncome->property_departure != NULL)
                {
                   
                   
                        AXIS2_FREE(env-> allocator, _ClassMilesIncome->property_departure);
                     _ClassMilesIncome->property_departure = NULL;
                }
            
                
                
                _ClassMilesIncome->is_valid_departure = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether departure is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesIncome_is_departure_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_TRUE);
               
               return !_ClassMilesIncome->is_valid_departure;
           }

           /**
            * Set departure to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_set_departure_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesIncome_reset_departure(_ClassMilesIncome, env);
           }

           

            /**
             * Getter for factor by  Property Number 6
             */
            double AXIS2_CALL
            adb_ClassMilesIncome_get_property6(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
            {
                return adb_ClassMilesIncome_get_factor(_ClassMilesIncome,
                                             env);
            }

            /**
             * getter for factor.
             */
            double AXIS2_CALL
            adb_ClassMilesIncome_get_factor(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (double)0);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, (double)0);
                  

                return _ClassMilesIncome->property_factor;
             }

            /**
             * setter for factor
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesIncome_set_factor(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env,
                    const double  arg_factor)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
                
                if(_ClassMilesIncome->is_valid_factor &&
                        arg_factor == _ClassMilesIncome->property_factor)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesIncome_reset_factor(_ClassMilesIncome, env);

                _ClassMilesIncome->property_factor = arg_factor;
                        _ClassMilesIncome->is_valid_factor = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for factor
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_reset_factor(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
               

               _ClassMilesIncome->is_valid_factor = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether factor is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesIncome_is_factor_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_TRUE);
               
               return !_ClassMilesIncome->is_valid_factor;
           }

           /**
            * Set factor to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_set_factor_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesIncome_reset_factor(_ClassMilesIncome, env);
           }

           

            /**
             * Getter for miles by  Property Number 7
             */
            double AXIS2_CALL
            adb_ClassMilesIncome_get_property7(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
            {
                return adb_ClassMilesIncome_get_miles(_ClassMilesIncome,
                                             env);
            }

            /**
             * getter for miles.
             */
            double AXIS2_CALL
            adb_ClassMilesIncome_get_miles(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (double)0);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, (double)0);
                  

                return _ClassMilesIncome->property_miles;
             }

            /**
             * setter for miles
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesIncome_set_miles(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env,
                    const double  arg_miles)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
                
                if(_ClassMilesIncome->is_valid_miles &&
                        arg_miles == _ClassMilesIncome->property_miles)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesIncome_reset_miles(_ClassMilesIncome, env);

                _ClassMilesIncome->property_miles = arg_miles;
                        _ClassMilesIncome->is_valid_miles = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for miles
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_reset_miles(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
               

               _ClassMilesIncome->is_valid_miles = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether miles is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesIncome_is_miles_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_TRUE);
               
               return !_ClassMilesIncome->is_valid_miles;
           }

           /**
            * Set miles to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_set_miles_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesIncome_reset_miles(_ClassMilesIncome, env);
           }

           

            /**
             * Getter for statCoef by  Property Number 8
             */
            double AXIS2_CALL
            adb_ClassMilesIncome_get_property8(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
            {
                return adb_ClassMilesIncome_get_statCoef(_ClassMilesIncome,
                                             env);
            }

            /**
             * getter for statCoef.
             */
            double AXIS2_CALL
            adb_ClassMilesIncome_get_statCoef(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (double)0);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, (double)0);
                  

                return _ClassMilesIncome->property_statCoef;
             }

            /**
             * setter for statCoef
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesIncome_set_statCoef(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env,
                    const double  arg_statCoef)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
                
                if(_ClassMilesIncome->is_valid_statCoef &&
                        arg_statCoef == _ClassMilesIncome->property_statCoef)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesIncome_reset_statCoef(_ClassMilesIncome, env);

                _ClassMilesIncome->property_statCoef = arg_statCoef;
                        _ClassMilesIncome->is_valid_statCoef = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for statCoef
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_reset_statCoef(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
               

               _ClassMilesIncome->is_valid_statCoef = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether statCoef is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesIncome_is_statCoef_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_TRUE);
               
               return !_ClassMilesIncome->is_valid_statCoef;
           }

           /**
            * Set statCoef to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_set_statCoef_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesIncome_reset_statCoef(_ClassMilesIncome, env);
           }

           

            /**
             * Getter for typeCoef by  Property Number 9
             */
            double AXIS2_CALL
            adb_ClassMilesIncome_get_property9(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env)
            {
                return adb_ClassMilesIncome_get_typeCoef(_ClassMilesIncome,
                                             env);
            }

            /**
             * getter for typeCoef.
             */
            double AXIS2_CALL
            adb_ClassMilesIncome_get_typeCoef(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env)
             {
                
                    AXIS2_ENV_CHECK(env, (double)0);
                    AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, (double)0);
                  

                return _ClassMilesIncome->property_typeCoef;
             }

            /**
             * setter for typeCoef
             */
            axis2_status_t AXIS2_CALL
            adb_ClassMilesIncome_set_typeCoef(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env,
                    const double  arg_typeCoef)
             {
                

                AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
                AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
                
                if(_ClassMilesIncome->is_valid_typeCoef &&
                        arg_typeCoef == _ClassMilesIncome->property_typeCoef)
                {
                    
                    return AXIS2_SUCCESS; 
                }

                adb_ClassMilesIncome_reset_typeCoef(_ClassMilesIncome, env);

                _ClassMilesIncome->property_typeCoef = arg_typeCoef;
                        _ClassMilesIncome->is_valid_typeCoef = AXIS2_TRUE;
                    
                return AXIS2_SUCCESS;
             }

             

           /**
            * resetter for typeCoef
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_reset_typeCoef(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               int i = 0;
               int count = 0;
               void *element = NULL;

               AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_FAILURE);
               

               _ClassMilesIncome->is_valid_typeCoef = AXIS2_FALSE; 
               return AXIS2_SUCCESS;
           }

           /**
            * Check whether typeCoef is nill
            */
           axis2_bool_t AXIS2_CALL
           adb_ClassMilesIncome_is_typeCoef_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               AXIS2_ENV_CHECK(env, AXIS2_TRUE);
               AXIS2_PARAM_CHECK(env->error, _ClassMilesIncome, AXIS2_TRUE);
               
               return !_ClassMilesIncome->is_valid_typeCoef;
           }

           /**
            * Set typeCoef to nill (currently the same as reset)
            */
           axis2_status_t AXIS2_CALL
           adb_ClassMilesIncome_set_typeCoef_nil(
                   adb_ClassMilesIncome_t* _ClassMilesIncome,
                   const axutil_env_t *env)
           {
               return adb_ClassMilesIncome_reset_typeCoef(_ClassMilesIncome, env);
           }

           

