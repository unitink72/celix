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
 * bundle_context.h
 *
 *  \date       Mar 26, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_CONTEXT_H_
#define BUNDLE_CONTEXT_H_

/**
 * A bundle's execution context within the Framework. The context is used to
 * grant access to other methods so that this bundle can interact with the
 * Framework.
 */
typedef struct bundleContext *BUNDLE_CONTEXT;

#include "service_factory.h"
#include "service_listener.h"
#include "bundle_listener.h"
#include "properties.h"
#include "array_list.h"

celix_status_t bundleContext_create(FRAMEWORK framework, BUNDLE bundle, BUNDLE_CONTEXT *bundle_context);
celix_status_t bundleContext_destroy(BUNDLE_CONTEXT context);

celix_status_t bundleContext_getBundle(BUNDLE_CONTEXT context, BUNDLE *bundle);
celix_status_t bundleContext_getFramework(BUNDLE_CONTEXT context, FRAMEWORK *framework);
celix_status_t bundleContext_getMemoryPool(BUNDLE_CONTEXT context, apr_pool_t **memory_pool);

celix_status_t bundleContext_installBundle(BUNDLE_CONTEXT context, char * location, BUNDLE *bundle);
celix_status_t bundleContext_installBundle2(BUNDLE_CONTEXT context, char * location, char *inputFile, BUNDLE *bundle);

celix_status_t bundleContext_registerService(BUNDLE_CONTEXT context, char * serviceName, void * svcObj,
        PROPERTIES properties, SERVICE_REGISTRATION *service_registration);
celix_status_t bundleContext_registerServiceFactory(BUNDLE_CONTEXT context, char * serviceName, service_factory_t factory,
        PROPERTIES properties, SERVICE_REGISTRATION *service_registration);

celix_status_t bundleContext_getServiceReferences(BUNDLE_CONTEXT context, const char * serviceName, char * filter, ARRAY_LIST *service_references);
celix_status_t bundleContext_getServiceReference(BUNDLE_CONTEXT context, char * serviceName, SERVICE_REFERENCE *service_reference);

celix_status_t bundleContext_getService(BUNDLE_CONTEXT context, SERVICE_REFERENCE reference, void **service_instance);
celix_status_t bundleContext_ungetService(BUNDLE_CONTEXT context, SERVICE_REFERENCE reference, bool *result);

celix_status_t bundleContext_getBundles(BUNDLE_CONTEXT context, ARRAY_LIST *bundles);
celix_status_t bundleContext_getBundleById(BUNDLE_CONTEXT context, long id, BUNDLE *bundle);

celix_status_t bundleContext_addServiceListener(BUNDLE_CONTEXT context, SERVICE_LISTENER listener, char * filter);
celix_status_t bundleContext_removeServiceListener(BUNDLE_CONTEXT context, SERVICE_LISTENER listener);

celix_status_t bundleContext_addBundleListener(BUNDLE_CONTEXT context, bundle_listener_t listener);
celix_status_t bundleContext_removeBundleListener(BUNDLE_CONTEXT context, bundle_listener_t listener);

celix_status_t bundleContext_getProperty(BUNDLE_CONTEXT context, const char *name, char **value);

#endif /* BUNDLE_CONTEXT_H_ */