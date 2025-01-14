/*
 * Copyright 2017 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "app/src/thread.h"

#include "app/src/mutex.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define EXPECT_THROW_ERROR_CODE(statement, error_code)               \
  EXPECT_THROW(                                                      \
      {                                                              \
        try {                                                        \
          GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(statement); \
        } catch (const std::system_error& exception) {               \
          EXPECT_EQ(error_code, exception.code());                   \
          throw;                                                     \
        }                                                            \
      },                                                             \
      std::system_error)

namespace {

using ::testing::Eq;

// Simple thread safe wrapper around a value T.
template <typename T>
class ThreadSafe {
 public:
  explicit ThreadSafe(T value) : value_(value) {}

  T get() const {
    firebase::MutexLock lock(const_cast<firebase::Mutex&>(mtx_));
    return value_;
  }

  void set(const T& value) {
    firebase::MutexLock lock(mtx_);
    value_ = value;
  }

 private:
  T value_;
  firebase::Mutex mtx_;
};

TEST(ThreadTest, ThreadExecutesAndJoinWaitsForItToFinish) {
  ThreadSafe<bool> value(false);

  firebase::Thread thread([](ThreadSafe<bool>* value) { value->set(true); },
                          &value);
  thread.Join();

  ASSERT_THAT(value.get(), Eq(true));
}

TEST(ThreadTest, ThreadIsNotJoinableAfterJoin) {
  firebase::Thread thread([] {});
  ASSERT_THAT(thread.Joinable(), Eq(true));

  thread.Join();
  ASSERT_THAT(thread.Joinable(), Eq(false));
}

TEST(ThreadTest, ThreadIsNotJoinableAfterDetach) {
  firebase::Thread thread([] {});
  ASSERT_THAT(thread.Joinable(), Eq(true));

  thread.Detach();
  ASSERT_THAT(thread.Joinable(), Eq(false));
}

TEST(ThreadTest, ThreadShouldNotBeJoinableAfterBeingMoveAssignedOutOf) {
  firebase::Thread source([] {});
  firebase::Thread target;

  ASSERT_THAT(source.Joinable(), Eq(true));

  // cast due to lack of std::move in STLPort
  target = static_cast<firebase::Thread&&>(source);
  ASSERT_THAT(source.Joinable(), Eq(false));
  ASSERT_THAT(target.Joinable(), Eq(true));
  target.Join();
}

TEST(ThreadTest, ThreadShouldNotBeJoinableAfterBeingMoveFrom) {
  firebase::Thread source([] {});

  ASSERT_THAT(source.Joinable(), Eq(true));

  // cast due to lack of std::move in STLPort
  firebase::Thread target(static_cast<firebase::Thread&&>(source));
  ASSERT_THAT(source.Joinable(), Eq(false));
  ASSERT_THAT(target.Joinable(), Eq(true));
  target.Join();
}

TEST(ThreadDeathTest, MovingIntoRunningThreadShouldAbort) {
  ASSERT_DEATH(
      {
        firebase::Thread thread([] {});

        thread = firebase::Thread();
      },
      "");
}

TEST(ThreadDeathTest, JoinEmptyThreadShouldAbort) {
  EXPECT_THROW_ERROR_CODE(
      {
        firebase::Thread thread;
        thread.Join();
      },
      std::errc::invalid_argument);
}

TEST(ThreadDeathTest, JoinThreadMultipleTimesShouldAbort) {
  EXPECT_THROW_ERROR_CODE(
      {
        firebase::Thread thread([] {});

        thread.Join();
        thread.Join();
      },
      std::errc::invalid_argument);
}

TEST(ThreadDeathTest, JoinDetachedThreadShouldAbort) {
  EXPECT_THROW_ERROR_CODE(
      {
        firebase::Thread thread([] {});

        thread.Detach();
        thread.Join();
      },
      std::errc::invalid_argument);
}

TEST(ThreadDeathTest, DetachJoinedThreadShouldAbort) {
  EXPECT_THROW_ERROR_CODE(
      {
        firebase::Thread thread([] {});

        thread.Join();
        thread.Detach();
      },
      std::errc::invalid_argument);
}

TEST(ThreadDeathTest, DetachEmptyThreadShouldAbort) {
  EXPECT_THROW_ERROR_CODE(
      {
        firebase::Thread thread;

        thread.Detach();
      },
      std::errc::invalid_argument);
}

TEST(ThreadDeathTest, DetachThreadMultipleTimesShouldAbort) {
  EXPECT_THROW_ERROR_CODE(
      {
        firebase::Thread thread([] {});

        thread.Detach();
        thread.Detach();
      },
      std::errc::invalid_argument);
}

TEST(ThreadDeathTest, WhenJoinableThreadIsDestructedShouldAbort) {
  ASSERT_DEATH({ firebase::Thread thread([] {}); }, "");
}

TEST(ThreadTest, ThreadIsEqualToItself) {
  firebase::Thread::Id thread_id = firebase::Thread::CurrentId();
  ASSERT_THAT(firebase::Thread::IsCurrentThread(thread_id), Eq(true));
}

TEST(ThreadTest, ThreadIsNotEqualToDifferentThread) {
  ThreadSafe<firebase::Thread::Id> value(firebase::Thread::CurrentId());

  firebase::Thread thread(
      [](ThreadSafe<firebase::Thread::Id>* value) {
        value->set(firebase::Thread::CurrentId());
      },
      &value);
  thread.Join();

  ASSERT_THAT(firebase::Thread::IsCurrentThread(value.get()), Eq(false));
}

}  // namespace
