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
 * service_registry.c
 *
 *  \date       Aug 6, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "service_registry.h"
#include "service_registration.h"
#include "module.h"
#include "bundle.h"
#include "listener_hook_service.h"
#include "constants.h"
#include "service_reference.h"
#include "framework.h"

struct serviceRegistry {
    framework_t framework;
	hash_map_t serviceRegistrations;
	hash_map_t serviceReferences;
	hash_map_t inUseMap;
	void (*serviceChanged)(framework_t, service_event_type_e, service_registration_t, properties_t);
	long currentServiceId;

	array_list_t listenerHooks;

	apr_thread_mutex_t * mutex;
};

struct usageCount {
	unsigned int count;
	service_reference_t reference;
	void * service;
};

typedef struct usageCount * USAGE_COUNT;

celix_status_t serviceRegistry_registerServiceInternal(service_registry_t registry, bundle_t bundle, char * serviceName, void * serviceObject, properties_t dictionary, bool isFactory, service_registration_t *registration);
celix_status_t serviceRegistry_addHooks(service_registry_t registry, char *serviceName, void *serviceObject, service_registration_t registration);
celix_status_t serviceRegistry_removeHook(service_registry_t registry, service_registration_t registration);

apr_status_t serviceRegistry_removeReference(void *referenceP);

USAGE_COUNT serviceRegistry_getUsageCount(service_registry_t registry, bundle_t bundle, service_reference_t reference) {
	array_list_t usages = (array_list_t) hashMap_get(registry->inUseMap, bundle);
	unsigned int i;
	for (i = 0; (usages != NULL) && (i < arrayList_size(usages)); i++) {
		USAGE_COUNT usage = (USAGE_COUNT) arrayList_get(usages, i);
		if (usage->reference == reference) {
			return usage;
		}
	}
	return NULL;
}

USAGE_COUNT serviceRegistry_addUsageCount(service_registry_t registry, bundle_t bundle, service_reference_t reference) {
	array_list_t usages = hashMap_get(registry->inUseMap, bundle);
	USAGE_COUNT usage = (USAGE_COUNT) malloc(sizeof(*usage));
	usage->reference = reference;
	usage->count = 0;
	usage->service = NULL;

	if (usages == NULL) {
		module_t mod = NULL;
		apr_pool_t *pool = NULL;
		bundle_getMemoryPool(bundle, &pool);
		arrayList_create(pool, &usages);
		bundle_getCurrentModule(bundle, &mod);
	}
	arrayList_add(usages, usage);
	hashMap_put(registry->inUseMap, bundle, usages);
	return usage;
}

void serviceRegistry_flushUsageCount(service_registry_t registry, bundle_t bundle, service_reference_t reference) {
	array_list_t usages = hashMap_get(registry->inUseMap, bundle);
	if (usages != NULL) {
		array_list_iterator_t iter = arrayListIterator_create(usages);
		while (arrayListIterator_hasNext(iter)) {
			USAGE_COUNT usage = arrayListIterator_next(iter);
			if (usage->reference == reference) {
				arrayListIterator_remove(iter);
				free(usage);
			}
		}
		arrayListIterator_destroy(iter);
		if (arrayList_size(usages) > 0) {
			hashMap_put(registry->inUseMap, bundle, usages);
		} else {
			array_list_t removed = hashMap_remove(registry->inUseMap, bundle);
			arrayList_destroy(removed);
		}
	}
}

service_registry_t serviceRegistry_create(framework_t framework, void (*serviceChanged)(framework_t, service_event_type_e, service_registration_t, properties_t)) {
	service_registry_t registry;
	apr_pool_t *pool = NULL;

	framework_getMemoryPool(framework, &pool);
	registry = (service_registry_t) apr_palloc(pool, (sizeof(*registry)));
	if (registry == NULL) {
	    // no memory
	} else {
		apr_status_t mutexattr;

        registry->serviceChanged = serviceChanged;
        registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
        registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);
        registry->framework = framework;

        arrayList_create(pool, &registry->listenerHooks);
        mutexattr = apr_thread_mutex_create(&registry->mutex, APR_THREAD_MUTEX_NESTED, pool);

        registry->currentServiceId = 1l;
	}
	return registry;
}

celix_status_t serviceRegistry_destroy(service_registry_t registry) {
    hashMap_destroy(registry->inUseMap, false, false);
    hashMap_destroy(registry->serviceRegistrations, false, false);
    arrayList_destroy(registry->listenerHooks);
    apr_thread_mutex_destroy(registry->mutex);

    return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_getRegisteredServices(service_registry_t registry, apr_pool_t *pool, bundle_t bundle, array_list_t *services) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_t regs = (array_list_t) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		unsigned int i;
		arrayList_create(pool, services);
		
		for (i = 0; i < arrayList_size(regs); i++) {
			service_registration_t reg = arrayList_get(regs, i);
			if (serviceRegistration_isValid(reg)) {
				// #todo Create SERVICE_REFERECEN for each registration
				service_reference_t reference = NULL;
				serviceRegistry_createServiceReference(registry, pool, reg, &reference);
				arrayList_add(*services, reference);
			}
		}
		return status;
	}
	return status;
}

