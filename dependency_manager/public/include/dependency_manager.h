/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * dependency_manager.h
 *
 *  \date       May 12, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DEPENDENCY_MANAGER_H_
#define DEPENDENCY_MANAGER_H_

#include "service_component.h"
#include "bundle_context.h"

struct dependencyManager {
	bundle_context_t context;
	array_list_t services;
};

typedef struct dependencyManager * DEPENDENCY_MANAGER;

DEPENDENCY_MANAGER dependencyManager_create(bundle_context_t context);
void dependencyManager_add(DEPENDENCY_MANAGER manager, SERVICE service);
void dependencyManager_remove(DEPENDENCY_MANAGER manager, SERVICE service);

void dm_startService(SERVICE service);
void dm_stopService(SERVICE service);

#endif /* DEPENDENCY_MANAGER_H_ */
