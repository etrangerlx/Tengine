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
# Scheduler module: base and advanced scheduler for different backends

set (_SCH_ROOT "${CMAKE_SOURCE_DIR}/source/scheduler")

# scheduler source files (explicit list, add new files here)
set (_SCH_LEVEL_SOURCE
    "${_SCH_ROOT}/scheduler.c"
)

# legacy variable export, kept for compatibility
set (TENGINE_SCHEDULER_SOURCE ${_SCH_LEVEL_SOURCE} CACHE INTERNAL "Tengine scheduler level source files" FORCE)

# IDE source groups
tengine_source_group ("scheduler" ${_SCH_LEVEL_SOURCE})

# module target
add_library (tengine_core_scheduler OBJECT ${_SCH_LEVEL_SOURCE})
target_link_libraries (tengine_core_scheduler PRIVATE tengine_build_config tengine_public_include)
