

        #ifndef ADB_BONUSREQUEST_H
        #define ADB_BONUSREQUEST_H

       /**
        * adb_BonusRequest.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_BonusRequest class
        */

        
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
        

        typedef struct adb_BonusRequest adb_BonusRequest_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_BonusRequest_t
         * @param env pointer to environment struct
         * @return newly created adb_BonusRequest_t object
         */
        adb_BonusRequest_t* AXIS2_CALL
        adb_BonusRequest_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _BonusRequest adb_BonusRequest_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_free (
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        /**
         * Free adb_BonusRequest_t object
         * @param  _BonusRequest adb_BonusRequest_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_free_obj (
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        
        

        /**
         * Getter for arrival. 
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_BonusRequest_get_arrival(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        /**
         * Setter for arrival.
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param arg_arrival axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_arrival(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_arrival);

        /**
         * Resetter for arrival
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_reset_arrival(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for count. 
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_BonusRequest_get_count(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        /**
         * Setter for count.
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param arg_count int
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_count(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env,
            const int  arg_count);

        /**
         * Resetter for count
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_reset_count(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for departure. 
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_BonusRequest_get_departure(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        /**
         * Setter for departure.
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param arg_departure axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_departure(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_departure);

        /**
         * Resetter for departure
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_reset_departure(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for ffpCardNumber. 
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_BonusRequest_get_ffpCardNumber(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        /**
         * Setter for ffpCardNumber.
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param arg_ffpCardNumber axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_ffpCardNumber(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_ffpCardNumber);

        /**
         * Resetter for ffpCardNumber
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_reset_ffpCardNumber(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for flightDate. 
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return axutil_date_time_t*
         */
        axutil_date_time_t* AXIS2_CALL
        adb_BonusRequest_get_flightDate(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        /**
         * Setter for flightDate.
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param arg_flightDate axutil_date_time_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_flightDate(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env,
            axutil_date_time_t*  arg_flightDate);

        /**
         * Resetter for flightDate
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_reset_flightDate(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for subClass. 
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_BonusRequest_get_subClass(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        /**
         * Setter for subClass.
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param arg_subClass axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_subClass(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_subClass);

        /**
         * Resetter for subClass
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_reset_subClass(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

        


        /******************************* Checking and Setting NIL values *********************************/
        

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether arrival is nill
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_BonusRequest_is_arrival_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);


        
        /**
         * Set arrival to nill (currently the same as reset)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_arrival_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether count is nill
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_BonusRequest_is_count_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);


        
        /**
         * Set count to nill (currently the same as reset)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_count_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether departure is nill
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_BonusRequest_is_departure_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);


        
        /**
         * Set departure to nill (currently the same as reset)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_departure_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether ffpCardNumber is nill
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_BonusRequest_is_ffpCardNumber_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);


        
        /**
         * Set ffpCardNumber to nill (currently the same as reset)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_ffpCardNumber_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether flightDate is nill
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_BonusRequest_is_flightDate_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);


        
        /**
         * Set flightDate to nill (currently the same as reset)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_flightDate_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether subClass is nill
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_BonusRequest_is_subClass_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);


        
        /**
         * Set subClass to nill (currently the same as reset)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_set_subClass_nil(
                adb_BonusRequest_t* _BonusRequest,
                const axutil_env_t *env);
        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_deserialize(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_BonusRequest_deserialize_obj(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_BonusRequest_declare_parent_namespaces(
                    adb_BonusRequest_t* _BonusRequest,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param BonusRequest_om_node node to serialize from
         * @param BonusRequest_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_BonusRequest_serialize(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env,
            axiom_node_t* BonusRequest_om_node, axiom_element_t *BonusRequest_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @param BonusRequest_om_node node to serialize from
         * @param BonusRequest_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_BonusRequest_serialize_obj(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env,
            axiom_node_t* BonusRequest_om_node, axiom_element_t *BonusRequest_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_BonusRequest is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_BonusRequest_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_BonusRequest_t
         * @param env pointer to environment struct
         * @param _arrival axis2_char_t*
         * @param _count int
         * @param _departure axis2_char_t*
         * @param _ffpCardNumber axis2_char_t*
         * @param _flightDate axutil_date_time_t*
         * @param _subClass axis2_char_t*
         * @return newly created adb_BonusRequest_t object
         */
        adb_BonusRequest_t* AXIS2_CALL
        adb_BonusRequest_create_with_values(
            const axutil_env_t *env,
                axis2_char_t* _arrival,
                int _count,
                axis2_char_t* _departure,
                axis2_char_t* _ffpCardNumber,
                axutil_date_time_t* _flightDate,
                axis2_char_t* _subClass);

        


                /**
                 * Free adb_BonusRequest_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _BonusRequest adb_BonusRequest_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                axis2_char_t* AXIS2_CALL
                adb_BonusRequest_free_popping_value(
                        adb_BonusRequest_t* _BonusRequest,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for arrival by property number (1)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_BonusRequest_get_property1(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for count by property number (2)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_BonusRequest_get_property2(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for departure by property number (3)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_BonusRequest_get_property3(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for ffpCardNumber by property number (4)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_BonusRequest_get_property4(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for flightDate by property number (5)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return axutil_date_time_t*
         */
        axutil_date_time_t* AXIS2_CALL
        adb_BonusRequest_get_property5(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for subClass by property number (6)
         * @param  _BonusRequest adb_BonusRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_BonusRequest_get_property6(
            adb_BonusRequest_t* _BonusRequest,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_BONUSREQUEST_H */
    

