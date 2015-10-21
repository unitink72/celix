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
 * dm_component.h
 *
 *  \date       8 Oct 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef COMPONENT_H_
#define COMPONENT_H_

#include <bundle_context.h>
#include <celix_errno.h>

#include "dm_service_dependency.h"

typedef struct dm_component *dm_component_pt;

#include "dm_component.h"

typedef celix_status_t (*init_fpt)(void *userData);
typedef celix_status_t (*start_fpt)(void *userData);
typedef celix_status_t (*stop_fpt)(void *userData);
typedef celix_status_t (*deinit_fpt)(void *userData);

celix_status_t component_create(bundle_context_pt context, dm_component_pt *component);
celix_status_t component_destroy(dm_component_pt *component);

celix_status_t component_addInterface(dm_component_pt component, char *serviceName, void *service, properties_pt properties);
celix_status_t component_setImplementation(dm_component_pt component, void *implementation);

/**
 * Returns an arraylist of service names. The caller owns the arraylist and strings (char *)
 */
celix_status_t component_getInterfaces(dm_component_pt component, array_list_pt *servicesNames);

celix_status_t component_addServiceDependency(dm_component_pt component, ...);
celix_status_t component_removeServiceDependency(dm_component_pt component, dm_service_dependency_pt dependency);

celix_status_t component_setCallbacks(dm_component_pt component, init_fpt init, start_fpt start, stop_fpt stop, deinit_fpt deinit);

/**
 * returns a dm_component_info_pt. Caller has ownership.
 */
celix_status_t component_getComponentInfo(dm_component_pt component, dm_component_info_pt *info);

#endif /* COMPONENT_H_ */
