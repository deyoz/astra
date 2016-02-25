

        #ifndef ADB_CANCELRESERVEREQUEST_H
        #define ADB_CANCELRESERVEREQUEST_H

       /**
        * adb_CancelReserveRequest.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_CancelReserveRequest class
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
        

        typedef struct adb_CancelReserveRequest adb_CancelReserveRequest_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_CancelReserveRequest_t
         * @param env pointer to environment struct
         * @return newly created adb_CancelReserveRequest_t object
         */
        adb_CancelReserveRequest_t* AXIS2_CALL
        adb_CancelReserveRequest_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_free (
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);

        /**
         * Free adb_CancelReserveRequest_t object
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_free_obj (
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        
        

        /**
         * Getter for ffpCardNumber. 
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_CancelReserveRequest_get_ffpCardNumber(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);

        /**
         * Setter for ffpCardNumber.
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @param arg_ffpCardNumber axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_set_ffpCardNumber(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_ffpCardNumber);

        /**
         * Resetter for ffpCardNumber
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_reset_ffpCardNumber(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for messageID. 
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_CancelReserveRequest_get_messageID(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);

        /**
         * Setter for messageID.
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @param arg_messageID axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_set_messageID(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_messageID);

        /**
         * Resetter for messageID
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_reset_messageID(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for token. 
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_CancelReserveRequest_get_token(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);

        /**
         * Setter for token.
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @param arg_token int
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_set_token(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env,
            const int  arg_token);

        /**
         * Resetter for token
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_reset_token(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);

        


        /******************************* Checking and Setting NIL values *********************************/
        

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether ffpCardNumber is nill
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_CancelReserveRequest_is_ffpCardNumber_nil(
                adb_CancelReserveRequest_t* _CancelReserveRequest,
                const axutil_env_t *env);


        
        /**
         * Set ffpCardNumber to nill (currently the same as reset)
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_set_ffpCardNumber_nil(
                adb_CancelReserveRequest_t* _CancelReserveRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether messageID is nill
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_CancelReserveRequest_is_messageID_nil(
                adb_CancelReserveRequest_t* _CancelReserveRequest,
                const axutil_env_t *env);


        
        /**
         * Set messageID to nill (currently the same as reset)
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_set_messageID_nil(
                adb_CancelReserveRequest_t* _CancelReserveRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether token is nill
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_CancelReserveRequest_is_token_nil(
                adb_CancelReserveRequest_t* _CancelReserveRequest,
                const axutil_env_t *env);


        
        /**
         * Set token to nill (currently the same as reset)
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_set_token_nil(
                adb_CancelReserveRequest_t* _CancelReserveRequest,
                const axutil_env_t *env);
        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_deserialize(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_CancelReserveRequest_deserialize_obj(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_CancelReserveRequest_declare_parent_namespaces(
                    adb_CancelReserveRequest_t* _CancelReserveRequest,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @param CancelReserveRequest_om_node node to serialize from
         * @param CancelReserveRequest_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_CancelReserveRequest_serialize(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env,
            axiom_node_t* CancelReserveRequest_om_node, axiom_element_t *CancelReserveRequest_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @param CancelReserveRequest_om_node node to serialize from
         * @param CancelReserveRequest_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_CancelReserveRequest_serialize_obj(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env,
            axiom_node_t* CancelReserveRequest_om_node, axiom_element_t *CancelReserveRequest_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_CancelReserveRequest is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_CancelReserveRequest_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_CancelReserveRequest_t
         * @param env pointer to environment struct
         * @param _ffpCardNumber axis2_char_t*
         * @param _messageID axis2_char_t*
         * @param _token int
         * @return newly created adb_CancelReserveRequest_t object
         */
        adb_CancelReserveRequest_t* AXIS2_CALL
        adb_CancelReserveRequest_create_with_values(
            const axutil_env_t *env,
                axis2_char_t* _ffpCardNumber,
                axis2_char_t* _messageID,
                int _token);

        


                /**
                 * Free adb_CancelReserveRequest_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _CancelReserveRequest adb_CancelReserveRequest_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                axis2_char_t* AXIS2_CALL
                adb_CancelReserveRequest_free_popping_value(
                        adb_CancelReserveRequest_t* _CancelReserveRequest,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for ffpCardNumber by property number (1)
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_CancelReserveRequest_get_property1(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for messageID by property number (2)
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_CancelReserveRequest_get_property2(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for token by property number (3)
         * @param  _CancelReserveRequest adb_CancelReserveRequest_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_CancelReserveRequest_get_property3(
            adb_CancelReserveRequest_t* _CancelReserveRequest,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_CANCELRESERVEREQUEST_H */
    

