/**
 * SQL Batch execution implementation using default sqliteBridge implementation
*/
#include "OSPJSIHelper.h"
#include "sqliteBridge.h"

namespace osp {

namespace jsi = facebook::jsi;

struct QuickQueryArguments {
  string sql;
  shared_ptr<vector<QuickValue>> params;
};

/**
 * Local Helper method to translate JSI objects QuickQueryArguments datastructure
 * MUST be called in the JavaScript Thread
 */
void jsiBatchParametersToQuickArguments(jsi::Runtime &rt, jsi::Array const &batchParams, std::vector<QuickQueryArguments> *commands);

/**
 * Execute a batch of commands in a exclusive transaction
 */
SequelBatchOperationResult executeBatch(std::string dbName, std::vector<QuickQueryArguments> *commands);

} // namespace osp
