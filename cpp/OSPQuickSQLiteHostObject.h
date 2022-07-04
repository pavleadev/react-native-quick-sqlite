//
//  OSPQuickSQLiteHostObject.hpp
//  react-native-quick-sqlite
//
//  Created by Oscar on 29.06.22.
//

#ifndef OSPQuickSQLiteHostObject_hpp
#define OSPQuickSQLiteHostObject_hpp

#include "OSPSmartHostObject.h"
#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <memory>
#include "OSPThreadPool.h""

namespace osp {
namespace jsi = facebook::jsi;
namespace react = facebook::react;

class JSI_EXPORT QuickSQLiteHostObject : public SmartHostObject {
public:
  explicit QuickSQLiteHostObject(
      std::shared_ptr<react::CallInvoker> jsCallInvoker, const char *docPath);

  virtual ~QuickSQLiteHostObject() {}

protected:
  const char *doc_path_;
  std::shared_ptr<ThreadPool> thread_pool_;
};

} // namespace osp

#endif /* OSPQuickSQLiteHostObject_hpp */
