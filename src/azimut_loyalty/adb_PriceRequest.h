

        #ifndef ADB_PRICEREQUEST_H
        #define ADB_PRICEREQUEST_H

       /**
        * adb_PriceRequest.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_PriceRequest class
        */

        

        #include <stdio.h>
        #include <axiom.h>
        #include <axis2_util.h>
        #include <axiom_soap.h>
        #include <axis2_client.h>

        #include "axis2_extension_mapper.h"

        #ifdef __cplusplus
        extern "C"
        {
        #endif

        #define ADB_DEFAULT_DIGIT_LIMIT 1024
        #define ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT 64
        

        typedef struct adb_PriceRequest adb_PriceRequest_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_PriceRequest_t
         * @param env pointer to environment struct
         * @return newly created adb_PriceRequest_t object
         */
        adb_PriceRequest_t* AXIS2_CALL
        adb_PriceRequest_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _PriceRequest adb_PriceRequest_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_free (
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        /**
         * Free adb_PriceRequest_t object
         * @param  _PriceRequest adb_PriceRequest_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_free_obj (
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        
        

        /**
         * Getter for arrival. 
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_PriceRequest_get_arrival(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        /**
         * Setter for arrival.
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @param arg_arrival axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_set_arrival(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_arrival);

        /**
         * Resetter for arrival
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_reset_arrival(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for baseClass. 
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_PriceRequest_get_baseClass(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        /**
         * Setter for baseClass.
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @param arg_baseClass axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_set_baseClass(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_baseClass);

        /**
         * Resetter for baseClass
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_reset_baseClass(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for count. 
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_PriceRequest_get_count(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        /**
         * Setter for count.
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @param arg_count int
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_set_count(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env,
            const int  arg_count);

        /**
         * Resetter for count
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_reset_count(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for departure. 
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_PriceRequest_get_departure(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        /**
         * Setter for departure.
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @param arg_departure axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_set_departure(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_departure);

        /**
         * Resetter for departure
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_reset_departure(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for ffpCardNumber. 
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_PriceRequest_get_ffpCardNumber(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        /**
         * Setter for ffpCardNumber.
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @param arg_ffpCardNumber axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_set_ffpCardNumber(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_ffpCardNumber);

        /**
         * Resetter for ffpCardNumber
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_reset_ffpCardNumber(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

        


        /******************************* Checking and Setting NIL values *********************************/
        

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether arrival is nill
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_PriceRequest_is_arrival_nil(
                adb_PriceRequest_t* _PriceRequest,
                const axutil_env_t *env);


        
        /**
         * Set arrival to nill (currently the same as reset)
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_set_arrival_nil(
                adb_PriceRequest_t* _PriceRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether baseClass is nill
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_PriceRequest_is_baseClass_nil(
                adb_PriceRequest_t* _PriceRequest,
                const axutil_env_t *env);


        
        /**
         * Set baseClass to nill (currently the same as reset)
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_set_baseClass_nil(
                adb_PriceRequest_t* _PriceRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether count is nill
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_PriceRequest_is_count_nil(
                adb_PriceRequest_t* _PriceRequest,
                const axutil_env_t *env);


        
        /**
         * Set count to nill (currently the same as reset)
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_set_count_nil(
                adb_PriceRequest_t* _PriceRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether departure is nill
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_PriceRequest_is_departure_nil(
                adb_PriceRequest_t* _PriceRequest,
                const axutil_env_t *env);


        
        /**
         * Set departure to nill (currently the same as reset)
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_set_departure_nil(
                adb_PriceRequest_t* _PriceRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether ffpCardNumber is nill
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_PriceRequest_is_ffpCardNumber_nil(
                adb_PriceRequest_t* _PriceRequest,
                const axutil_env_t *env);


        
        /**
         * Set ffpCardNumber to nill (currently the same as reset)
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_set_ffpCardNumber_nil(
                adb_PriceRequest_t* _PriceRequest,
                const axutil_env_t *env);
        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_deserialize(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PriceRequest_deserialize_obj(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_PriceRequest_declare_parent_namespaces(
                    adb_PriceRequest_t* _PriceRequest,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @param PriceRequest_om_node node to serialize from
         * @param PriceRequest_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_PriceRequest_serialize(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env,
            axiom_node_t* PriceRequest_om_node, axiom_element_t *PriceRequest_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @param PriceRequest_om_node node to serialize from
         * @param PriceRequest_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_PriceRequest_serialize_obj(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env,
            axiom_node_t* PriceRequest_om_node, axiom_element_t *PriceRequest_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_PriceRequest is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_PriceRequest_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_PriceRequest_t
         * @param env pointer to environment struct
         * @param _arrival axis2_char_t*
         * @param _baseClass axis2_char_t*
         * @param _count int
         * @param _departure axis2_char_t*
         * @param _ffpCardNumber axis2_char_t*
         * @return newly created adb_PriceRequest_t object
         */
        adb_PriceRequest_t* AXIS2_CALL
        adb_PriceRequest_create_with_values(
            const axutil_env_t *env,
                axis2_char_t* _arrival,
                axis2_char_t* _baseClass,
                int _count,
                axis2_char_t* _departure,
                axis2_char_t* _ffpCardNumber);

        


                /**
                 * Free adb_PriceRequest_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _PriceRequest adb_PriceRequest_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                axis2_char_t* AXIS2_CALL
                adb_PriceRequest_free_popping_value(
                        adb_PriceRequest_t* _PriceRequest,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for arrival by property number (1)
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_PriceRequest_get_property1(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for baseClass by property number (2)
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_PriceRequest_get_property2(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for count by property number (3)
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_PriceRequest_get_property3(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for departure by property number (4)
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_PriceRequest_get_property4(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for ffpCardNumber by property number (5)
         * @param  _PriceRequest adb_PriceRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_PriceRequest_get_property5(
            adb_PriceRequest_t* _PriceRequest,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_PRICEREQUEST_H */
    

