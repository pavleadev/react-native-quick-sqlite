//
//  SmartHostObject.hpp
//  react-native-quick-sqlite
//
//  Created by Oscar on 29.06.22.
//

#ifndef SmartHostObject_hpp
#define SmartHostObject_hpp

#include <ReactCommon/TurboModuleUtils.h>
#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace osp {

namespace jsi = facebook::jsi;
namespace react = facebook::react;

typedef std::function<jsi::Value(jsi::Runtime &runtime)> JSIValueBuilder;

typedef std::pair<std::string, JSIValueBuilder> FieldDefinition;

FieldDefinition buildPair(std::string name, jsi::HostFunctionType &&f);

class JSI_EXPORT SmartHostObject : public jsi::HostObject {
public:
  SmartHostObject(std::shared_ptr<react::CallInvoker> jsCallInvoker)
      : weakJsCallInvoker(jsCallInvoker) {}

  virtual ~SmartHostObject() {}

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &runtime);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &propNameId);

  std::vector<std::pair<std::string, JSIValueBuilder>> fields;

protected:
  std::weak_ptr<react::CallInvoker> weakJsCallInvoker;
};

} // namespace osp

#endif /* SmartHostObject_hpp */
