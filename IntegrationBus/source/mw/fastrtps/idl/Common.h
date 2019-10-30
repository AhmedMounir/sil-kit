// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*! 
 * @file Common.h
 * This header file contains the declaration of the described types in the IDL file.
 *
 * This file was generated by the tool gen.
 */

#ifndef _IB_MW_IDL_COMMON_H_
#define _IB_MW_IDL_COMMON_H_

// TODO Poner en el contexto.

#include <stdint.h>
#include <array>
#include <string>
#include <vector>
#include <map>
#include <bitset>

#if defined(_WIN32)
#if defined(EPROSIMA_USER_DLL_EXPORT)
#define eProsima_user_DllExport __declspec( dllexport )
#else
#define eProsima_user_DllExport
#endif
#else
#define eProsima_user_DllExport
#endif

#if defined(_WIN32)
#if defined(EPROSIMA_USER_DLL_EXPORT)
#if defined(Common_SOURCE)
#define Common_DllAPI __declspec( dllexport )
#else
#define Common_DllAPI __declspec( dllimport )
#endif // Common_SOURCE
#else
#define Common_DllAPI
#endif
#else
#define Common_DllAPI
#endif // _WIN32

namespace eprosima
{
    namespace fastcdr
    {
        class Cdr;
    }
}

namespace ib
{
    namespace mw
    {
        namespace idl
        {
            typedef uint16_t ParticipantIdT;
            typedef uint16_t EndpointIdT;
            /*!
             * @brief This class represents the structure EndpointAddress defined by the user in the IDL file.
             * @ingroup COMMON
             */
            class EndpointAddress
            {
            public:

                /*!
                 * @brief Default constructor.
                 */
                eProsima_user_DllExport EndpointAddress();

                /*!
                 * @brief Default destructor.
                 */
                eProsima_user_DllExport ~EndpointAddress();

                /*!
                 * @brief Copy constructor.
                 * @param x Reference to the object ib::mw::idl::EndpointAddress that will be copied.
                 */
                eProsima_user_DllExport EndpointAddress(const EndpointAddress &x);

                /*!
                 * @brief Move constructor.
                 * @param x Reference to the object ib::mw::idl::EndpointAddress that will be copied.
                 */
                eProsima_user_DllExport EndpointAddress(EndpointAddress &&x);

                /*!
                 * @brief Copy assignment.
                 * @param x Reference to the object ib::mw::idl::EndpointAddress that will be copied.
                 */
                eProsima_user_DllExport EndpointAddress& operator=(const EndpointAddress &x);

                /*!
                 * @brief Move assignment.
                 * @param x Reference to the object ib::mw::idl::EndpointAddress that will be copied.
                 */
                eProsima_user_DllExport EndpointAddress& operator=(EndpointAddress &&x);

                /*!
                 * @brief This function sets a value in member participantId
                 * @param _participantId New value for member participantId
                 */
                inline eProsima_user_DllExport void participantId(ib::mw::idl::ParticipantIdT _participantId)
                {
                    m_participantId = _participantId;
                }

                /*!
                 * @brief This function returns the value of member participantId
                 * @return Value of member participantId
                 */
                inline eProsima_user_DllExport ib::mw::idl::ParticipantIdT participantId() const
                {
                    return m_participantId;
                }

                /*!
                 * @brief This function returns a reference to member participantId
                 * @return Reference to member participantId
                 */
                inline eProsima_user_DllExport ib::mw::idl::ParticipantIdT& participantId()
                {
                    return m_participantId;
                }
                /*!
                 * @brief This function sets a value in member endpointId
                 * @param _endpointId New value for member endpointId
                 */
                inline eProsima_user_DllExport void endpointId(ib::mw::idl::EndpointIdT _endpointId)
                {
                    m_endpointId = _endpointId;
                }

                /*!
                 * @brief This function returns the value of member endpointId
                 * @return Value of member endpointId
                 */
                inline eProsima_user_DllExport ib::mw::idl::EndpointIdT endpointId() const
                {
                    return m_endpointId;
                }

                /*!
                 * @brief This function returns a reference to member endpointId
                 * @return Reference to member endpointId
                 */
                inline eProsima_user_DllExport ib::mw::idl::EndpointIdT& endpointId()
                {
                    return m_endpointId;
                }

                /*!
                 * @brief This function returns the maximum serialized size of an object
                 * depending on the buffer alignment.
                 * @param current_alignment Buffer alignment.
                 * @return Maximum serialized size.
                 */
                eProsima_user_DllExport static size_t getMaxCdrSerializedSize(size_t current_alignment = 0);

                /*!
                 * @brief This function returns the serialized size of a data depending on the buffer alignment.
                 * @param data Data which is calculated its serialized size.
                 * @param current_alignment Buffer alignment.
                 * @return Serialized size.
                 */
                eProsima_user_DllExport static size_t getCdrSerializedSize(const ib::mw::idl::EndpointAddress& data, size_t current_alignment = 0);


                /*!
                 * @brief This function serializes an object using CDR serialization.
                 * @param cdr CDR serialization object.
                 */
                eProsima_user_DllExport void serialize(eprosima::fastcdr::Cdr &cdr) const;

                /*!
                 * @brief This function deserializes an object using CDR serialization.
                 * @param cdr CDR serialization object.
                 */
                eProsima_user_DllExport void deserialize(eprosima::fastcdr::Cdr &cdr);



                /*!
                 * @brief This function returns the maximum serialized size of the Key of an object
                 * depending on the buffer alignment.
                 * @param current_alignment Buffer alignment.
                 * @return Maximum serialized size.
                 */
                eProsima_user_DllExport static size_t getKeyMaxCdrSerializedSize(size_t current_alignment = 0);

                /*!
                 * @brief This function tells you if the Key has been defined for this type
                 */
                eProsima_user_DllExport static bool isKeyDefined();

                /*!
                 * @brief This function serializes the key members of an object using CDR serialization.
                 * @param cdr CDR serialization object.
                 */
                eProsima_user_DllExport void serializeKey(eprosima::fastcdr::Cdr &cdr) const;

            private:
                ib::mw::idl::ParticipantIdT m_participantId;
                ib::mw::idl::EndpointIdT m_endpointId;
            };
        }
    }
}

#endif // _IB_MW_IDL_COMMON_H_