service_registration_t serviceRegistry_registerService(service_registry_t registry, bundle_t bundle, char * serviceName, void * serviceObject, properties_t dictionary) {
    service_registration_t registration = NULL;
    serviceRegistry_registerServiceInternal(registry, bundle, serviceName, serviceObject, dictionary, false, &registration);
    return registration;
}

service_registration_t serviceRegistry_registerServiceFactory(service_registry_t registry, bundle_t bundle, char * serviceName, service_factory_t factory, properties_t dictionary) {
    service_registration_t registration = NULL;
    serviceRegistry_registerServiceInternal(registry, bundle, serviceName, (void *) factory, dictionary, true, &registration);
    return registration;
}

celix_status_t serviceRegistry_registerServiceInternal(service_registry_t registry, bundle_t bundle, char * serviceName, void * serviceObject, properties_t dictionary, bool isFactory, service_registration_t *registration) {
	array_list_t regs;
	apr_pool_t *pool = NULL;
	apr_thread_mutex_lock(registry->mutex);

	bundle_getMemoryPool(bundle, &pool);

	if (isFactory) {
	    *registration = serviceRegistration_createServiceFactory(pool, registry, bundle, serviceName, ++registry->currentServiceId, serviceObject, dictionary);
	} else {
	    *registration = serviceRegistration_create(pool, registry, bundle, serviceName, ++registry->currentServiceId, serviceObject, dictionary);
	}

	serviceRegistry_addHooks(registry, serviceName, serviceObject, *registration);

	regs = (array_list_t) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs == NULL) {
		regs = NULL;
		arrayList_create(pool, &regs);
	}
	arrayList_add(regs, *registration);
	hashMap_put(registry->serviceRegistrations, bundle, regs);

	apr_thread_mutex_unlock(registry->mutex);

	if (registry->serviceChanged != NULL) {
//		service_event_t event = (service_event_t) malloc(sizeof(*event));
//		event->type = REGISTERED;
//		event->reference = (*registration)->reference;
		registry->serviceChanged(registry->framework, SERVICE_EVENT_REGISTERED, *registration, NULL);
//		free(event);
//		event = NULL;
	}

	return CELIX_SUCCESS;
}

void serviceRegistry_unregisterService(service_registry_t registry, bundle_t bundle, service_registration_t registration) {
	// array_list_t clients;
	unsigned int i;
	array_list_t regs;
	array_list_t references = NULL;

	apr_thread_mutex_lock(registry->mutex);

	serviceRegistry_removeHook(registry, registration);

	regs = (array_list_t) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		arrayList_removeElement(regs, registration);
		hashMap_put(registry->serviceRegistrations, bundle, regs);
	}

	apr_thread_mutex_unlock(registry->mutex);

	if (registry->serviceChanged != NULL) {
		registry->serviceChanged(registry->framework, SERVICE_EVENT_UNREGISTERING, registration, NULL);
	}

	apr_thread_mutex_lock(registry->mutex);
	// unget service

	serviceRegistration_getServiceReferences(registration, &references);
	for (i = 0; i < arrayList_size(references); i++) {
		service_reference_t reference = (service_reference_t) arrayList_get(references, i);
		apr_pool_t *pool = NULL;
		array_list_t clients = NULL;
		unsigned int j;

		framework_getMemoryPool(registry->framework, &pool);
		clients = serviceRegistry_getUsingBundles(registry, pool, reference);
		for (j = 0; (clients != NULL) && (j < arrayList_size(clients)); j++) {
			bundle_t client = (bundle_t) arrayList_get(clients, j);
			while (serviceRegistry_ungetService(registry, client, reference)) {
				;
			}
		}
		arrayList_destroy(clients);

		serviceReference_invalidate(reference);
	}
	serviceRegistration_invalidate(registration);

	serviceRegistration_destroy(registration);

	apr_thread_mutex_unlock(registry->mutex);

}

void serviceRegistry_unregisterServices(service_registry_t registry, bundle_t bundle) {
	array_list_t regs = NULL;
	unsigned int i;
	apr_thread_mutex_lock(registry->mutex);
	regs = (array_list_t) hashMap_get(registry->serviceRegistrations, bundle);
	apr_thread_mutex_unlock(registry->mutex);
	
	for (i = 0; (regs != NULL) && i < arrayList_size(regs); i++) {
		service_registration_t reg = arrayList_get(regs, i);
		if (serviceRegistration_isValid(reg)) {
			serviceRegistration_unregister(reg);
		}
	}

	if (regs != NULL && arrayList_isEmpty(regs)) {
		array_list_t removed = hashMap_remove(registry->serviceRegistrations, bundle);
		arrayList_destroy(removed);
		removed = NULL;
	}

	apr_thread_mutex_lock(registry->mutex);
	hashMap_remove(registry->serviceRegistrations, bundle);
	apr_thread_mutex_unlock(registry->mutex);
}

