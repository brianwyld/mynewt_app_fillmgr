/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/* Fill management */
#ifndef H_FILLMGR_H
#define H_FILLMGR_H
#include <stdint.h>

// Use the PTI module id, as won't have both at same time
#define MY_MOD_ID   (APP_MOD_PTI)

// define our specific ul tags that only our app needs to decode
#define UL_APP_FILLMGR_RANGE (APP_CORE_UL_APP_SPECIFIC_START)
#define UL_APP_FILLMGR_STATE (APP_CORE_UL_APP_SPECIFIC_START+1)

typedef enum { FILL_STATE_HS, FILL_STATE_DISABLED, FILL_STATE_EMPTY, FILL_STATE_INTERMEDIATE, FILL_STATE_FULL } FILLMGR_STATE_t;

#endif /* H_FILLMGR_H */