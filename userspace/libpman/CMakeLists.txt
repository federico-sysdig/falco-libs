# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2023 The Falco Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
# in compliance with the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License
# is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
# or implied. See the License for the specific language governing permissions and limitations under
# the License.
#

add_compile_options(${FALCOSECURITY_LIBS_USERSPACE_COMPILE_FLAGS})
add_link_options(${FALCOSECURITY_LIBS_USERSPACE_LINK_FLAGS})

add_library(
	pman
	src/stats.c
	src/maps.c
	src/lifecycle.c
	src/programs.c
	src/ringbuffer.c
	src/configuration.c
	src/state.c
	src/sc_set.c
)

target_include_directories(
	pman
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
			$<BUILD_INTERFACE:${LIBS_DIR}> # ppm_enum and tables
			$<BUILD_INTERFACE:${LIBS_DIR}/userspace> # scap-stats struct
			${ZLIB_INCLUDE}
			${LIBBPF_INCLUDE}
			${MODERN_BPF_SKEL_DIR}
			${LIBELF_INCLUDE}
)

target_link_libraries(
	pman PUBLIC scap_event_schema scap_platform ${LIBBPF_LIB} ${LIBELF_LIB} ${ZLIB_LIB}
)

if(TARGET ProbeSkeleton)
	add_dependencies(pman ProbeSkeleton)
endif()

if(USE_BUNDLED_LIBBPF)
	add_dependencies(pman libbpf)
endif()

install(
	TARGETS pman
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
			COMPONENT "scap"
			OPTIONAL
)