celix_status_t serviceRegistry_createServiceReference(service_registry_t registry, apr_pool_t *pool, service_registration_t registration, service_reference_t *reference) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_t bundle = NULL;
	array_list_t references = NULL;

	apr_pool_t *spool = NULL;
	apr_pool_create(&spool, pool);

	serviceRegistration_getBundle(registration, &bundle);
	serviceReference_create(spool, bundle, registration, reference);

	apr_pool_pre_cleanup_register(spool, *reference, serviceRegistry_removeReference);

	serviceRegistration_getServiceReferences(registration, &references);
	arrayList_add(references, *reference);

	return status;
}

celix_status_t serviceRegistry_getServiceReferences(service_registry_t registry, apr_pool_t *pool, const char *serviceName, filter_t filter, array_list_t *references) {
	celix_status_t status = CELIX_SUCCESS;
	hash_map_values_t registrations;
	hash_map_iterator_t iterator;
	arrayList_create(pool, references);

	registrations = hashMapValues_create(registry->serviceRegistrations);
	iterator = hashMapValues_iterator(registrations);
	while (hashMapIterator_hasNext(iterator)) {
		array_list_t regs = (array_list_t) hashMapIterator_nextValue(iterator);
		unsigned int regIdx;
		for (regIdx = 0; (regs != NULL) && regIdx < arrayList_size(regs); regIdx++) {
			service_registration_t registration = (service_registration_t) arrayList_get(regs, regIdx);
			properties_t props = NULL;

			status = serviceRegistration_getProperties(registration, &props);
			if (status == CELIX_SUCCESS) {
				bool matched = false;
				bool matchResult = false;
				if (filter != NULL) {
					filter_match(filter, props, &matchResult);
				}
				if ((serviceName == NULL) && ((filter == NULL) || matchResult)) {
					matched = true;
				} else if (serviceName != NULL) {
					char *className = NULL;
					bool matchResult = false;
					serviceRegistration_getServiceName(registration, &className);
					if (filter != NULL) {
						filter_match(filter, props, &matchResult);
					}
					if ((strcmp(className, serviceName) == 0) && ((filter == NULL) || matchResult)) {
						matched = true;
					}
				}
				if (matched) {
					if (serviceRegistration_isValid(registration)) {
						service_reference_t reference = NULL;
						serviceRegistry_createServiceReference(registry, pool, registration, &reference);
						arrayList_add(*references, reference);
					}
				}
			}
		}
	}
	hashMapIterator_destroy(iterator);
	hashMapValues_destroy(registrations);

	return status;
}

apr_status_t serviceRegistry_removeReference(void *referenceP) {
	service_reference_t reference = referenceP;
	service_registration_t registration = NULL;
	serviceReference_getServiceRegistration(reference, &registration);

	if (registration != NULL) {
		array_list_t references = NULL;
		serviceRegistration_getServiceReferences(registration, &references);
		arrayList_removeElement(references, reference);
	}

	return APR_SUCCESS;
}

array_list_t serviceRegistry_getServicesInUse(service_registry_t registry, bundle_t bundle) {
	array_list_t usages = hashMap_get(registry->inUseMap, bundle);
	if (usages != NULL) {
		unsigned int i;
		array_list_t references = NULL;
		apr_pool_t *pool = NULL;
		bundle_getMemoryPool(bundle, &pool);
		arrayList_create(pool, &references);
		
		for (i = 0; i < arrayList_size(usages); i++) {
			USAGE_COUNT usage = arrayList_get(usages, i);
			arrayList_add(references, usage->reference);
		}
		return references;
	}
	return NULL;
}

void * serviceRegistry_getService(service_registry_t registry, bundle_t bundle, service_reference_t reference) {
	service_registration_t registration = NULL;
	void * service = NULL;
	USAGE_COUNT usage = NULL;
	serviceReference_getServiceRegistration(reference, &registration);
	
	apr_thread_mutex_lock(registry->mutex);

	if (serviceRegistration_isValid(registration)) {
		usage = serviceRegistry_getUsageCount(registry, bundle, reference);
		if (usage == NULL) {
			usage = serviceRegistry_addUsageCount(registry, bundle, reference);
		}
		usage->count++;
		service = usage->service;
	}
	apr_thread_mutex_unlock(registry->mutex);

	if ((usage != NULL) && (service == NULL)) {
		serviceRegistration_getService(registration, bundle, &service);
	}
	apr_thread_mutex_lock(registry->mutex);
	if ((!serviceRegistration_isValid(registration)) || (service == NULL)) {
		serviceRegistry_flushUsageCount(registry, bundle, reference);
	} else {
		usage->service = service;
	}
	apr_thread_mutex_unlock(registry->mutex);

	return service;
}

