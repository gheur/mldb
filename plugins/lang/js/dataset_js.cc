// This file is part of MLDB. Copyright 2015 mldb.ai inc. All rights reserved.

/** dataset_js.cc
    Jeremy Barnes, 14 June 2015
    Copyright (c) 2015 mldb.ai inc.  All rights reserved.

    JS interface for datasets.
*/

#include "dataset_js.h"
#include "mldb/core/dataset.h"
#include "js_common.h"

using namespace std;



namespace MLDB {


/*****************************************************************************/
/* DATASET JS                                                                */
/*****************************************************************************/

v8::Handle<v8::Object>
DatasetJS::
create(std::shared_ptr<Dataset> dataset, JsPluginContext * context)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    auto obj = context->Dataset.Get(isolate)->GetFunction()->NewInstance();
    auto * wrapped = new DatasetJS();
    wrapped->dataset = dataset;
    wrapped->wrap(obj, context);
    return obj;
}

Dataset *
DatasetJS::
getShared(const v8::Handle<v8::Object> & val)
{
    return reinterpret_cast<DatasetJS *>
        (v8::Handle<v8::External>::Cast
         (val->GetInternalField(0))->Value())->dataset.get();
}

v8::Local<v8::FunctionTemplate>
DatasetJS::
registerMe()
{
    using namespace v8;

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    EscapableHandleScope scope(isolate);

    auto fntmpl = CreateFunctionTemplate("Dataset");
    auto prototmpl = fntmpl->PrototypeTemplate();

    prototmpl->Set(String::NewFromUtf8(isolate, "recordRow"),
                   FunctionTemplate::New(isolate, recordRow));
    prototmpl->Set(String::NewFromUtf8(isolate, "recordRows"),
                   FunctionTemplate::New(isolate, recordRows));
    prototmpl->Set(String::NewFromUtf8(isolate, "recordColumn"),
                   FunctionTemplate::New(isolate, recordColumn));
    prototmpl->Set(String::NewFromUtf8(isolate, "recordColumns"),
                   FunctionTemplate::New(isolate, recordColumns));

    prototmpl->Set(String::NewFromUtf8(isolate, "commit"),
                   FunctionTemplate::New(isolate, commit));
    prototmpl->Set(String::NewFromUtf8(isolate, "status"),
                   FunctionTemplate::New(isolate, status));
    prototmpl->Set(String::NewFromUtf8(isolate, "id"),
                   FunctionTemplate::New(isolate, id));
    prototmpl->Set(String::NewFromUtf8(isolate, "type"),
                   FunctionTemplate::New(isolate, type));
    prototmpl->Set(String::NewFromUtf8(isolate, "config"),
                   FunctionTemplate::New(isolate, config));
        
    prototmpl->Set(String::NewFromUtf8(isolate, "getColumnPaths"),
                   FunctionTemplate::New(isolate, getColumnPaths));
    prototmpl->Set(String::NewFromUtf8(isolate, "getTimestampRange"),
                   FunctionTemplate::New(isolate, getTimestampRange));
        
    return scope.Escape(fntmpl);
}

void
DatasetJS::
recordRow(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    JsContextScope scope(args.This());
    try {
        Dataset * dataset = getShared(args.This());
         
        auto rowName = JS::getArg<RowPath>(args, 0, "rowName");
        auto values = JS::getArg<std::vector<std::tuple<ColumnPath, CellValue, Date> > >(args, 1, "values", {});

        {
            //v8::Unlocker unlocker(args.GetIsolate());
            dataset->recordRow(std::move(rowName), std::move(values));
        }

        args.GetReturnValue().Set(args.This());
    } HANDLE_JS_EXCEPTIONS(args);
}

