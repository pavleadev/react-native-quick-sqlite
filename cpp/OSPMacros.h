//
//  OSPMacros.h
//  Pods
//
//  Created by Oscar on 04.07.22.
//

#ifndef OSPMacros_h
#define OSPMacros_h

#define JSIF(capture)                                         \
capture(jsi::Runtime &runtime, const jsi::Value &thisValue, \
const jsi::Value *arguments, size_t count)          \
->jsi::Value

#endif /* OSPMacros_h */