bool serviceRegistry_ungetService(service_registry_t registry, bundle_t bundle, service_reference_t reference) {
	service_registration_t registration = NULL;
	USAGE_COUNT usage = NULL;
	serviceReference_getServiceRegistration(reference, &registration);
	
	apr_thread_mutex_lock(registry->mutex);

	usage = serviceRegistry_getUsageCount(registry, bundle, reference);
	if (usage == NULL) {
		apr_thread_mutex_unlock(registry->mutex);
		return false;
	}

	usage->count--;


	if ((serviceRegistration_isValid(registration)) || (usage->count <= 0)) {
		usage->service = NULL;
		serviceRegistry_flushUsageCount(registry, bundle, reference);
	}

	apr_thread_mutex_unlock(registry->mutex);

	return true;
}

void serviceRegistry_ungetServices(service_registry_t registry, bundle_t bundle) {
	array_list_t fusages;
	array_list_t usages;
	unsigned int i;

	apr_pool_t *pool = NULL;
	bundle_getMemoryPool(bundle, &pool);

	apr_thread_mutex_lock(registry->mutex);
	usages = hashMap_get(registry->inUseMap, bundle);
	apr_thread_mutex_unlock(registry->mutex);

	if (usages == NULL || arrayList_isEmpty(usages)) {
		return;
	}

	// usage arrays?
	fusages = arrayList_clone(pool, usages);
	
	for (i = 0; i < arrayList_size(fusages); i++) {
		USAGE_COUNT usage = arrayList_get(fusages, i);
		service_reference_t reference = usage->reference;
		while (serviceRegistry_ungetService(registry, bundle, reference)) {
			//
		}
	}

	arrayList_destroy(fusages);
}

array_list_t serviceRegistry_getUsingBundles(service_registry_t registry, apr_pool_t *pool, service_reference_t reference) {
	array_list_t bundles = NULL;
	hash_map_iterator_t iter;
	apr_pool_t *npool;
	apr_pool_create(&npool, pool);
	arrayList_create(npool, &bundles);
	iter = hashMapIterator_create(registry->inUseMap);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_t entry = hashMapIterator_nextEntry(iter);
		bundle_t bundle = hashMapEntry_getKey(entry);
		array_list_t usages = hashMapEntry_getValue(entry);
		unsigned int i;
		for (i = 0; i < arrayList_size(usages); i++) {
			USAGE_COUNT usage = arrayList_get(usages, i);
			if (usage->reference == reference) {
				arrayList_add(bundles, bundle);
			}
		}
	}
	hashMapIterator_destroy(iter);
	return bundles;
}

celix_status_t serviceRegistry_addHooks(service_registry_t registry, char *serviceName, void *serviceObject, service_registration_t registration) {
	celix_status_t status = CELIX_SUCCESS;

	if (strcmp(listener_hook_service_name, serviceName) == 0) {
		arrayList_add(registry->listenerHooks, registration);
	}

	return status;
}

celix_status_t serviceRegistry_removeHook(service_registry_t registry, service_registration_t registration) {
	celix_status_t status = CELIX_SUCCESS;
	char *serviceName = NULL;

	properties_t props = NULL;
	serviceRegistration_getProperties(registration, &props);
	serviceName = properties_get(props, (char *) OBJECTCLASS);
	if (strcmp(listener_hook_service_name, serviceName) == 0) {
		arrayList_removeElement(registry->listenerHooks, registration);
	}

	return status;
}

celix_status_t serviceRegistry_getListenerHooks(service_registry_t registry, apr_pool_t *pool, array_list_t *hooks) {
	celix_status_t status = CELIX_SUCCESS;

	if (registry == NULL || *hooks != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		status = arrayList_create(pool, hooks);
		if (status == CELIX_SUCCESS) {
			unsigned int i;
			for (i = 0; i < arrayList_size(registry->listenerHooks); i++) {
				service_registration_t registration = arrayList_get(registry->listenerHooks, i);
				service_reference_t reference = NULL;
				serviceRegistry_createServiceReference(registry, pool, registration, &reference);
				arrayList_add(*hooks, reference);
			}
		}
	}

	return status;
}

celix_status_t serviceRegistry_servicePropertiesModified(service_registry_t registry, service_registration_t registration, properties_t oldprops) {
	if (registry->serviceChanged != NULL) {
		registry->serviceChanged(registry->framework, SERVICE_EVENT_MODIFIED, registration, oldprops);
	}

	return CELIX_SUCCESS;
}
