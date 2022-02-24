/**
 * Copyright 2020 Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-.amazon.com.-AmznSL-1.0
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 * http://aws.amazon.com/asl/
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "PayloadManager.h"
#include "AwsIotChannel.h"
#include "AwsIotConnectivityModule.h"
#include <cstring>
#include <functional>
#include <gtest/gtest.h>
#include <list>

using namespace Aws::IoTFleetWise::Platform::PersistencyManagement;
using namespace Aws::IoTFleetWise::OffboardConnectivity;
using namespace Aws::IoTFleetWise::OffboardConnectivityAwsIot;
using Aws::IoTFleetWise::OffboardConnectivity::CollectionSchemeParams;

TEST( PayloadManagerTest, TestNoConnectionDataPersistency )
{
    char buffer[PATH_MAX];
    if ( getcwd( buffer, sizeof( buffer ) ) != NULL )
    {
        const std::shared_ptr<CacheAndPersist> persistencyPtr =
            std::make_shared<CacheAndPersist>( std::string( buffer ), 131072 );
        persistencyPtr->init();
        persistencyPtr->erase( EDGE_TO_CLOUD_PAYLOAD );
        PayloadManager testSend( persistencyPtr );

        std::string testData = "abcdefjh!24$iklmnop!24$3@qaabcdefjh!24$iklmnop!24$3@qaabcdefjh!24$iklmnop!24$3@qabbbb";
        testData +=
            '\0'; // make sure compression can handle null characters and does not stop after the first null character
        testData += "testproto";
        size_t size = testData.size();
        const uint8_t *stringData = reinterpret_cast<const uint8_t *>( testData.data() );

        CollectionSchemeParams collectionSchemeParams;
        collectionSchemeParams.persist = true;
        collectionSchemeParams.compression = false;
        collectionSchemeParams.priority = 0;

        ASSERT_EQ( testSend.storeData( stringData, size, collectionSchemeParams ), true );

        std::vector<std::string> payloads;
        testSend.retrieveData( payloads );
        ASSERT_EQ( payloads.size(), 1 );

        ASSERT_TRUE( 0 == std::memcmp( payloads[0].c_str(), testData.c_str(), testData.size() ) );
        persistencyPtr->erase( EDGE_TO_CLOUD_PAYLOAD );
    }
}

TEST( PayloadManagerTest, TestNoConnectionNoDataPersistency )
{
    char buffer[PATH_MAX];
    if ( getcwd( buffer, sizeof( buffer ) ) != NULL )
    {
        const std::shared_ptr<CacheAndPersist> persistencyPtr =
            std::make_shared<CacheAndPersist>( std::string( buffer ), 131072 );
        persistencyPtr->init();
        persistencyPtr->erase( EDGE_TO_CLOUD_PAYLOAD );
        PayloadManager testSend( persistencyPtr );

        std::string testData = "abcdefjh!24$^^77@p!24$3@";
        size_t size = testData.size();
        uint8_t *buf = new uint8_t[size];
        memcpy( buf, &testData, size );
        CollectionSchemeParams collectionSchemeParams;
        collectionSchemeParams.persist = false;
        collectionSchemeParams.compression = false;
        collectionSchemeParams.priority = 0;

        ASSERT_FALSE( testSend.storeData( buf, size, collectionSchemeParams ) );
        persistencyPtr->erase( EDGE_TO_CLOUD_PAYLOAD );
        delete[] buf;
    }
}

TEST( PayloadManagerTest, TestCompression )
{
    char buffer[PATH_MAX];
    if ( getcwd( buffer, sizeof( buffer ) ) != NULL )
    {
        const std::shared_ptr<CacheAndPersist> persistencyPtr =
            std::make_shared<CacheAndPersist>( std::string( buffer ), 131072 );
        persistencyPtr->init();
        persistencyPtr->erase( EDGE_TO_CLOUD_PAYLOAD );
        PayloadManager testSend( persistencyPtr );

        std::string testData = "abcdefjh!24$iklmnop!24$3@qrstuvwxyz";

        CollectionSchemeParams collectionSchemeParams;
        collectionSchemeParams.persist = true;
        collectionSchemeParams.compression = true;
        collectionSchemeParams.priority = 0;

        std::string payloadData;
        ASSERT_TRUE( snappy::Compress( testData.data(), testData.size(), &payloadData ) );

        ASSERT_EQ( testSend.storeData( reinterpret_cast<const uint8_t *>( payloadData.c_str() ),
                                       payloadData.size(),
                                       collectionSchemeParams ),
                   true );

        std::vector<std::string> payloads;
        testSend.retrieveData( payloads );
        ASSERT_EQ( payloads.size(), 1 );
        ASSERT_STRNE( payloads[0].c_str(), testData.c_str() );

        ASSERT_TRUE( snappy::Uncompress( payloads[0].c_str(), payloads[0].size(), &payloadData ) );
        ASSERT_STREQ( payloadData.c_str(), testData.c_str() );

        persistencyPtr->erase( EDGE_TO_CLOUD_PAYLOAD );
    }
}
