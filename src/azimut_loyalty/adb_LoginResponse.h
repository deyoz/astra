

        #ifndef ADB_LOGINRESPONSE_H
        #define ADB_LOGINRESPONSE_H

       /**
        * adb_LoginResponse.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_LoginResponse class
        */

        
          #include "adb_ErrorGroup.h"
          
            #include <axutil_date_time.h>
          

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
        

        typedef struct adb_LoginResponse adb_LoginResponse_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_LoginResponse_t
         * @param env pointer to environment struct
         * @return newly created adb_LoginResponse_t object
         */
        adb_LoginResponse_t* AXIS2_CALL
        adb_LoginResponse_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _LoginResponse adb_LoginResponse_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_free (
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        /**
         * Free adb_LoginResponse_t object
         * @param  _LoginResponse adb_LoginResponse_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_free_obj (
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        /******** Deprecated for array types, Use 'Getters and Setters for Arrays' instead ***********/
        

        /**
         * Getter for errors. Deprecated for array types, Use adb_LoginResponse_get_errors_at instead
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return Array of adb_ErrorGroup_t*s.
         */
        axutil_array_list_t* AXIS2_CALL
        adb_LoginResponse_get_errors(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        /**
         * Setter for errors.Deprecated for array types, Use adb_LoginResponse_set_errors_at
         * or adb_LoginResponse_add_errors instead.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param arg_errors Array of adb_ErrorGroup_t*s.
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_errors(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            axutil_array_list_t*  arg_errors);

        /**
         * Resetter for errors
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_reset_errors(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for access. 
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_bool_t
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_get_access(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        /**
         * Setter for access.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param arg_access axis2_bool_t
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_access(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            axis2_bool_t  arg_access);

        /**
         * Resetter for access
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_reset_access(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for dob. 
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axutil_date_time_t*
         */
        axutil_date_time_t* AXIS2_CALL
        adb_LoginResponse_get_dob(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        /**
         * Setter for dob.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param arg_dob axutil_date_time_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_dob(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            axutil_date_time_t*  arg_dob);

        /**
         * Resetter for dob
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_reset_dob(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for email. 
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_LoginResponse_get_email(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        /**
         * Setter for email.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param arg_email axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_email(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            const axis2_char_t*  arg_email);

        /**
         * Resetter for email
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_reset_email(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for lastName. 
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_LoginResponse_get_lastName(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        /**
         * Setter for lastName.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param arg_lastName axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_lastName(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            const axis2_char_t*  arg_lastName);

        /**
         * Resetter for lastName
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_reset_lastName(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for name. 
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_LoginResponse_get_name(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        /**
         * Setter for name.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param arg_name axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_name(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            const axis2_char_t*  arg_name);

        /**
         * Resetter for name
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_reset_name(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for secondName. 
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_LoginResponse_get_secondName(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        /**
         * Setter for secondName.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param arg_secondName axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_secondName(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            const axis2_char_t*  arg_secondName);

        /**
         * Resetter for secondName
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_reset_secondName(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        
        

        /**
         * Getter for status. 
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_LoginResponse_get_status(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

        /**
         * Setter for status.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param arg_status axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_status(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            const axis2_char_t*  arg_status);

        /**
         * Resetter for status
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_reset_status(
            adb_LoginResponse_t* _LoginResponse,
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
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param i index of the item to return
         * @return ith adb_ErrorGroup_t* of the array
         */
        adb_ErrorGroup_t* AXIS2_CALL
        adb_LoginResponse_get_errors_at(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env, int i);

        /**
         * Set the ith element of errors. (If the ith already exist, it will be replaced)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param i index of the item to return
         * @param arg_errors element to set adb_ErrorGroup_t* to the array
         * @return ith adb_ErrorGroup_t* of the array
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_errors_at(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env, int i,
                adb_ErrorGroup_t* arg_errors);


        /**
         * Add to errors.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param arg_errors element to add adb_ErrorGroup_t* to the array
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_add_errors(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env,
                adb_ErrorGroup_t* arg_errors);

        /**
         * Get the size of the errors array.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct.
         * @return the size of the errors array.
         */
        int AXIS2_CALL
        adb_LoginResponse_sizeof_errors(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env);

        /**
         * Remove the ith element of errors.
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param i index of the item to remove
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_remove_errors_at(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env, int i);

        


        /******************************* Checking and Setting NIL values *********************************/
        /* Use 'Checking and Setting NIL values for Arrays' to check and set nil for individual elements */

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether errors is nill
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_is_errors_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);


        
        /**
         * Set errors to nill (currently the same as reset)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_errors_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether access is nill
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_is_access_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);


        
        /**
         * Set access to nill (currently the same as reset)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_access_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether dob is nill
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_is_dob_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);


        
        /**
         * Set dob to nill (currently the same as reset)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_dob_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether email is nill
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_is_email_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);


        
        /**
         * Set email to nill (currently the same as reset)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_email_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether lastName is nill
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_is_lastName_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);


        
        /**
         * Set lastName to nill (currently the same as reset)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_lastName_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether name is nill
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_is_name_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);


        
        /**
         * Set name to nill (currently the same as reset)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_name_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether secondName is nill
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_is_secondName_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);


        
        /**
         * Set secondName to nill (currently the same as reset)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_secondName_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);
        

        /**
         * Check whether status is nill
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_is_status_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);


        
        /**
         * Set status to nill (currently the same as reset)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_status_nil(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env);
        
        /*************************** Checking and Setting 'NIL' values in Arrays *****************************/

        /**
         * NOTE: You may set this to remove specific elements in the array
         *       But you can not remove elements, if the specific property is declared to be non-nillable or sizeof(array) < minOccurs
         */
        
        /**
         * Check whether errors is nill at i
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct.
         * @param i index of the item to return.
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_is_errors_nil_at(
                adb_LoginResponse_t* _LoginResponse,
                const axutil_env_t *env, int i);
 
       
        /**
         * Set errors to nill at i
         * @param  _LoginResponse _ adb_LoginResponse_t object
         * @param env pointer to environment struct.
         * @param i index of the item to set.
         * @return AXIS2_SUCCESS on success, or AXIS2_FAILURE otherwise.
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_set_errors_nil_at(
                adb_LoginResponse_t* _LoginResponse, 
                const axutil_env_t *env, int i);

        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_deserialize(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_LoginResponse_deserialize_obj(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_LoginResponse_declare_parent_namespaces(
                    adb_LoginResponse_t* _LoginResponse,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param LoginResponse_om_node node to serialize from
         * @param LoginResponse_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_LoginResponse_serialize(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            axiom_node_t* LoginResponse_om_node, axiom_element_t *LoginResponse_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @param LoginResponse_om_node node to serialize from
         * @param LoginResponse_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_LoginResponse_serialize_obj(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env,
            axiom_node_t* LoginResponse_om_node, axiom_element_t *LoginResponse_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_LoginResponse is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_LoginResponse_t
         * @param env pointer to environment struct
         * @param _errors axutil_array_list_t*
         * @param _access axis2_bool_t
         * @param _dob axutil_date_time_t*
         * @param _email axis2_char_t*
         * @param _lastName axis2_char_t*
         * @param _name axis2_char_t*
         * @param _secondName axis2_char_t*
         * @param _status axis2_char_t*
         * @return newly created adb_LoginResponse_t object
         */
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
                axis2_char_t* _status);

        


                /**
                 * Free adb_LoginResponse_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _LoginResponse adb_LoginResponse_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                axutil_array_list_t* AXIS2_CALL
                adb_LoginResponse_free_popping_value(
                        adb_LoginResponse_t* _LoginResponse,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for errors by property number (1)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return Array of adb_ErrorGroup_t*s.
         */
        axutil_array_list_t* AXIS2_CALL
        adb_LoginResponse_get_property1(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for access by property number (2)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_bool_t
         */
        axis2_bool_t AXIS2_CALL
        adb_LoginResponse_get_property2(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for dob by property number (3)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axutil_date_time_t*
         */
        axutil_date_time_t* AXIS2_CALL
        adb_LoginResponse_get_property3(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for email by property number (4)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_LoginResponse_get_property4(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for lastName by property number (5)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_LoginResponse_get_property5(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for name by property number (6)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_LoginResponse_get_property6(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for secondName by property number (7)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_LoginResponse_get_property7(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

    
        

        /**
         * Getter for status by property number (8)
         * @param  _LoginResponse adb_LoginResponse_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_LoginResponse_get_property8(
            adb_LoginResponse_t* _LoginResponse,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_LOGINRESPONSE_H */
    