void
DatasetJS::
recordRows(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    JsContextScope scope(args.This());
    try {
        Dataset * dataset = getShared(args.This());
        
        // Look at rows
        // If it's an array of arrays, it's in the vector<pair> format
        // If it's an array of objects, it's in vector<MatrixNamedRow> format

        auto array = args[0].As<v8::Array>();
        if (array.IsEmpty())
            throw HttpReturnException(400, "value " + JS::cstr(args[0]) + " is not an array");
        if (array->Length() == 0)
            return;

        auto el = array->Get(0);
        if (el->IsArray()) {
            // Note: goes first, because an array is also an object
            auto rows = JS::getArg<std::vector<std::pair<RowPath, std::vector<std::tuple<ColumnPath, CellValue, Date> > > > >(args, 0, "rows", {});
            v8::Unlocker unlocker(args.GetIsolate());
            dataset->recordRows(std::move(rows));
        }
        else if (el->IsObject()) {
            std::vector<std::pair<RowPath, ExpressionValue> > toRecord;
            toRecord.reserve(array->Length());

            auto columns = v8::String::NewFromUtf8(args.GetIsolate(), "columns");
            auto rowPath = v8::String::NewFromUtf8(args.GetIsolate(), "rowPath");

            for (size_t i = 0;  i < array->Length();  ++i) {
                MatrixNamedRow row;
                auto obj = array->Get(i).As<v8::Object>();

                if (obj.IsEmpty()) {
                    throw HttpReturnException(400, "recordRow element is not object");
                }

                toRecord.emplace_back(from_js(JS::JSValue(obj->Get(rowPath)),
                                              &row.rowName),
                                      from_js(JS::JSValue(obj->Get(columns)),
                                              &row.columns));
            }
            
            v8::Unlocker unlocker(args.GetIsolate());
            dataset->recordRowsExpr(std::move(toRecord));
        }
        else throw HttpReturnException(400, "Can't call recordRows with argument "
                                       + JS::cstr(el));
        
        args.GetReturnValue().Set(args.This());
    } HANDLE_JS_EXCEPTIONS(args);
}

void
DatasetJS::
recordColumn(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    JsContextScope scope(args.This());
    try {
        Dataset * dataset = getShared(args.This());
        
        auto columnName = JS::getArg<ColumnPath>(args, 0, "columnName");
        auto column = JS::getArg<std::vector<std::tuple<ColumnPath, CellValue, Date> > >(args, 1, "values", {});

        {
            //v8::Unlocker unlocker(args.GetIsolate());
            dataset->recordColumn(std::move(columnName), std::move(column));
        }

        args.GetReturnValue().Set(args.This());
    } HANDLE_JS_EXCEPTIONS(args);
}

void
DatasetJS::
recordColumns(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    JsContextScope scope(args.This());
    try {
        Dataset * dataset = getShared(args.This());
            
        auto columns = JS::getArg<std::vector<std::pair<ColumnPath, std::vector<std::tuple<ColumnPath, CellValue, Date> > > > >(args, 0, "columns", {});
        dataset->recordColumns(std::move(columns));

        args.GetReturnValue().Set(args.This());
    } HANDLE_JS_EXCEPTIONS(args);
}

void
DatasetJS::
commit(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    try {
        Dataset * dataset = getShared(args.This());
            
        dataset->commit();
            
        args.GetReturnValue().Set(args.This());
    } HANDLE_JS_EXCEPTIONS(args);
}

void
DatasetJS::
status(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    try {
        Dataset * dataset = getShared(args.This());
            
        args.GetReturnValue().Set(JS::toJS(jsonEncode(dataset->getStatus())));
    } HANDLE_JS_EXCEPTIONS(args);
}
    
void
DatasetJS::
id(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    try {
        Dataset * dataset = getShared(args.This());
            
        args.GetReturnValue().Set(JS::toJS(jsonEncode(dataset->getId())));
    } HANDLE_JS_EXCEPTIONS(args);
}
    
void
DatasetJS::
type(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    try {
        Dataset * dataset = getShared(args.This());
            
        args.GetReturnValue().Set(JS::toJS(jsonEncode(dataset->getType())));
    } HANDLE_JS_EXCEPTIONS(args);
}
    
void
DatasetJS::
config(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    try {
        Dataset * dataset = getShared(args.This());
        
        args.GetReturnValue().Set(JS::toJS(jsonEncode(dataset->getConfig())));
    } HANDLE_JS_EXCEPTIONS(args);
}

void
DatasetJS::
getColumnPaths(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    try {
        Dataset * dataset = getShared(args.This());

        args.GetReturnValue().Set(JS::toJS(dataset->getColumnIndex()->getColumnPaths()));
    } HANDLE_JS_EXCEPTIONS(args);
}

void
DatasetJS::
getTimestampRange(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    try {
        Dataset * dataset = getShared(args.This());
        args.GetReturnValue().Set(JS::toJS(dataset->getTimestampRange()));
    } HANDLE_JS_EXCEPTIONS(args);
}

} // namespace MLDB

