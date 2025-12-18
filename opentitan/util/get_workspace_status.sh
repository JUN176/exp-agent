#!/bin/bash
# Copyright lowRISC contributors (OpenTitan project).
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

# This script will be run by bazel when the build process wants to generate
# information about the status of the workspace.
#
# The output will be key-value pairs in the form:
# KEY1 VALUE1
# KEY2 VALUE2
#
# If this script exits with a non-zero exit code, it's considered as a failure
# and the output will be discarded.

# Try to get commit hash, fallback if Git is not available
git_rev=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
echo "BUILD_SCM_REVISION ${git_rev}"

# Try to get Git description (tag or short hash), fallback if unavailable
git_version=$(git describe --always --tags 2>/dev/null || echo "v0.0.0-nogit")
echo "BUILD_GIT_VERSION ${git_version}"

# Determine if tree is clean or modified, fallback to "clean"
if git diff-index --quiet HEAD -- 2>/dev/null; then
  tree_status="clean"
else
  tree_status="modified"
fi
echo "BUILD_SCM_STATUS ${tree_status}"
