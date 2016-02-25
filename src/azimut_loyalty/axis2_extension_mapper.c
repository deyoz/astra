

        /**
         * axis2_extension_mapper.c
         *
         * This file was auto-generated from WSDL
         * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
         */

        #include "axis2_extension_mapper.h"

        #include "adb_PayMilesRequest.h"
        #include "adb_ReserveStatusResponse.h"
        #include "adb_ErrorGroup.h"
        #include "adb_ReserveStatusRequest.h"
        #include "adb_AvailableMilesResponse.h"
        #include "adb_BonusResponse.h"
        #include "adb_AvailableMilesRequest.h"
        #include "adb_ReserveMilesResponse.h"
        #include "adb_ErrorGroupResponse.h"
        #include "adb_PriceResponse.h"
        #include "adb_CancelReserveRequest.h"
        #include "adb_ClassMilesIncome.h"
        #include "adb_CancelReserveResponse.h"
        #include "adb_LoginResponse.h"
        #include "adb_PayMilesResponse.h"
        #include "adb_ClassMilesCost.h"
        #include "adb_BonusRequest.h"
        #include "adb_LoginRequest.h"
        #include "adb_PriceRequest.h"
        #include "adb_ReserveMilesRequest.h"
        

        struct adb_type
        {
            axis2_char_t *property_Type;
        };

        /**
         * Auxiliary function to determine an ADB object type from its Axiom node.
         * @param env pointer to environment struct
         * @param node double pointer to the parent node to deserialize
         * @return type name on success, else NULL
         */
        axis2_char_t *AXIS2_CALL
        axis2_extension_mapper_type_from_node(
            const axutil_env_t *env,
            axiom_node_t** node)
        {
            axiom_node_t *parent = *node;
            axutil_qname_t *element_qname = NULL;
            axiom_element_t *element = NULL;

            axutil_hash_index_t *hi;
            void *val;
            axiom_attribute_t *type_attr;
            axutil_hash_t *ht;
            axis2_char_t *temp;
            axis2_char_t *type;

            while(parent && axiom_node_get_node_type(parent, env) != AXIOM_ELEMENT)
            {
                parent = axiom_node_get_next_sibling(parent, env);
            }

            if (NULL == parent)
            {
                /* This should be checked before everything */
                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI,
                            "Failed in building adb object : "
                            "NULL elemenet can not be passed to deserialize");
                return AXIS2_FAILURE;
            }

            element = (axiom_element_t *)axiom_node_get_data_element(parent, env);

            ht = axiom_element_get_all_attributes(element, env);

            if (ht == NULL)
                return NULL;

            for (hi = axutil_hash_first(ht, env); hi; hi = axutil_hash_next(env, hi)) {
                axis2_char_t *localpart;
                axutil_hash_this(hi, NULL, NULL, &val);
                type_attr = (axiom_attribute_t *)val;
                localpart = axutil_qname_get_localpart(axiom_attribute_get_qname(type_attr, env), env);
                if (axutil_strcmp(localpart, "type") == 0) break;
            }

            type = axiom_attribute_get_value(type_attr, env);
            if (type != NULL && (temp = axutil_strchr(type, ':')) != NULL)
            {
                if (axutil_strchr(temp, ':') != NULL)
                    type = temp + 1; /* Pointer arithmetic */
            }

            return type;
        }

        axis2_char_t* AXIS2_CALL
        adb_type_get_type(const adb_type_t *object)
        {
            if (object != NULL)
              return object->property_Type;

            return NULL;
        }

        adb_type_t* AXIS2_CALL
        axis2_extension_mapper_create_from_node(
            const axutil_env_t *env,
            axiom_node_t** node,
            axis2_char_t *default_type)
        {
            axis2_char_t *type = axis2_extension_mapper_type_from_node(env, node);

            if (type != NULL)
            {
              
              if (axutil_strcmp(type, "PayMilesRequest") == 0)
              {
                  return (adb_type_t*) adb_PayMilesRequest_create(env);
              }
              
              if (axutil_strcmp(type, "ReserveStatusResponse") == 0)
              {
                  return (adb_type_t*) adb_ReserveStatusResponse_create(env);
              }
              
              if (axutil_strcmp(type, "ErrorGroup") == 0)
              {
                  return (adb_type_t*) adb_ErrorGroup_create(env);
              }
              
              if (axutil_strcmp(type, "ReserveStatusRequest") == 0)
              {
                  return (adb_type_t*) adb_ReserveStatusRequest_create(env);
              }
              
              if (axutil_strcmp(type, "AvailableMilesResponse") == 0)
              {
                  return (adb_type_t*) adb_AvailableMilesResponse_create(env);
              }
              
              if (axutil_strcmp(type, "BonusResponse") == 0)
              {
                  return (adb_type_t*) adb_BonusResponse_create(env);
              }
              
              if (axutil_strcmp(type, "AvailableMilesRequest") == 0)
              {
                  return (adb_type_t*) adb_AvailableMilesRequest_create(env);
              }
              
              if (axutil_strcmp(type, "ReserveMilesResponse") == 0)
              {
                  return (adb_type_t*) adb_ReserveMilesResponse_create(env);
              }
              
              if (axutil_strcmp(type, "ErrorGroupResponse") == 0)
              {
                  return (adb_type_t*) adb_ErrorGroupResponse_create(env);
              }
              
              if (axutil_strcmp(type, "PriceResponse") == 0)
              {
                  return (adb_type_t*) adb_PriceResponse_create(env);
              }
              
              if (axutil_strcmp(type, "CancelReserveRequest") == 0)
              {
                  return (adb_type_t*) adb_CancelReserveRequest_create(env);
              }
              
              if (axutil_strcmp(type, "ClassMilesIncome") == 0)
              {
                  return (adb_type_t*) adb_ClassMilesIncome_create(env);
              }
              
              if (axutil_strcmp(type, "CancelReserveResponse") == 0)
              {
                  return (adb_type_t*) adb_CancelReserveResponse_create(env);
              }
              
              if (axutil_strcmp(type, "LoginResponse") == 0)
              {
                  return (adb_type_t*) adb_LoginResponse_create(env);
              }
              
              if (axutil_strcmp(type, "PayMilesResponse") == 0)
              {
                  return (adb_type_t*) adb_PayMilesResponse_create(env);
              }
              
              if (axutil_strcmp(type, "ClassMilesCost") == 0)
              {
                  return (adb_type_t*) adb_ClassMilesCost_create(env);
              }
              
              if (axutil_strcmp(type, "BonusRequest") == 0)
              {
                  return (adb_type_t*) adb_BonusRequest_create(env);
              }
              
              if (axutil_strcmp(type, "LoginRequest") == 0)
              {
                  return (adb_type_t*) adb_LoginRequest_create(env);
              }
              
              if (axutil_strcmp(type, "PriceRequest") == 0)
              {
                  return (adb_type_t*) adb_PriceRequest_create(env);
              }
              
              if (axutil_strcmp(type, "ReserveMilesRequest") == 0)
              {
                  return (adb_type_t*) adb_ReserveMilesRequest_create(env);
              }
              
            }

            
            if (axutil_strcmp(default_type, "adb_PayMilesRequest") == 0)
            {
                return (adb_type_t*) adb_PayMilesRequest_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveStatusResponse") == 0)
            {
                return (adb_type_t*) adb_ReserveStatusResponse_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_ErrorGroup") == 0)
            {
                return (adb_type_t*) adb_ErrorGroup_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveStatusRequest") == 0)
            {
                return (adb_type_t*) adb_ReserveStatusRequest_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_AvailableMilesResponse") == 0)
            {
                return (adb_type_t*) adb_AvailableMilesResponse_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_BonusResponse") == 0)
            {
                return (adb_type_t*) adb_BonusResponse_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_AvailableMilesRequest") == 0)
            {
                return (adb_type_t*) adb_AvailableMilesRequest_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveMilesResponse") == 0)
            {
                return (adb_type_t*) adb_ReserveMilesResponse_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_ErrorGroupResponse") == 0)
            {
                return (adb_type_t*) adb_ErrorGroupResponse_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_PriceResponse") == 0)
            {
                return (adb_type_t*) adb_PriceResponse_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_CancelReserveRequest") == 0)
            {
                return (adb_type_t*) adb_CancelReserveRequest_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_ClassMilesIncome") == 0)
            {
                return (adb_type_t*) adb_ClassMilesIncome_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_CancelReserveResponse") == 0)
            {
                return (adb_type_t*) adb_CancelReserveResponse_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_LoginResponse") == 0)
            {
                return (adb_type_t*) adb_LoginResponse_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_PayMilesResponse") == 0)
            {
                return (adb_type_t*) adb_PayMilesResponse_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_ClassMilesCost") == 0)
            {
                return (adb_type_t*) adb_ClassMilesCost_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_BonusRequest") == 0)
            {
                return (adb_type_t*) adb_BonusRequest_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_LoginRequest") == 0)
            {
                return (adb_type_t*) adb_LoginRequest_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_PriceRequest") == 0)
            {
                return (adb_type_t*) adb_PriceRequest_create(env);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveMilesRequest") == 0)
            {
                return (adb_type_t*) adb_ReserveMilesRequest_create(env);
            }
            

            return NULL;
        }

        axis2_status_t AXIS2_CALL
        axis2_extension_mapper_free(
            adb_type_t* _object,
            const axutil_env_t *env,
            axis2_char_t *default_type)
        {
            if (_object != NULL && adb_type_get_type(_object) != NULL)
            {
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PayMilesRequest") == 0)
                {
                    return adb_PayMilesRequest_free_obj(
                    (adb_PayMilesRequest_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveStatusResponse") == 0)
                {
                    return adb_ReserveStatusResponse_free_obj(
                    (adb_ReserveStatusResponse_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ErrorGroup") == 0)
                {
                    return adb_ErrorGroup_free_obj(
                    (adb_ErrorGroup_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveStatusRequest") == 0)
                {
                    return adb_ReserveStatusRequest_free_obj(
                    (adb_ReserveStatusRequest_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_AvailableMilesResponse") == 0)
                {
                    return adb_AvailableMilesResponse_free_obj(
                    (adb_AvailableMilesResponse_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_BonusResponse") == 0)
                {
                    return adb_BonusResponse_free_obj(
                    (adb_BonusResponse_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_AvailableMilesRequest") == 0)
                {
                    return adb_AvailableMilesRequest_free_obj(
                    (adb_AvailableMilesRequest_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveMilesResponse") == 0)
                {
                    return adb_ReserveMilesResponse_free_obj(
                    (adb_ReserveMilesResponse_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ErrorGroupResponse") == 0)
                {
                    return adb_ErrorGroupResponse_free_obj(
                    (adb_ErrorGroupResponse_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PriceResponse") == 0)
                {
                    return adb_PriceResponse_free_obj(
                    (adb_PriceResponse_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_CancelReserveRequest") == 0)
                {
                    return adb_CancelReserveRequest_free_obj(
                    (adb_CancelReserveRequest_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ClassMilesIncome") == 0)
                {
                    return adb_ClassMilesIncome_free_obj(
                    (adb_ClassMilesIncome_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_CancelReserveResponse") == 0)
                {
                    return adb_CancelReserveResponse_free_obj(
                    (adb_CancelReserveResponse_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_LoginResponse") == 0)
                {
                    return adb_LoginResponse_free_obj(
                    (adb_LoginResponse_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PayMilesResponse") == 0)
                {
                    return adb_PayMilesResponse_free_obj(
                    (adb_PayMilesResponse_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ClassMilesCost") == 0)
                {
                    return adb_ClassMilesCost_free_obj(
                    (adb_ClassMilesCost_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_BonusRequest") == 0)
                {
                    return adb_BonusRequest_free_obj(
                    (adb_BonusRequest_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_LoginRequest") == 0)
                {
                    return adb_LoginRequest_free_obj(
                    (adb_LoginRequest_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PriceRequest") == 0)
                {
                    return adb_PriceRequest_free_obj(
                    (adb_PriceRequest_t*) _object, env);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveMilesRequest") == 0)
                {
                    return adb_ReserveMilesRequest_free_obj(
                    (adb_ReserveMilesRequest_t*) _object, env);
                }
            
            }

            
            if (axutil_strcmp(default_type, "adb_PayMilesRequest") == 0)
            {
                return adb_PayMilesRequest_free_obj(
                (adb_PayMilesRequest_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveStatusResponse") == 0)
            {
                return adb_ReserveStatusResponse_free_obj(
                (adb_ReserveStatusResponse_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_ErrorGroup") == 0)
            {
                return adb_ErrorGroup_free_obj(
                (adb_ErrorGroup_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveStatusRequest") == 0)
            {
                return adb_ReserveStatusRequest_free_obj(
                (adb_ReserveStatusRequest_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_AvailableMilesResponse") == 0)
            {
                return adb_AvailableMilesResponse_free_obj(
                (adb_AvailableMilesResponse_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_BonusResponse") == 0)
            {
                return adb_BonusResponse_free_obj(
                (adb_BonusResponse_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_AvailableMilesRequest") == 0)
            {
                return adb_AvailableMilesRequest_free_obj(
                (adb_AvailableMilesRequest_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveMilesResponse") == 0)
            {
                return adb_ReserveMilesResponse_free_obj(
                (adb_ReserveMilesResponse_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_ErrorGroupResponse") == 0)
            {
                return adb_ErrorGroupResponse_free_obj(
                (adb_ErrorGroupResponse_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_PriceResponse") == 0)
            {
                return adb_PriceResponse_free_obj(
                (adb_PriceResponse_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_CancelReserveRequest") == 0)
            {
                return adb_CancelReserveRequest_free_obj(
                (adb_CancelReserveRequest_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_ClassMilesIncome") == 0)
            {
                return adb_ClassMilesIncome_free_obj(
                (adb_ClassMilesIncome_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_CancelReserveResponse") == 0)
            {
                return adb_CancelReserveResponse_free_obj(
                (adb_CancelReserveResponse_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_LoginResponse") == 0)
            {
                return adb_LoginResponse_free_obj(
                (adb_LoginResponse_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_PayMilesResponse") == 0)
            {
                return adb_PayMilesResponse_free_obj(
                (adb_PayMilesResponse_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_ClassMilesCost") == 0)
            {
                return adb_ClassMilesCost_free_obj(
                (adb_ClassMilesCost_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_BonusRequest") == 0)
            {
                return adb_BonusRequest_free_obj(
                (adb_BonusRequest_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_LoginRequest") == 0)
            {
                return adb_LoginRequest_free_obj(
                (adb_LoginRequest_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_PriceRequest") == 0)
            {
                return adb_PriceRequest_free_obj(
                (adb_PriceRequest_t*) _object, env);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveMilesRequest") == 0)
            {
                return adb_ReserveMilesRequest_free_obj(
                (adb_ReserveMilesRequest_t*) _object, env);
            }
            

            return AXIS2_FAILURE;
        }

        axis2_status_t AXIS2_CALL
        axis2_extension_mapper_deserialize(
            adb_type_t* _object,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs,
            axis2_char_t *default_type)
        {
            if (_object != NULL && adb_type_get_type(_object) != NULL)
            {
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PayMilesRequest") == 0)
                {
                    return adb_PayMilesRequest_deserialize_obj(
                    (adb_PayMilesRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveStatusResponse") == 0)
                {
                    return adb_ReserveStatusResponse_deserialize_obj(
                    (adb_ReserveStatusResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ErrorGroup") == 0)
                {
                    return adb_ErrorGroup_deserialize_obj(
                    (adb_ErrorGroup_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveStatusRequest") == 0)
                {
                    return adb_ReserveStatusRequest_deserialize_obj(
                    (adb_ReserveStatusRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_AvailableMilesResponse") == 0)
                {
                    return adb_AvailableMilesResponse_deserialize_obj(
                    (adb_AvailableMilesResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_BonusResponse") == 0)
                {
                    return adb_BonusResponse_deserialize_obj(
                    (adb_BonusResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_AvailableMilesRequest") == 0)
                {
                    return adb_AvailableMilesRequest_deserialize_obj(
                    (adb_AvailableMilesRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveMilesResponse") == 0)
                {
                    return adb_ReserveMilesResponse_deserialize_obj(
                    (adb_ReserveMilesResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ErrorGroupResponse") == 0)
                {
                    return adb_ErrorGroupResponse_deserialize_obj(
                    (adb_ErrorGroupResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PriceResponse") == 0)
                {
                    return adb_PriceResponse_deserialize_obj(
                    (adb_PriceResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_CancelReserveRequest") == 0)
                {
                    return adb_CancelReserveRequest_deserialize_obj(
                    (adb_CancelReserveRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ClassMilesIncome") == 0)
                {
                    return adb_ClassMilesIncome_deserialize_obj(
                    (adb_ClassMilesIncome_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_CancelReserveResponse") == 0)
                {
                    return adb_CancelReserveResponse_deserialize_obj(
                    (adb_CancelReserveResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_LoginResponse") == 0)
                {
                    return adb_LoginResponse_deserialize_obj(
                    (adb_LoginResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PayMilesResponse") == 0)
                {
                    return adb_PayMilesResponse_deserialize_obj(
                    (adb_PayMilesResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ClassMilesCost") == 0)
                {
                    return adb_ClassMilesCost_deserialize_obj(
                    (adb_ClassMilesCost_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_BonusRequest") == 0)
                {
                    return adb_BonusRequest_deserialize_obj(
                    (adb_BonusRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_LoginRequest") == 0)
                {
                    return adb_LoginRequest_deserialize_obj(
                    (adb_LoginRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PriceRequest") == 0)
                {
                    return adb_PriceRequest_deserialize_obj(
                    (adb_PriceRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveMilesRequest") == 0)
                {
                    return adb_ReserveMilesRequest_deserialize_obj(
                    (adb_ReserveMilesRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
                }
            
            }

            
            if (axutil_strcmp(default_type, "adb_PayMilesRequest") == 0)
            {
                return adb_PayMilesRequest_deserialize_obj(
                (adb_PayMilesRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveStatusResponse") == 0)
            {
                return adb_ReserveStatusResponse_deserialize_obj(
                (adb_ReserveStatusResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_ErrorGroup") == 0)
            {
                return adb_ErrorGroup_deserialize_obj(
                (adb_ErrorGroup_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveStatusRequest") == 0)
            {
                return adb_ReserveStatusRequest_deserialize_obj(
                (adb_ReserveStatusRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_AvailableMilesResponse") == 0)
            {
                return adb_AvailableMilesResponse_deserialize_obj(
                (adb_AvailableMilesResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_BonusResponse") == 0)
            {
                return adb_BonusResponse_deserialize_obj(
                (adb_BonusResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_AvailableMilesRequest") == 0)
            {
                return adb_AvailableMilesRequest_deserialize_obj(
                (adb_AvailableMilesRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveMilesResponse") == 0)
            {
                return adb_ReserveMilesResponse_deserialize_obj(
                (adb_ReserveMilesResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_ErrorGroupResponse") == 0)
            {
                return adb_ErrorGroupResponse_deserialize_obj(
                (adb_ErrorGroupResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_PriceResponse") == 0)
            {
                return adb_PriceResponse_deserialize_obj(
                (adb_PriceResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_CancelReserveRequest") == 0)
            {
                return adb_CancelReserveRequest_deserialize_obj(
                (adb_CancelReserveRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_ClassMilesIncome") == 0)
            {
                return adb_ClassMilesIncome_deserialize_obj(
                (adb_ClassMilesIncome_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_CancelReserveResponse") == 0)
            {
                return adb_CancelReserveResponse_deserialize_obj(
                (adb_CancelReserveResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_LoginResponse") == 0)
            {
                return adb_LoginResponse_deserialize_obj(
                (adb_LoginResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_PayMilesResponse") == 0)
            {
                return adb_PayMilesResponse_deserialize_obj(
                (adb_PayMilesResponse_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_ClassMilesCost") == 0)
            {
                return adb_ClassMilesCost_deserialize_obj(
                (adb_ClassMilesCost_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_BonusRequest") == 0)
            {
                return adb_BonusRequest_deserialize_obj(
                (adb_BonusRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_LoginRequest") == 0)
            {
                return adb_LoginRequest_deserialize_obj(
                (adb_LoginRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_PriceRequest") == 0)
            {
                return adb_PriceRequest_deserialize_obj(
                (adb_PriceRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveMilesRequest") == 0)
            {
                return adb_ReserveMilesRequest_deserialize_obj(
                (adb_ReserveMilesRequest_t*) _object, env, dp_parent, dp_is_early_node_valid, dont_care_minoccurs);
            }
            

            return AXIS2_FAILURE;
        }

        axiom_node_t* AXIS2_CALL
        axis2_extension_mapper_serialize(
            adb_type_t* _object,
            const axutil_env_t *env,
            axiom_node_t* om_node,
            axiom_element_t *om_element,
            int tag_closed,
            axutil_hash_t *namespaces,
            int *next_ns_index,
            axis2_char_t *default_type)
        {
            if (_object != NULL && adb_type_get_type(_object) != NULL)
            {
                
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PayMilesRequest") == 0)
                {
                    return adb_PayMilesRequest_serialize_obj(
                    (adb_PayMilesRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveStatusResponse") == 0)
                {
                    return adb_ReserveStatusResponse_serialize_obj(
                    (adb_ReserveStatusResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ErrorGroup") == 0)
                {
                    return adb_ErrorGroup_serialize_obj(
                    (adb_ErrorGroup_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveStatusRequest") == 0)
                {
                    return adb_ReserveStatusRequest_serialize_obj(
                    (adb_ReserveStatusRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_AvailableMilesResponse") == 0)
                {
                    return adb_AvailableMilesResponse_serialize_obj(
                    (adb_AvailableMilesResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_BonusResponse") == 0)
                {
                    return adb_BonusResponse_serialize_obj(
                    (adb_BonusResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_AvailableMilesRequest") == 0)
                {
                    return adb_AvailableMilesRequest_serialize_obj(
                    (adb_AvailableMilesRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveMilesResponse") == 0)
                {
                    return adb_ReserveMilesResponse_serialize_obj(
                    (adb_ReserveMilesResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ErrorGroupResponse") == 0)
                {
                    return adb_ErrorGroupResponse_serialize_obj(
                    (adb_ErrorGroupResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PriceResponse") == 0)
                {
                    return adb_PriceResponse_serialize_obj(
                    (adb_PriceResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_CancelReserveRequest") == 0)
                {
                    return adb_CancelReserveRequest_serialize_obj(
                    (adb_CancelReserveRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ClassMilesIncome") == 0)
                {
                    return adb_ClassMilesIncome_serialize_obj(
                    (adb_ClassMilesIncome_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_CancelReserveResponse") == 0)
                {
                    return adb_CancelReserveResponse_serialize_obj(
                    (adb_CancelReserveResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_LoginResponse") == 0)
                {
                    return adb_LoginResponse_serialize_obj(
                    (adb_LoginResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PayMilesResponse") == 0)
                {
                    return adb_PayMilesResponse_serialize_obj(
                    (adb_PayMilesResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ClassMilesCost") == 0)
                {
                    return adb_ClassMilesCost_serialize_obj(
                    (adb_ClassMilesCost_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_BonusRequest") == 0)
                {
                    return adb_BonusRequest_serialize_obj(
                    (adb_BonusRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_LoginRequest") == 0)
                {
                    return adb_LoginRequest_serialize_obj(
                    (adb_LoginRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_PriceRequest") == 0)
                {
                    return adb_PriceRequest_serialize_obj(
                    (adb_PriceRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
                if (axutil_strcmp(adb_type_get_type(_object), "adb_ReserveMilesRequest") == 0)
                {
                    return adb_ReserveMilesRequest_serialize_obj(
                    (adb_ReserveMilesRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
                }
            
            }

            
            if (axutil_strcmp(default_type, "adb_PayMilesRequest") == 0)
            {
                return adb_PayMilesRequest_serialize_obj(
                (adb_PayMilesRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveStatusResponse") == 0)
            {
                return adb_ReserveStatusResponse_serialize_obj(
                (adb_ReserveStatusResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_ErrorGroup") == 0)
            {
                return adb_ErrorGroup_serialize_obj(
                (adb_ErrorGroup_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveStatusRequest") == 0)
            {
                return adb_ReserveStatusRequest_serialize_obj(
                (adb_ReserveStatusRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_AvailableMilesResponse") == 0)
            {
                return adb_AvailableMilesResponse_serialize_obj(
                (adb_AvailableMilesResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_BonusResponse") == 0)
            {
                return adb_BonusResponse_serialize_obj(
                (adb_BonusResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_AvailableMilesRequest") == 0)
            {
                return adb_AvailableMilesRequest_serialize_obj(
                (adb_AvailableMilesRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveMilesResponse") == 0)
            {
                return adb_ReserveMilesResponse_serialize_obj(
                (adb_ReserveMilesResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_ErrorGroupResponse") == 0)
            {
                return adb_ErrorGroupResponse_serialize_obj(
                (adb_ErrorGroupResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_PriceResponse") == 0)
            {
                return adb_PriceResponse_serialize_obj(
                (adb_PriceResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_CancelReserveRequest") == 0)
            {
                return adb_CancelReserveRequest_serialize_obj(
                (adb_CancelReserveRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_ClassMilesIncome") == 0)
            {
                return adb_ClassMilesIncome_serialize_obj(
                (adb_ClassMilesIncome_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_CancelReserveResponse") == 0)
            {
                return adb_CancelReserveResponse_serialize_obj(
                (adb_CancelReserveResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_LoginResponse") == 0)
            {
                return adb_LoginResponse_serialize_obj(
                (adb_LoginResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_PayMilesResponse") == 0)
            {
                return adb_PayMilesResponse_serialize_obj(
                (adb_PayMilesResponse_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_ClassMilesCost") == 0)
            {
                return adb_ClassMilesCost_serialize_obj(
                (adb_ClassMilesCost_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_BonusRequest") == 0)
            {
                return adb_BonusRequest_serialize_obj(
                (adb_BonusRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_LoginRequest") == 0)
            {
                return adb_LoginRequest_serialize_obj(
                (adb_LoginRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_PriceRequest") == 0)
            {
                return adb_PriceRequest_serialize_obj(
                (adb_PriceRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            
            if (axutil_strcmp(default_type, "adb_ReserveMilesRequest") == 0)
            {
                return adb_ReserveMilesRequest_serialize_obj(
                (adb_ReserveMilesRequest_t*) _object, env, om_node, om_element, tag_closed, namespaces, next_ns_index);
            }
            

            return AXIS2_FAILURE;
        }
    

