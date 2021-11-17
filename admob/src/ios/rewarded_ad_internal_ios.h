/*
 * Copyright 2021 Google LLC
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

#ifndef FIREBASE_ADMOB_SRC_IOS_REWARDED_AD_INTERNAL_IOS_H_
#define FIREBASE_ADMOB_SRC_IOS_REWARDED_AD_INTERNAL_IOS_H_

extern "C" {
#include <objc/objc.h>
}  // extern "C"

#include "admob/src/common/rewarded_ad_internal.h"
#include "app/src/mutex.h"

namespace firebase {
namespace admob {
namespace internal {

class RewardedAdInternalIOS : public RewardedAdInternal {
 public:
  explicit RewardedAdInternalIOS(RewardedAd* base);
  ~RewardedAdInternalIOS();

  Future<void> Initialize(AdParent parent) override;
  Future<AdResult> LoadAd(const char* ad_unit_id,
                          const AdRequest& request) override;
  Future<void> Show(UserEarnedRewardListener* listener) override;
  bool is_initialized() const override { return initialized_; }

 private:
  /// Prevents duplicate invocations of initailize on the Interstitial Ad.
  bool initialized_;

  /// Contains information to asynchronously complete the LoadAd Future.
  FutureCallbackData<AdResult>* ad_load_callback_data_;

  /// The GADRewardedAd object. Declared as an "id" type to avoid
  /// referencing an Objective-C class in this header.
  id rewarded_ad_;

  /// The publisher-provided view (UIView) that's the parent view of the
  /// rewarded ad. Declared as an "id" type to avoid referencing an
  /// Objective-C class in this header.
  id parent_view_;

  /// The delegate object to listen for callbacks. Declared as an "id"
  /// type to avoid referencing an Objective-C++ class in this header.
  id rewarded_ad_delegate_;

  // Mutex to guard against concurrent operations;
  Mutex mutex_;
};

}  // namespace internal
}  // namespace admob
}  // namespace firebase

#endif  // FIREBASE_ADMOB_SRC_IOS_REWARDED_AD_INTERNAL_IOS_H_
