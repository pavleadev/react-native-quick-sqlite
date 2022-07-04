/*
 * Sequel.mm
 *
 * Created by Oscar Franco on 2021/03/07
 * Copyright (c) 2021 Oscar Franco
 *
 * This code is licensed under the MIT license
 */

#import "QuickSQLite.h"

#import <React/RCTBridge+Private.h>
#import <React/RCTUtils.h>
#import <ReactCommon/RCTTurboModule.h>
#import <jsi/jsi.h>

#import "../cpp/OSPQuickSQLiteHostObject.h"

@implementation QuickSQLite

RCT_EXPORT_MODULE(QuickSQLite)

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install) {
  NSLog(@"Installing QuickSQLite module...");

  RCTBridge *bridge = [RCTBridge currentBridge];
  RCTCxxBridge *cxxBridge = (RCTCxxBridge *)bridge;
  if (cxxBridge == nil) {
    return @false;
  }

  using namespace facebook;

  auto jsiRuntime = (jsi::Runtime *)cxxBridge.runtime;
  if (jsiRuntime == nil) {
    return @false;
  }
  auto &runtime = *jsiRuntime;
  auto callInvoker = bridge.jsCallInvoker;

  // Get iOS app's document directory (to safely store database .sqlite3 file)
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, true);
  NSString *documentPath = [paths objectAtIndex:0];

  auto hostObject = std::static_pointer_cast<jsi::HostObject>(
      std::make_shared<osp::QuickSQLiteHostObject>(callInvoker, [documentPath UTF8String]));
  auto object = jsi::Object::createFromHostObject(runtime, hostObject);

  runtime.global().setProperty(runtime, "__QuickSQLiteProxy",
                               std::move(object));

  NSLog(@"Successfully installed JSI bindings for react-native-quick-sqlite!");
  return @true;
}

// RCT_EXPORT_MODULE()

// + (BOOL)requiresMainQueueSetup {
//   return YES;
// }

// - (void)setBridge:(RCTBridge *)bridge {
//   _bridge = bridge;
//   _setBridgeOnMainQueue = RCTIsMainQueue();

//   RCTCxxBridge *cxxBridge = (RCTCxxBridge *)self.bridge;
//   if (!cxxBridge.runtime) {
//     return;
//   }

//   auto callInvoker = bridge.jsCallInvoker;

//   // Get iOS app's document directory (to safely store database .sqlite3
//   file) NSArray *paths =
//   NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask,
//   true); NSString *documentPath = [paths objectAtIndex:0];

//   install(*(facebook::jsi::Runtime *)cxxBridge.runtime,
//   callInvoker,[documentPath UTF8String]);
// }

@end
