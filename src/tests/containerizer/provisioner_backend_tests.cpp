/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <process/gtest.hpp>

#include <stout/foreach.hpp>
#include <stout/gtest.hpp>
#include <stout/os.hpp>
#include <stout/os/permissions.hpp>
#include <stout/path.hpp>
#include <stout/strings.hpp>

#ifdef __linux__
#include "linux/fs.hpp"
#endif // __linux__

#include "slave/containerizer/provisioners/backends/bind.hpp"

#include "tests/flags.hpp"
#include "tests/utils.hpp"

using namespace process;

using namespace mesos::internal::slave;

using std::string;
using std::vector;

namespace mesos {
namespace internal {
namespace tests {

#ifdef __linux__
class BindBackendTest : public TemporaryDirectoryTest
{
protected:
  void TearDown()
  {
    // Clean up by unmounting any leftover mounts in 'sandbox'.
    Try<fs::MountInfoTable> mountTable = fs::MountInfoTable::read();
    ASSERT_SOME(mountTable);

    // TODO(xujyan): Make sandbox a plain string instead of an option.
    ASSERT_SOME(sandbox);
    foreach (const fs::MountInfoTable::Entry& entry, mountTable.get().entries) {
      if (strings::startsWith(entry.target, sandbox.get())) {
        fs::unmount(entry.target, MNT_DETACH);
      }
    }

    TemporaryDirectoryTest::TearDown();
  }
};


// Provision a rootfs using a BindBackend to another directory and
// verify if it is read-only within the mount.
TEST_F(BindBackendTest, ROOT_BindBackend)
{
  string rootfs = path::join(os::getcwd(), "source");

  // Create a writable directory under the dummy rootfs.
  Try<Nothing> mkdir = os::mkdir(path::join(rootfs, "tmp"));
  ASSERT_SOME(mkdir);

  hashmap<string, Owned<Backend>> backends = Backend::create(slave::Flags());
  ASSERT_TRUE(backends.contains("bind"));

  string target = path::join(os::getcwd(), "target");

  AWAIT_READY(backends["bind"]->provision({rootfs}, target));

  EXPECT_TRUE(os::stat::isdir(path::join(target, "tmp")));

  // 'target' _appears_ to be writable but is really not due to read-only mount.
  Try<mode_t> mode = os::stat::mode(path::join(target, "tmp"));
  ASSERT_SOME(mode);
  EXPECT_TRUE(os::Permissions(mode.get()).owner.w);
  EXPECT_ERROR(os::write(path::join(target, "tmp", "test"), "data"));

  AWAIT_READY(backends["bind"]->destroy(target));

  EXPECT_FALSE(os::exists(target));
}
#endif // __linux__

} // namespace tests {
} // namespace internal {
} // namespace mesos {
