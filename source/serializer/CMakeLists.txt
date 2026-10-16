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
# Serializer module: multi-framework model parsers, tmfile format

set (_SRL_SRC_ROOT "${CMAKE_SOURCE_DIR}/source/serializer")
set (_SRL_BIN_ROOT "${CMAKE_BINARY_DIR}/source/serializer")

# serializer framework and tmfile parser source files (explicit list, add new files here)
set (_SRL_LEVEL_SOURCE
    "${_SRL_SRC_ROOT}/serializer.c"
    "${_SRL_SRC_ROOT}/tmfile/tm2_serializer.c"
)

# tmfile per-op parameter parsers: one source file per operator, plugin style.
# CONFIGURE_DEPENDS keeps the glob re-evaluated on rebuild.
file (GLOB _SRL_TM2_OPS_SOURCE CONFIGURE_DEPENDS "${_SRL_SRC_ROOT}/tmfile/op/*.c")


# legacy variable export, kept for compatibility
set (TENGINE_SERIALIZER_HEADER_PATH "${_SRL_SRC_ROOT}" "${_SRL_BIN_ROOT}" CACHE INTERNAL "Tengine serializer level header searching path" FORCE)
set (TENGINE_SERIALIZER_SOURCE      ${_SRL_LEVEL_SOURCE} ${_SRL_TM2_OPS_SOURCE}     CACHE INTERNAL "Tengine serializer level source files"           FORCE)


# IDE source groups
tengine_source_group ("serializer"          ${_SRL_LEVEL_SOURCE})
tengine_source_group ("serializer/tmfile/op" ${_SRL_TM2_OPS_SOURCE})


# module target
add_library (tengine_core_serializer OBJECT ${_SRL_LEVEL_SOURCE} ${_SRL_TM2_OPS_SOURCE})
target_link_libraries (tengine_core_serializer PRIVATE tengine_build_config tengine_public_include)
