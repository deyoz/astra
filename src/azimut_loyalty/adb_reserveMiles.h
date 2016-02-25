

        #ifndef ADB_RESERVEMILES_H
        #define ADB_RESERVEMILES_H

       /**
        * adb_reserveMiles.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_reserveMiles class
        */

        
          #include "adb_ReserveMilesRequest.h"
          

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
        

        typedef struct adb_reserveMiles adb_reserveMiles_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_reserveMiles_t
         * @param env pointer to environment struct
         * @return newly created adb_reserveMiles_t object
         */
        adb_reserveMiles_t* AXIS2_CALL
        adb_reserveMiles_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _reserveMiles adb_reserveMiles_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_reserveMiles_free (
            adb_reserveMiles_t* _reserveMiles,
            const axutil_env_t *env);

        /**
         * Free adb_reserveMiles_t object
         * @param  _reserveMiles adb_reserveMiles_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_reserveMiles_free_obj (
            adb_reserveMiles_t* _reserveMiles,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        
        

        /**
         * Getter for request. 
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @return adb_ReserveMilesRequest_t*
         */
        adb_ReserveMilesRequest_t* AXIS2_CALL
        adb_reserveMiles_get_request(
            adb_reserveMiles_t* _reserveMiles,
            const axutil_env_t *env);

        /**
         * Setter for request.
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @param arg_request adb_ReserveMilesRequest_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_reserveMiles_set_request(
            adb_reserveMiles_t* _reserveMiles,
            const axutil_env_t *env,
            adb_ReserveMilesRequest_t*  arg_request);

        /**
         * Resetter for request
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_reserveMiles_reset_request(
            adb_reserveMiles_t* _reserveMiles,
            const axutil_env_t *env);

        


        /******************************* Checking and Setting NIL values *********************************/
        

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether request is nill
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_reserveMiles_is_request_nil(
                adb_reserveMiles_t* _reserveMiles,
                const axutil_env_t *env);


        
        /**
         * Set request to nill (currently the same as reset)
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_reserveMiles_set_request_nil(
                adb_reserveMiles_t* _reserveMiles,
                const axutil_env_t *env);
        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_reserveMiles_deserialize(
            adb_reserveMiles_t* _reserveMiles,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_reserveMiles_deserialize_obj(
            adb_reserveMiles_t* _reserveMiles,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_reserveMiles_declare_parent_namespaces(
                    adb_reserveMiles_t* _reserveMiles,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @param reserveMiles_om_node node to serialize from
         * @param reserveMiles_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_reserveMiles_serialize(
            adb_reserveMiles_t* _reserveMiles,
            const axutil_env_t *env,
            axiom_node_t* reserveMiles_om_node, axiom_element_t *reserveMiles_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @param reserveMiles_om_node node to serialize from
         * @param reserveMiles_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_reserveMiles_serialize_obj(
            adb_reserveMiles_t* _reserveMiles,
            const axutil_env_t *env,
            axiom_node_t* reserveMiles_om_node, axiom_element_t *reserveMiles_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_reserveMiles is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_reserveMiles_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_reserveMiles_t
         * @param env pointer to environment struct
         * @param _request adb_ReserveMilesRequest_t*
         * @return newly created adb_reserveMiles_t object
         */
        adb_reserveMiles_t* AXIS2_CALL
        adb_reserveMiles_create_with_values(
            const axutil_env_t *env,
                adb_ReserveMilesRequest_t* _request);

        


                /**
                 * Free adb_reserveMiles_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _reserveMiles adb_reserveMiles_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                adb_ReserveMilesRequest_t* AXIS2_CALL
                adb_reserveMiles_free_popping_value(
                        adb_reserveMiles_t* _reserveMiles,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for request by property number (1)
         * @param  _reserveMiles adb_reserveMiles_t object
         * @param env pointer to environment struct
         * @return adb_ReserveMilesRequest_t*
         */
        adb_ReserveMilesRequest_t* AXIS2_CALL
        adb_reserveMiles_get_property1(
            adb_reserveMiles_t* _reserveMiles,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_RESERVEMILES_H */
    

