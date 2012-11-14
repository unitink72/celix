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
 * version.h
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef VERSION_H_
#define VERSION_H_

#include <apr_general.h>

#include "celix_errno.h"

/**
 * @defgroup Version Version
 * @ingroup framework
 * @{
 */

/**
 * The definition of the VERSION abstract data type.
 */
typedef struct version * VERSION;

/**
 * Creates a new VERSION using the supplied arguments.
 *
 * @param pool The pool to create the version on.
 * @param major Major component of the version identifier.
 * @param minor Minor component of the version identifier.
 * @param micro Micro component of the version identifier.
 * @param qualifier Qualifier component of the version identifier. If
 *        <code>null</code> is specified, then the qualifier will be set to
 *        the empty string.
 * @param version The created VERSION
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ENOMEM If allocating memory for <code>version</code> failed.
 * 		- CELIX_ILLEGAL_ARGUMENT If the numerical components are negative
 * 		  or the qualifier string is invalid.
 */
celix_status_t version_createVersion(apr_pool_t *pool, int major, int minor, int micro, char * qualifier, VERSION *version);

/**
 * Creates a clone of <code>version</code> allocated on <code>pool</code>.
 *
 * @param version The version to clone
 * @param pool The pool in which the clone must be allocated
 * @param clone The cloned version
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ENOMEM If allocating memory for <code>version</code> failed.
 * 		- CELIX_ILLEGAL_ARGUMENT If the numerical components are negative
 * 		  or the qualifier string is invalid.
 */
celix_status_t version_clone(VERSION version, apr_pool_t *pool, VERSION *clone);

/**
 * Creates a version identifier from the specified string.
 *
 * <p>
 * Here is the grammar for version strings.
 *
 * <pre>
 * version ::= major('.'minor('.'micro('.'qualifier)?)?)?
 * major ::= digit+
 * minor ::= digit+
 * micro ::= digit+
 * qualifier ::= (alpha|digit|'_'|'-')+
 * digit ::= [0..9]
 * alpha ::= [a..zA..Z]
 * </pre>
 *
 * There must be no whitespace in version.
 *
 * @param pool The pool to create the version on.
 * @param versionStr String representation of the version identifier.
 * @param version The created VERSION
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ENOMEM If allocating memory for <code>version</code> failed.
 * 		- CELIX_ILLEGAL_ARGUMENT If the numerical components are negative,
 * 		  	the qualifier string is invalid or <code>versionStr</code> is impropertly formatted.
 */
celix_status_t version_createVersionFromString(apr_pool_t *pool, char * versionStr, VERSION *version);

/**
 * The empty version "0.0.0".
 *
 * @param pool The pool to create the version on.
 * @param version The created VERSION
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- CELIX_ENOMEM If allocating memory for <code>version</code> failed.
 * 		- CELIX_ILLEGAL_ARGUMENT If the numerical components are negative,
 * 		  	the qualifier string is invalid or <code>versionStr</code> is impropertly formatted.
 */
celix_status_t version_createEmptyVersion(apr_pool_t *pool, VERSION *version);

/**
 * Compares this <code>Version</code> object to another object.
 *
 * <p>
 * A version is considered to be <b>less than </b> another version if its
 * major component is less than the other version's major component, or the
 * major components are equal and its minor component is less than the other
 * version's minor component, or the major and minor components are equal
 * and its micro component is less than the other version's micro component,
 * or the major, minor and micro components are equal and it's qualifier
 * component is less than the other version's qualifier component (using
 * <code>String.compareTo</code>).
 *
 * <p>
 * A version is considered to be <b>equal to</b> another version if the
 * major, minor and micro components are equal and the qualifier component
 * is equal (using <code>String.compareTo</code>).
 *
 * @param version The <code>VERSION</code> to be compared with <code>compare</code>.
 * @param compare The <code>VERSION</code> to be compared with <code>version</code>.
 * @param result A negative integer, zero, or a positive integer if <code>version</code> is
 *         less than, equal to, or greater than the <code>compare</code>.
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 */
celix_status_t version_compareTo(VERSION version, VERSION compare, int *result);

/**
 * Returns the string representation of <code>version</code> identifier.
 *
 * <p>
 * The format of the version string will be <code>major.minor.micro</code>
 * if qualifier is the empty string or
 * <code>major.minor.micro.qualifier</code> otherwise.
 *
 * @return The string representation of this version identifier.
 * @param version The <code>VERSION</code> to get the string representation from.
 * @param pool The pool on which the string has to be allocated.
 * @param string Pointer to the string (char *) in which the result will be placed.
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 */
celix_status_t version_toString(VERSION version, apr_pool_t *pool, char **string);

/**
 * @}
 */

#endif /* VERSION_H_ */