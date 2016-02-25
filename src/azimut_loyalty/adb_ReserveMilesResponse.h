

        #ifndef ADB_RESERVEMILESRESPONSE_H
        #define ADB_RESERVEMILESRESPONSE_H

       /**
        * adb_ReserveMilesResponse.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_ReserveMilesResponse class
        */

        
          #include "adb_ErrorGroup.h"
          

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
        

        typedef struct adb_ReserveMilesResponse adb_ReserveMilesResponse_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_ReserveMilesResponse_t
         * @param env pointer to environment struct
         * @return newly created adb_ReserveMilesResponse_t object
         */
        adb_ReserveMilesResponse_t* AXIS2_CALL
        adb_ReserveMilesResponse_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_free (
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);

        /**
         * Free adb_ReserveMilesResponse_t object
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_free_obj (
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        /******** Deprecated for array types, Use 'Getters and Setters for Arrays' instead ***********/
        

        /**
         * Getter for errors. Deprecated for array types, Use adb_ReserveMilesResponse_get_errors_at instead
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return Array of adb_ErrorGroup_t*s.
         */
        axutil_array_list_t* AXIS2_CALL
        adb_ReserveMilesResponse_get_errors(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);

        /**
         * Setter for errors.Deprecated for array types, Use adb_ReserveMilesResponse_set_errors_at
         * or adb_ReserveMilesResponse_add_errors instead.
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param arg_errors Array of adb_ErrorGroup_t*s.
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_set_errors(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env,
            axutil_array_list_t*  arg_errors);

        /**
         * Resetter for errors
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_reset_errors(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for dateValidBeforeUtc. 
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesResponse_get_dateValidBeforeUtc(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);

        /**
         * Setter for dateValidBeforeUtc.
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param arg_dateValidBeforeUtc axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_set_dateValidBeforeUtc(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env,
            const axis2_char_t*  arg_dateValidBeforeUtc);

        /**
         * Resetter for dateValidBeforeUtc
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_reset_dateValidBeforeUtc(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for token. 
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_ReserveMilesResponse_get_token(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);

        /**
         * Setter for token.
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param arg_token int
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_set_token(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env,
            const int  arg_token);

        /**
         * Resetter for token
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_reset_token(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);

        
        /****************************** Getters and Setters For Arrays **********************************/
        /************ Array Specific Operations: get_at, set_at, add, remove_at, sizeof *****************/

        /**
         * E.g. use of get_at, set_at, add and sizeof
         *
         * for(i = 0; i < adb_element_sizeof_property(adb_object, env); i ++ )
         * {
         *     // Getting ith value to property_object variable
         *     property_object = adb_element_get_property_at(adb_object, env, i);
         *
         *     // Setting ith value from property_object variable
         *     adb_element_set_property_at(adb_object, env, i, property_object);
         *
         *     // Appending the value to the end of the array from property_object variable
         *     adb_element_add_property(adb_object, env, property_object);
         *
         *     // Removing the ith value from an array
         *     adb_element_remove_property_at(adb_object, env, i);
         *     
         * }
         *
         */

        
        
        /**
         * Get the ith element of errors.
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param i index of the item to return
         * @return ith adb_ErrorGroup_t* of the array
         */
        adb_ErrorGroup_t* AXIS2_CALL
        adb_ReserveMilesResponse_get_errors_at(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env, int i);

        /**
         * Set the ith element of errors. (If the ith already exist, it will be replaced)
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param i index of the item to return
         * @param arg_errors element to set adb_ErrorGroup_t* to the array
         * @return ith adb_ErrorGroup_t* of the array
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_set_errors_at(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env, int i,
                adb_ErrorGroup_t* arg_errors);


        /**
         * Add to errors.
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param arg_errors element to add adb_ErrorGroup_t* to the array
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_add_errors(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env,
                adb_ErrorGroup_t* arg_errors);

        /**
         * Get the size of the errors array.
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct.
         * @return the size of the errors array.
         */
        int AXIS2_CALL
        adb_ReserveMilesResponse_sizeof_errors(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env);

        /**
         * Remove the ith element of errors.
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param i index of the item to remove
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_remove_errors_at(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env, int i);

        


        /******************************* Checking and Setting NIL values *********************************/
        /* Use 'Checking and Setting NIL values for Arrays' to check and set nil for individual elements */

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether errors is nill
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesResponse_is_errors_nil(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env);


        
        /**
         * Set errors to nill (currently the same as reset)
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_set_errors_nil(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether dateValidBeforeUtc is nill
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesResponse_is_dateValidBeforeUtc_nil(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env);


        
        /**
         * Set dateValidBeforeUtc to nill (currently the same as reset)
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_set_dateValidBeforeUtc_nil(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether token is nill
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesResponse_is_token_nil(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env);


        
        /**
         * Set token to nill (currently the same as reset)
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_set_token_nil(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env);
        
        /*************************** Checking and Setting 'NIL' values in Arrays *****************************/

        /**
         * NOTE: You may set this to remove specific elements in the array
         *       But you can not remove elements, if the specific property is declared to be non-nillable or sizeof(array) < minOccurs
         */
        
        /**
         * Check whether errors is nill at i
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct.
         * @param i index of the item to return.
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesResponse_is_errors_nil_at(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                const axutil_env_t *env, int i);
 
       
        /**
         * Set errors to nill at i
         * @param  _ReserveMilesResponse _ adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct.
         * @param i index of the item to set.
         * @return AXIS2_SUCCESS on success, or AXIS2_FAILURE otherwise.
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_set_errors_nil_at(
                adb_ReserveMilesResponse_t* _ReserveMilesResponse, 
                const axutil_env_t *env, int i);

        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_deserialize(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesResponse_deserialize_obj(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_ReserveMilesResponse_declare_parent_namespaces(
                    adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param ReserveMilesResponse_om_node node to serialize from
         * @param ReserveMilesResponse_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_ReserveMilesResponse_serialize(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env,
            axiom_node_t* ReserveMilesResponse_om_node, axiom_element_t *ReserveMilesResponse_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @param ReserveMilesResponse_om_node node to serialize from
         * @param ReserveMilesResponse_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_ReserveMilesResponse_serialize_obj(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env,
            axiom_node_t* ReserveMilesResponse_om_node, axiom_element_t *ReserveMilesResponse_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_ReserveMilesResponse is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesResponse_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_ReserveMilesResponse_t
         * @param env pointer to environment struct
         * @param _errors axutil_array_list_t*
         * @param _dateValidBeforeUtc axis2_char_t*
         * @param _token int
         * @return newly created adb_ReserveMilesResponse_t object
         */
        adb_ReserveMilesResponse_t* AXIS2_CALL
        adb_ReserveMilesResponse_create_with_values(
            const axutil_env_t *env,
                axutil_array_list_t* _errors,
                axis2_char_t* _dateValidBeforeUtc,
                int _token);

        


                /**
                 * Free adb_ReserveMilesResponse_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                axutil_array_list_t* AXIS2_CALL
                adb_ReserveMilesResponse_free_popping_value(
                        adb_ReserveMilesResponse_t* _ReserveMilesResponse,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for errors by property number (1)
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return Array of adb_ErrorGroup_t*s.
         */
        axutil_array_list_t* AXIS2_CALL
        adb_ReserveMilesResponse_get_property1(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for dateValidBeforeUtc by property number (2)
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesResponse_get_property2(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for token by property number (3)
         * @param  _ReserveMilesResponse adb_ReserveMilesResponse_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_ReserveMilesResponse_get_property3(
            adb_ReserveMilesResponse_t* _ReserveMilesResponse,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_RESERVEMILESRESPONSE_H */
    

