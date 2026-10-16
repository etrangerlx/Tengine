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

SET (_SCH_ROOT "${CMAKE_SOURCE_DIR}/source/scheduler")

# scheduler source files (explicit list, add new files here)
SET (_SCH_LEVEL_SOURCE
    "${_SCH_ROOT}/scheduler.c"
)

# legacy variable export, kept for compatibility
SET (TENGINE_SCHEDULER_SOURCE ${_SCH_LEVEL_SOURCE} CACHE INTERNAL "Tengine scheduler level source files" FORCE)

# IDE source groups
TENGINE_SOURCE_GROUP ("scheduler" ${_SCH_LEVEL_SOURCE})

# module target
ADD_LIBRARY (tengine_core_scheduler OBJECT ${_SCH_LEVEL_SOURCE})
TARGET_LINK_LIBRARIES (tengine_core_scheduler PRIVATE tengine_build_config tengine_public_include)
