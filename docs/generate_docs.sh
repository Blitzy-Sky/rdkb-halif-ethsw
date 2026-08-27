#!/usr/bin/env bash

# *
# * If not stated otherwise in this file or this component's LICENSE file the
# * following copyright and licenses apply:
# *
# * Copyright 2023 RDK Management
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# * http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *

# In the future this should moved to a fixed verison
HAL_GENERATOR_VERSION=1.2.0

# This will look up the last tag in the git repo, depending on the project this may require modification
PROJECT_VERSION=$(git describe --tags | head -n1)

# Both conditional declarations of this interface must be extracted, so the documentation
# build predefines the two feature macros that guard them:
# CcspHalExtSw_ethPortConfigure() needs FEATURE_RDKB_WAN_MANAGER and
# FEATURE_RDKB_AUTO_PORT_SWITCH, and CcspHalExtSw_getCurrentWanHWConf() needs
# FEATURE_RDKB_AUTO_PORT_SWITCH. The value is written in Doxygen configuration syntax,
# not shell syntax: it is appended to the generator's configuration file verbatim, so a
# shell-quoted 'FEATURE...' would be read as a macro whose name begins with a quote.
DOXYGEN_EXTRA_PARAMS="PREDEFINED = FEATURE_RDKB_WAN_MANAGER=1 FEATURE_RDKB_AUTO_PORT_SWITCH=1"

# The pinned generator accepts DOXYGEN_EXTRA_PARAMS on its make command line and then
# discards it: its Makefile runs `doxygen Doxyfile.cfg` and never reads the variable, and
# that configuration leaves PREDEFINED empty. Without the flags Doxygen's preprocessor
# drops both #ifdef regions and the generated reference publishes eighteen of the twenty
# declared functions. The generator is maintained in another repository and consumed at a
# pinned revision, so the setting is applied here instead: a wrapper named `doxygen` is
# placed ahead of the real one on PATH and appends DOXYGEN_EXTRA_PARAMS to the
# configuration it is handed, which is where a later PREDEFINED assignment overrides the
# empty one. Remove this function, and the PATH it prepends, once the generator forwards
# DOXYGEN_EXTRA_PARAMS into its own Doxygen invocation.
apply_extra_doxygen_params() {
    local doxygen_bin
    doxygen_bin=$(command -v doxygen) || {
        echo "Doxygen is not found in the PATH:${PATH}, install it with ./build/doxygen_install.sh" >&2
        return 1
    }
    DOXYGEN_WRAPPER_DIR=$(mktemp -d) || return 1
    trap 'rm -rf "${DOXYGEN_WRAPPER_DIR}"' EXIT
    {
        echo '#!/usr/bin/env bash'
        printf 'doxygen_bin=%q\n' "${doxygen_bin}"
        printf 'extra_params=%q\n' "${DOXYGEN_EXTRA_PARAMS}"
        echo '# The generator invokes Doxygen with one configuration file; its Makefile'
        echo '# also probes the version with -v. Only the first form is augmented.'
        echo 'if [ "$#" -ne 1 ] || [ ! -f "$1" ]; then exec "${doxygen_bin}" "$@"; fi'
        echo '{ cat "$1"; printf "%s\n" "${extra_params}"; } | exec "${doxygen_bin}" -'
    } > "${DOXYGEN_WRAPPER_DIR}/doxygen" || return 1
    chmod +x "${DOXYGEN_WRAPPER_DIR}/doxygen" || return 1
    PATH="${DOXYGEN_WRAPPER_DIR}:${PATH}"
    export PATH
}
apply_extra_doxygen_params || exit 1

# Check if the common document configuration is present, if not clone it
if [ -d "./build" ]; then
    make -C ./build PROJECT_NAME="RDK-B EthSW HAL" PROJECT_VERSION=${PROJECT_VERSION} DOXYGEN_EXTRA_PARAMS="${DOXYGEN_EXTRA_PARAMS}"

else
    echo "Cloning Common documentation generation"
    git clone git@github.com:rdkcentral/hal-doxygen.git build
    cd ./build
    git checkout ${HAL_GENERATOR_VERSION}
    cd ..
    ./${0}
fi
