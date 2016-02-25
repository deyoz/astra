

        #ifndef ADB_AVAILABLEMILESRESPONSE_H
        #define ADB_AVAILABLEMILESRESPONSE_H

       /**
        * adb_AvailableMilesResponse.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_AvailableMilesResponse class
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
        

        typedef struct adb_AvailableMilesResponse adb_AvailableMilesResponse_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_AvailableMilesResponse_t
         * @param env pointer to environment struct
         * @return newly created adb_AvailableMilesResponse_t object
         */
        adb_AvailableMilesResponse_t* AXIS2_CALL
        adb_AvailableMilesResponse_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_free (
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env);

        /**
         * Free adb_AvailableMilesResponse_t object
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_free_obj (
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        /******** Deprecated for array types, Use 'Getters and Setters for Arrays' instead ***********/
        

        /**
         * Getter for errors. Deprecated for array types, Use adb_AvailableMilesResponse_get_errors_at instead
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return Array of adb_ErrorGroup_t*s.
         */
        axutil_array_list_t* AXIS2_CALL
        adb_AvailableMilesResponse_get_errors(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env);

        /**
         * Setter for errors.Deprecated for array types, Use adb_AvailableMilesResponse_set_errors_at
         * or adb_AvailableMilesResponse_add_errors instead.
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param arg_errors Array of adb_ErrorGroup_t*s.
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_set_errors(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env,
            axutil_array_list_t*  arg_errors);

        /**
         * Resetter for errors
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_reset_errors(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for ffpCardNumber. 
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_AvailableMilesResponse_get_ffpCardNumber(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env);

        /**
         * Setter for ffpCardNumber.
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param arg_ffpCardNumber axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_set_ffpCardNumber(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env,
            const axis2_char_t*  arg_ffpCardNumber);

        /**
         * Resetter for ffpCardNumber
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_reset_ffpCardNumber(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for miles. 
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_AvailableMilesResponse_get_miles(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env);

        /**
         * Setter for miles.
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param arg_miles double
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_set_miles(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env,
            const double  arg_miles);

        /**
         * Resetter for miles
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_reset_miles(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
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
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param i index of the item to return
         * @return ith adb_ErrorGroup_t* of the array
         */
        adb_ErrorGroup_t* AXIS2_CALL
        adb_AvailableMilesResponse_get_errors_at(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env, int i);

        /**
         * Set the ith element of errors. (If the ith already exist, it will be replaced)
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param i index of the item to return
         * @param arg_errors element to set adb_ErrorGroup_t* to the array
         * @return ith adb_ErrorGroup_t* of the array
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_set_errors_at(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env, int i,
                adb_ErrorGroup_t* arg_errors);


        /**
         * Add to errors.
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param arg_errors element to add adb_ErrorGroup_t* to the array
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_add_errors(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env,
                adb_ErrorGroup_t* arg_errors);

        /**
         * Get the size of the errors array.
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct.
         * @return the size of the errors array.
         */
        int AXIS2_CALL
        adb_AvailableMilesResponse_sizeof_errors(
                    adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                    const axutil_env_t *env);

        /**
         * Remove the ith element of errors.
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param i index of the item to remove
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_remove_errors_at(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env, int i);

        


        /******************************* Checking and Setting NIL values *********************************/
        /* Use 'Checking and Setting NIL values for Arrays' to check and set nil for individual elements */

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether errors is nill
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_AvailableMilesResponse_is_errors_nil(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env);


        
        /**
         * Set errors to nill (currently the same as reset)
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_set_errors_nil(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether ffpCardNumber is nill
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_AvailableMilesResponse_is_ffpCardNumber_nil(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env);


        
        /**
         * Set ffpCardNumber to nill (currently the same as reset)
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_set_ffpCardNumber_nil(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether miles is nill
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_AvailableMilesResponse_is_miles_nil(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env);


        
        /**
         * Set miles to nill (currently the same as reset)
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_set_miles_nil(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env);
        
        /*************************** Checking and Setting 'NIL' values in Arrays *****************************/

        /**
         * NOTE: You may set this to remove specific elements in the array
         *       But you can not remove elements, if the specific property is declared to be non-nillable or sizeof(array) < minOccurs
         */
        
        /**
         * Check whether errors is nill at i
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct.
         * @param i index of the item to return.
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_AvailableMilesResponse_is_errors_nil_at(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                const axutil_env_t *env, int i);
 
       
        /**
         * Set errors to nill at i
         * @param  _AvailableMilesResponse _ adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct.
         * @param i index of the item to set.
         * @return AXIS2_SUCCESS on success, or AXIS2_FAILURE otherwise.
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_set_errors_nil_at(
                adb_AvailableMilesResponse_t* _AvailableMilesResponse, 
                const axutil_env_t *env, int i);

        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_deserialize(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_AvailableMilesResponse_deserialize_obj(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_AvailableMilesResponse_declare_parent_namespaces(
                    adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param AvailableMilesResponse_om_node node to serialize from
         * @param AvailableMilesResponse_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_AvailableMilesResponse_serialize(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env,
            axiom_node_t* AvailableMilesResponse_om_node, axiom_element_t *AvailableMilesResponse_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @param AvailableMilesResponse_om_node node to serialize from
         * @param AvailableMilesResponse_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_AvailableMilesResponse_serialize_obj(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env,
            axiom_node_t* AvailableMilesResponse_om_node, axiom_element_t *AvailableMilesResponse_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_AvailableMilesResponse is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_AvailableMilesResponse_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_AvailableMilesResponse_t
         * @param env pointer to environment struct
         * @param _errors axutil_array_list_t*
         * @param _ffpCardNumber axis2_char_t*
         * @param _miles double
         * @return newly created adb_AvailableMilesResponse_t object
         */
        adb_AvailableMilesResponse_t* AXIS2_CALL
        adb_AvailableMilesResponse_create_with_values(
            const axutil_env_t *env,
                axutil_array_list_t* _errors,
                axis2_char_t* _ffpCardNumber,
                double _miles);

        


                /**
                 * Free adb_AvailableMilesResponse_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                axutil_array_list_t* AXIS2_CALL
                adb_AvailableMilesResponse_free_popping_value(
                        adb_AvailableMilesResponse_t* _AvailableMilesResponse,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for errors by property number (1)
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return Array of adb_ErrorGroup_t*s.
         */
        axutil_array_list_t* AXIS2_CALL
        adb_AvailableMilesResponse_get_property1(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for ffpCardNumber by property number (2)
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_AvailableMilesResponse_get_property2(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for miles by property number (3)
         * @param  _AvailableMilesResponse adb_AvailableMilesResponse_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_AvailableMilesResponse_get_property3(
            adb_AvailableMilesResponse_t* _AvailableMilesResponse,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_AVAILABLEMILESRESPONSE_H */
    

