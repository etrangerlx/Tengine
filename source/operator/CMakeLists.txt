# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# License); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# Copyright (c) 2021, OPEN AI LAB
#
# Operator module: NN operator registration framework and prototype operators

SET (_OP_ROOT "${CMAKE_SOURCE_DIR}/source/operator")

# operator framework source files (explicit list, add new files here)
SET (_OP_LEVEL_SOURCE
    "${_OP_ROOT}/op.c"
)

# prototype operators: one source file per operator, plugin style directory.
# CONFIGURE_DEPENDS keeps the glob re-evaluated on rebuild.
FILE (GLOB _OP_PROTOTYPE_SOURCE CONFIGURE_DEPENDS "${_OP_ROOT}/prototype/*.c")


# legacy variable export, kept for compatibility with the device aggregator
# and standalone HCL builds
SET (TENGINE_OP_HEADER_PATH     "${_OP_ROOT}/prototype"                     CACHE INTERNAL "Tengine operator level header searching path"   FORCE)
SET (TENGINE_OPERATOR_SOURCE    ${_OP_LEVEL_SOURCE} ${_OP_PROTOTYPE_SOURCE} CACHE INTERNAL "Tengine operator level source files"            FORCE)


# IDE source groups
TENGINE_SOURCE_GROUP ("operator"            ${_OP_LEVEL_SOURCE})
TENGINE_SOURCE_GROUP ("operator/prototype"  ${_OP_PROTOTYPE_SOURCE})


# module target
ADD_LIBRARY (tengine_core_operator OBJECT ${_OP_LEVEL_SOURCE} ${_OP_PROTOTYPE_SOURCE})
TARGET_LINK_LIBRARIES (tengine_core_operator PRIVATE tengine_build_config tengine_public_include)
