#include "OSPQuickSQLiteHostObject.h"
#include "OSPMacros.h"
#include "OSPJSIHelper.h"
#include <string>
#include "sqliteBridge.h"
#include "OSPSQLBatchExecutor.h"
#include "OSPSQLFileLoader.h"
#include <vector>

namespace osp {
namespace jsi = facebook::jsi;
namespace react = facebook::react;

jsi::Object createError(jsi::Runtime &runtime, std::string message)
{
  auto res = jsi::Object(runtime);
  res.setProperty(runtime, "status", jsi::Value(1));
  res.setProperty(runtime, "message", jsi::String::createFromUtf8(runtime, message));
  return res;
}

jsi::Object createOk(jsi::Runtime &runtime)
{
  auto res = jsi::Object(runtime);
  res.setProperty(runtime, "status", jsi::Value(0));
  return res;
}

QuickSQLiteHostObject::QuickSQLiteHostObject(std::shared_ptr<react::CallInvoker> jsCallInvoker, const char* docPath): SmartHostObject(jsCallInvoker) {
  doc_path_ = docPath;
  thread_pool_ = std::make_shared<ThreadPool>();

  this->fields.push_back(buildPair("open", JSIF([=]){
    if (count == 0)
    {
      return createError(runtime, "[react-native-quick-sqlite][open] database name is required");
    }

    if (!arguments[0].isString())
    {
      return createError(runtime, "[react-native-quick-sqlite][open] database name must be a string");
    }

    std::string db_name = arguments[0].asString(runtime).utf8(runtime);
    std::string temp_doc_path = std::string(doc_path_);
    if (count > 1 && !arguments[1].isUndefined() && !arguments[1].isNull())
    {
      if (!arguments[1].isString())
      {
        return createError(runtime, "[react-native-quick-sqlite][open] database location must be a string");
      }

      temp_doc_path = temp_doc_path + "/" + arguments[1].asString(runtime).utf8(runtime);
    }

    SQLiteOPResult result = sqliteOpenDb(db_name, temp_doc_path);

    if (result.type == SQLiteError)
    {
      return createError(runtime, result.errorMessage.c_str());
    }

    return createOk(runtime);
  }));

  this->fields.push_back(buildPair("attach", JSIF([=]) {
    if(count < 3) {
      return createError(runtime, "[react-native-quick-sqlite][attach] Incorrect number of arguments");
    }
    if (!arguments[0].isString() || !arguments[1].isString() || !arguments[2].isString())
    {
      jsi::detail::throwJSError(runtime, "dbName, databaseToAttach and alias must be a strings");
      return {};
    }

    string tempDocPath = string(doc_path_);
    if (count > 3 && !arguments[3].isUndefined() && !arguments[3].isNull())
    {
      if (!arguments[3].isString())
      {
        return createError(runtime, "[react-native-quick-sqlite][attach] database location must be a string");
      }

      tempDocPath = tempDocPath + "/" + arguments[3].asString(runtime).utf8(runtime);
    }

    string dbName = arguments[0].asString(runtime).utf8(runtime);
    string databaseToAttach = arguments[1].asString(runtime).utf8(runtime);
    string alias = arguments[2].asString(runtime).utf8(runtime);
    SQLiteOPResult result = sqliteAttachDb(dbName, tempDocPath, databaseToAttach, alias);

    if (result.type == SQLiteError)
    {
      return createError(runtime, result.errorMessage.c_str());
    }

    return createOk(runtime);
  }));

  this->fields.push_back(buildPair("detach", JSIF([=]) {
    if(count < 2) {
      return createError(runtime, "[react-native-quick-sqlite][detach] Incorrect number of arguments");
    }
    if (!arguments[0].isString() || !arguments[1].isString())
    {
      jsi::detail::throwJSError(runtime, "dbName, databaseToAttach and alias must be a strings");
      return {};
    }

    string dbName = arguments[0].asString(runtime).utf8(runtime);
    string alias = arguments[1].asString(runtime).utf8(runtime);
    SQLiteOPResult result = sqliteDetachDb(dbName, alias);

    if (result.type == SQLiteError)
    {
      return createError(runtime, result.errorMessage.c_str());
    }

    return createOk(runtime);
  }));

  this->fields.push_back(buildPair("close", JSIF([=]) {
    if (count == 0)
    {
      return createError(runtime, "[react-native-quick-sqlite][close] database name is required");
    }

    if (!arguments[0].isString())
    {
      return createError(runtime, "[react-native-quick-sqlite][close] database name must be a string");
    }

    string dbName = arguments[0].asString(runtime).utf8(runtime);

    SQLiteOPResult result = sqliteCloseDb(dbName);

    if (result.type == SQLiteError)
    {
      return createError(runtime, result.errorMessage.c_str());
    }

    return createOk(runtime);
  }));

  this->fields.push_back(buildPair("delete", JSIF([=]) {
    if (count == 0)
    {
      return createError(runtime, "[react-native-quick-sqlite][open] database name is required");
    }

    if (!arguments[0].isString())
    {
      return createError(runtime, "[react-native-quick-sqlite][open] database name must be a string");
    }

    string dbName = arguments[0].asString(runtime).utf8(runtime);

    string tempDocPath = string(doc_path_);
    if (count > 1 && !arguments[1].isUndefined() && !arguments[1].isNull())
    {
      if (!arguments[1].isString())
      {
        return createError(runtime, "[react-native-quick-sqlite][open] database location must be a string");
      }

      tempDocPath = tempDocPath + "/" + arguments[1].asString(runtime).utf8(runtime);
    }


    SQLiteOPResult result = sqliteRemoveDb(dbName, tempDocPath);

    if (result.type == SQLiteError)
    {
      return createError(runtime, result.errorMessage.c_str());
    }

    return createOk(runtime);
  }));

  this->fields.push_back(buildPair("executeSql", JSIF([=]) {
    const string dbName = arguments[0].asString(runtime).utf8(runtime);
    const string query = arguments[1].asString(runtime).utf8(runtime);
    const jsi::Value &originalParams = arguments[2];

    vector<QuickValue> params;
    jsiQueryArgumentsToSequelParam(runtime, originalParams, &params);

    // Filling the results
    vector<map<string, QuickValue>> results;
    vector<QuickColumnMetadata> metadata;
    auto status = sqliteExecute(dbName, query, &params, &results, &metadata);


    auto jsiResult = createSequelQueryExecutionResult(runtime, status, &results, &metadata);
    return jsiResult;
  }));

  this->fields.push_back(buildPair("executeSqlBatch", JSIF([=]) {
    if (count < 2)
    {
      return createError(runtime, "[react-native-quick-sqlite][execSQLBatch] - Incorrect parameter count");
    }

    const jsi::Value &params = arguments[1];
    if (params.isNull() || params.isUndefined())
    {
      return createError(runtime, "[react-native-quick-sqlite][execSQLBatch] - An array of SQL commands or parameters is needed");
    }
    const string dbName = arguments[0].asString(runtime).utf8(runtime);
    const jsi::Array &batchParams = params.asObject(runtime).asArray(runtime);
    std::vector<QuickQueryArguments> commands;
    jsiBatchParametersToQuickArguments(runtime, batchParams, &commands);

    auto batchResult = executeBatch(dbName, &commands);
    if (batchResult.type == SQLiteOk)
    {
      auto res = jsi::Object(runtime);
      res.setProperty(runtime, "status", jsi::Value(0));
      res.setProperty(runtime, "rowsAffected", jsi::Value(batchResult.affectedRows));
      return move(res);
    }
    else
    {
      return createError(runtime, batchResult.message);
    }
  }));

  this->fields.push_back(buildPair("asyncExecuteSqlBatch", JSIF([=]) {
    if (count < 3)
    {
      jsi::detail::throwJSError(runtime, "[react-native-quick-sqlite][asyncExecuteSqlBatch] Incorrect parameter count");
      return {};
    }

    const jsi::Value &params = arguments[1];
    const jsi::Value &callbackHolder = arguments[2];
    if (!callbackHolder.isObject() || !callbackHolder.asObject(runtime).isFunction(runtime))
    {
      jsi::detail::throwJSError(runtime, "[react-native-quick-sqlite][asyncExecuteSqlBatch] The callback argument must be a function");
      return {};
    }

    if (params.isNull() || params.isUndefined())
    {
      jsi::detail::throwJSError(runtime, "[react-native-quick-sqlite][asyncExecuteSqlBatch] - An array of SQL commands or parameters is needed");
      return {};
    }

    const string dbName = arguments[0].asString(runtime).utf8(runtime);
    const jsi::Array &batchParams = params.asObject(runtime).asArray(runtime);
    auto callback = make_shared<jsi::Value>(callbackHolder.asObject(runtime));

    vector<QuickQueryArguments> commands;
    jsiBatchParametersToQuickArguments(runtime, batchParams, &commands);

    auto task =
    [&runtime, &jsCallInvoker, dbName, commands = make_shared<vector<QuickQueryArguments>>(commands), callback]()
    {
      try
      {
        // Inside the new worker thread, we can now call sqlite operations
        auto batchResult = executeBatch(dbName, commands.get());
        jsCallInvoker->invokeAsync([&runtime, batchResult = move(batchResult), callback]
                             {
          if(batchResult.type == SQLiteOk)
          {
            auto res = jsi::Object(runtime);
            res.setProperty(runtime, "status", jsi::Value(0));
            res.setProperty(runtime, "rowsAffected", jsi::Value(batchResult.affectedRows));
            callback->asObject(runtime).asFunction(runtime).call(runtime, move(res));
          } else
          {
            callback->asObject(runtime).asFunction(runtime).call(runtime, createError(runtime, batchResult.message));
          } });
      }
      catch (std::exception &exc)
      {
        jsCallInvoker->invokeAsync([&runtime, callback, &exc]
                             { callback->asObject(runtime).asFunction(runtime).call(runtime, createError(runtime, exc.what())); });
      }
    };
    thread_pool_->queueWork(task);
    return {};
  }));

  this->fields.push_back(buildPair("loadSqlFile", JSIF([=]) {
    const string dbName = arguments[0].asString(runtime).utf8(runtime);
    const string sqlFileName = arguments[1].asString(runtime).utf8(runtime);

    const auto importResult = importSQLFile(dbName, sqlFileName);
    if (importResult.type == SQLiteOk)
    {
      auto res = jsi::Object(runtime);
      res.setProperty(runtime, "status", jsi::Value(0));
      res.setProperty(runtime, "rowsAffected", jsi::Value(importResult.affectedRows));
      res.setProperty(runtime, "commands", jsi::Value(importResult.commands));
      return move(res);
    }
    else
    {
      return createError(runtime, "[react-native-quick-sqlite][loadSQLFile] Could not open file");
    }
  }));

  this->fields.push_back(buildPair("asyncLoadSqlFile", JSIF([=]) {
    if (count < 3)
    {
      jsi::detail::throwJSError(runtime, "[react-native-quick-sqlite][asyncLoadSqlFile] Incorrect parameter count");
      return {};
    }

    const jsi::Value &callbackHolder = arguments[2];
    if (!callbackHolder.isObject() || !callbackHolder.asObject(runtime).isFunction(runtime))
    {
      jsi::detail::throwJSError(runtime, "[react-native-quick-sqlite][asyncLoadSqlFile] The callback argument must be a function");
      return {};
    }

    const string dbName = arguments[0].asString(runtime).utf8(runtime);
    const string sqlFileName = arguments[1].asString(runtime).utf8(runtime);
    auto callback = make_shared<jsi::Value>(callbackHolder.asObject(runtime));

    auto task =
    [&runtime, &jsCallInvoker, dbName, sqlFileName, callback]()
    {
      try
      {
        // Running the import operation in another thread
        const auto importResult = importSQLFile(dbName, sqlFileName);

        // Executing the callback invoke inside the JavaScript thread in order to safe build JSI objects that depends on jsi::Runtime and must be synchronized.
        jsCallInvoker->invokeAsync([&runtime, result = move(importResult), callback]
                             {
          if(result.type == SQLiteOk)
          {
            auto res = jsi::Object(runtime);
            res.setProperty(runtime, "status", jsi::Value(0));
            res.setProperty(runtime, "rowsAffected", jsi::Value(result.affectedRows));
            res.setProperty(runtime, "commands", jsi::Value(result.commands));
            callback->asObject(runtime).asFunction(runtime).call(runtime, move(res));
          } else {
            callback->asObject(runtime).asFunction(runtime).call(runtime, createError(runtime, result.message));
          } });
      }
      catch (std::exception &exc)
      {
        jsCallInvoker->invokeAsync([&runtime, err = exc.what(), callback]
                             { callback->asObject(runtime).asFunction(runtime).call(runtime, createError(runtime, "Unknown error")); });
      }
    };
    thread_pool_->queueWork(task);
    return {};
  }));

  this->fields.push_back(buildPair("asyncExecuteSql", JSIF([=]) {
    if (count < 4)
    {
      jsi::detail::throwJSError(runtime, "[react-native-quick-sqlite][asyncExecuteSql] Incorrect arguments for asyncExecuteSQL");
      return {};
    }

    const jsi::Value &callbackHolder = arguments[3];
    if (!callbackHolder.isObject() || !callbackHolder.asObject(runtime).isFunction(runtime))
    {
      jsi::detail::throwJSError(runtime, "[react-native-quick-sqlite][asyncExecuteSql] The callback argument must be a function");
      return {};
    }

    const string dbName = arguments[0].asString(runtime).utf8(runtime);
    const string query = arguments[1].asString(runtime).utf8(runtime);
    const jsi::Value &originalParams = arguments[2];
    auto callback = make_shared<jsi::Value>(callbackHolder.asObject(runtime));

    // Converting query parameters inside the javascript caller thread
    vector<QuickValue> params;
    jsiQueryArgumentsToSequelParam(runtime, originalParams, &params);

    auto task =
    [&runtime, &jsCallInvoker, dbName, query, params = make_shared<vector<QuickValue>>(params), callback]()
    {
      try
      {
        // Inside the new worker thread, we can now call sqlite operations
        vector<map<string, QuickValue>> results;
        vector<QuickColumnMetadata> metadata;
        auto status = sqliteExecute(dbName, query, params.get(), &results, &metadata);
        jsCallInvoker->invokeAsync([&runtime, results = make_shared<vector<map<string, QuickValue>>>(results), metadata = make_shared<vector<QuickColumnMetadata>>(metadata), status_copy = move(status), callback]
                             {
          // Now, back into the JavaScript thread, we can translate the results
          // back to a JSI Object to pass on the callback
          auto jsiResult = createSequelQueryExecutionResult(runtime, status_copy, results.get(), metadata.get());
          callback->asObject(runtime).asFunction(runtime).call(runtime, move(jsiResult)); });
      }
      catch (std::exception &exc)
      {
        jsCallInvoker->invokeAsync([&runtime, callback, &exc]
                             { callback->asObject(runtime).asFunction(runtime).call(runtime, createError(runtime, exc.what())); });
      }
    };

    thread_pool_->queueWork(task);

    return {};
  }));
}

} // namespace osp
