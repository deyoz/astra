

        #ifndef ADB_PAYMILESREQUEST_H
        #define ADB_PAYMILESREQUEST_H

       /**
        * adb_PayMilesRequest.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_PayMilesRequest class
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
        

        typedef struct adb_PayMilesRequest adb_PayMilesRequest_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_PayMilesRequest_t
         * @param env pointer to environment struct
         * @return newly created adb_PayMilesRequest_t object
         */
        adb_PayMilesRequest_t* AXIS2_CALL
        adb_PayMilesRequest_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _PayMilesRequest adb_PayMilesRequest_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_free (
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env);

        /**
         * Free adb_PayMilesRequest_t object
         * @param  _PayMilesRequest adb_PayMilesRequest_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_free_obj (
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        /******** Deprecated for array types, Use 'Getters and Setters for Arrays' instead ***********/
        

        /**
         * Getter for ffpCardNumber. 
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_PayMilesRequest_get_ffpCardNumber(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for ffpCardNumber.
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_ffpCardNumber axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_set_ffpCardNumber(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_ffpCardNumber);

        /**
         * Resetter for ffpCardNumber
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_reset_ffpCardNumber(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for tickets. Deprecated for array types, Use adb_PayMilesRequest_get_tickets_at instead
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return Array of axis2_char_t*s.
         */
        axutil_array_list_t* AXIS2_CALL
        adb_PayMilesRequest_get_tickets(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for tickets.Deprecated for array types, Use adb_PayMilesRequest_set_tickets_at
         * or adb_PayMilesRequest_add_tickets instead.
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_tickets Array of axis2_char_t*s.
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_set_tickets(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env,
            axutil_array_list_t*  arg_tickets);

        /**
         * Resetter for tickets
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_reset_tickets(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for token. 
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_PayMilesRequest_get_token(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for token.
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_token int
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_set_token(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env,
            const int  arg_token);

        /**
         * Resetter for token
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_reset_token(
            adb_PayMilesRequest_t* _PayMilesRequest,
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
         * Get the ith element of tickets.
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param i index of the item to return
         * @return ith axis2_char_t* of the array
         */
        axis2_char_t* AXIS2_CALL
        adb_PayMilesRequest_get_tickets_at(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env, int i);

        /**
         * Set the ith element of tickets. (If the ith already exist, it will be replaced)
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param i index of the item to return
         * @param arg_tickets element to set axis2_char_t* to the array
         * @return ith axis2_char_t* of the array
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_set_tickets_at(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env, int i,
                const axis2_char_t* arg_tickets);


        /**
         * Add to tickets.
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_tickets element to add axis2_char_t* to the array
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_add_tickets(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env,
                const axis2_char_t* arg_tickets);

        /**
         * Get the size of the tickets array.
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct.
         * @return the size of the tickets array.
         */
        int AXIS2_CALL
        adb_PayMilesRequest_sizeof_tickets(
                    adb_PayMilesRequest_t* _PayMilesRequest,
                    const axutil_env_t *env);

        /**
         * Remove the ith element of tickets.
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param i index of the item to remove
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_remove_tickets_at(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env, int i);

        


        /******************************* Checking and Setting NIL values *********************************/
        /* Use 'Checking and Setting NIL values for Arrays' to check and set nil for individual elements */

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether ffpCardNumber is nill
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_PayMilesRequest_is_ffpCardNumber_nil(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set ffpCardNumber to nill (currently the same as reset)
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_set_ffpCardNumber_nil(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether tickets is nill
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_PayMilesRequest_is_tickets_nil(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set tickets to nill (currently the same as reset)
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_set_tickets_nil(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether token is nill
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_PayMilesRequest_is_token_nil(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set token to nill (currently the same as reset)
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_set_token_nil(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env);
        
        /*************************** Checking and Setting 'NIL' values in Arrays *****************************/

        /**
         * NOTE: You may set this to remove specific elements in the array
         *       But you can not remove elements, if the specific property is declared to be non-nillable or sizeof(array) < minOccurs
         */
        
        /**
         * Check whether tickets is nill at i
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct.
         * @param i index of the item to return.
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_PayMilesRequest_is_tickets_nil_at(
                adb_PayMilesRequest_t* _PayMilesRequest,
                const axutil_env_t *env, int i);
 
       
        /**
         * Set tickets to nill at i
         * @param  _PayMilesRequest _ adb_PayMilesRequest_t object
         * @param env pointer to environment struct.
         * @param i index of the item to set.
         * @return AXIS2_SUCCESS on success, or AXIS2_FAILURE otherwise.
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_set_tickets_nil_at(
                adb_PayMilesRequest_t* _PayMilesRequest, 
                const axutil_env_t *env, int i);

        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_deserialize(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_PayMilesRequest_deserialize_obj(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_PayMilesRequest_declare_parent_namespaces(
                    adb_PayMilesRequest_t* _PayMilesRequest,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param PayMilesRequest_om_node node to serialize from
         * @param PayMilesRequest_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_PayMilesRequest_serialize(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env,
            axiom_node_t* PayMilesRequest_om_node, axiom_element_t *PayMilesRequest_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @param PayMilesRequest_om_node node to serialize from
         * @param PayMilesRequest_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_PayMilesRequest_serialize_obj(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env,
            axiom_node_t* PayMilesRequest_om_node, axiom_element_t *PayMilesRequest_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_PayMilesRequest is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_PayMilesRequest_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_PayMilesRequest_t
         * @param env pointer to environment struct
         * @param _ffpCardNumber axis2_char_t*
         * @param _tickets axutil_array_list_t*
         * @param _token int
         * @return newly created adb_PayMilesRequest_t object
         */
        adb_PayMilesRequest_t* AXIS2_CALL
        adb_PayMilesRequest_create_with_values(
            const axutil_env_t *env,
                axis2_char_t* _ffpCardNumber,
                axutil_array_list_t* _tickets,
                int _token);

        


                /**
                 * Free adb_PayMilesRequest_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _PayMilesRequest adb_PayMilesRequest_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                axis2_char_t* AXIS2_CALL
                adb_PayMilesRequest_free_popping_value(
                        adb_PayMilesRequest_t* _PayMilesRequest,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for ffpCardNumber by property number (1)
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_PayMilesRequest_get_property1(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for tickets by property number (2)
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return Array of axis2_char_t*s.
         */
        axutil_array_list_t* AXIS2_CALL
        adb_PayMilesRequest_get_property2(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for token by property number (3)
         * @param  _PayMilesRequest adb_PayMilesRequest_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_PayMilesRequest_get_property3(
            adb_PayMilesRequest_t* _PayMilesRequest,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_PAYMILESREQUEST_H */
    

