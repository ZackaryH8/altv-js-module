#include "Serialization.h"
#include "V8ResourceImpl.h"
#include "Bindings.h"

alt::MValue V8Helpers::V8ToMValue(v8::Local<v8::Value> val, bool allowFunction)
{
    auto& core = alt::ICore::Instance();

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> ctx = isolate->GetEnteredOrMicrotaskContext();

    if(val.IsEmpty()) return core.CreateMValueNone();

    if(val->IsUndefined()) return core.CreateMValueNone();

    if(val->IsNull()) return core.CreateMValueNil();

    if(val->IsBoolean()) return core.CreateMValueBool(val->BooleanValue(isolate));

    if(val->IsInt32()) return core.CreateMValueInt(val->Int32Value(ctx).ToChecked());

    if(val->IsUint32()) return core.CreateMValueUInt(val->Uint32Value(ctx).ToChecked());

    if(val->IsBigInt()) return core.CreateMValueInt(val.As<v8::BigInt>()->Int64Value());

    if(val->IsNumber()) return core.CreateMValueDouble(val->NumberValue(ctx).ToChecked());

    if(val->IsString()) return core.CreateMValueString(*v8::String::Utf8Value(isolate, val));

    if(val->IsObject())
    {
        if(val->IsArray())
        {
            v8::Local<v8::Array> v8Arr = val.As<v8::Array>();
            alt::MValueList list = core.CreateMValueList(v8Arr->Length());

            for(uint32_t i = 0; i < v8Arr->Length(); ++i)
            {
                v8::Local<v8::Value> value;
                if(!v8Arr->Get(ctx, i).ToLocal(&value)) continue;
                list->Set(i, V8ToMValue(value, allowFunction));
            }

            return list;
        }
        else if(val->IsFunction())
        {
            if(!allowFunction)
            {
                Log::Error << V8::SourceLocation::GetCurrent(isolate).ToString() << " "
                           << "Cannot convert function to MValue" << Log::Endl;
                return core.CreateMValueNone();
            }
            v8::Local<v8::Function> v8Func = val.As<v8::Function>();
            return V8ResourceImpl::Get(ctx)->GetFunction(v8Func);
        }
        else if(val->IsArrayBuffer())
        {
            auto v8Buffer = val.As<v8::ArrayBuffer>()->GetBackingStore();
            return core.CreateMValueByteArray((uint8_t*)v8Buffer->Data(), v8Buffer->ByteLength());
        }
        else if(val->IsMap())
        {
            v8::Local<v8::Map> map = val.As<v8::Map>();
            v8::Local<v8::Array> mapArr = map->AsArray();
            uint32_t size = mapArr->Length();

            alt::MValueDict dict = alt::ICore::Instance().CreateMValueDict();
            for(uint32_t i = 0; i < size; i += 2)
            {
                auto maybeKey = mapArr->Get(ctx, i);
                auto maybeValue = mapArr->Get(ctx, i + 1);
                v8::Local<v8::Value> key;
                v8::Local<v8::Value> value;

                if(!maybeKey.ToLocal(&key)) continue;
                if(!maybeValue.ToLocal(&value)) continue;
                alt::String keyString = V8::Stringify(ctx, val);
                if(keyString.IsEmpty()) continue;
                dict->Set(keyString, V8ToMValue(value, false));
            }
            return dict;
        }
        else
        {
            V8ResourceImpl* resource = V8ResourceImpl::Get(ctx);
            v8::Local<v8::Object> v8Obj = val.As<v8::Object>();

            // if (v8Obj->InstanceOf(ctx, v8Vector3->JSValue(isolate, ctx)).ToChecked())
            if(resource->IsVector3(v8Obj))
            {
                v8::Local<v8::Value> x, y, z;
                V8_CHECK_RETN(v8Obj->Get(ctx, V8::Vector3_XKey(isolate)).ToLocal(&x), "Failed to convert Vector3 to MValue", core.CreateMValueNil());
                V8_CHECK_RETN(v8Obj->Get(ctx, V8::Vector3_YKey(isolate)).ToLocal(&y), "Failed to convert Vector3 to MValue", core.CreateMValueNil());
                V8_CHECK_RETN(v8Obj->Get(ctx, V8::Vector3_ZKey(isolate)).ToLocal(&z), "Failed to convert Vector3 to MValue", core.CreateMValueNil());

                return core.CreateMValueVector3(alt::Vector3f{ x.As<v8::Number>()->Value(), y.As<v8::Number>()->Value(), z.As<v8::Number>()->Value() });
            }
            else if(resource->IsVector2(v8Obj))
            {
                v8::Local<v8::Value> x, y;
                V8_CHECK_RETN(v8Obj->Get(ctx, V8::Vector3_XKey(isolate)).ToLocal(&x), "Failed to convert Vector2 to MValue", core.CreateMValueNil());
                V8_CHECK_RETN(v8Obj->Get(ctx, V8::Vector3_YKey(isolate)).ToLocal(&y), "Failed to convert Vector2 to MValue", core.CreateMValueNil());

                return core.CreateMValueVector2(alt::Vector2f{ x.As<v8::Number>()->Value(), y.As<v8::Number>()->Value() });
            }
            else if(resource->IsRGBA(v8Obj))
            {
                v8::Local<v8::Value> r, g, b, a;
                V8_CHECK_RETN(v8Obj->Get(ctx, V8::RGBA_RKey(isolate)).ToLocal(&r), "Failed to convert RGBA to MValue", core.CreateMValueNil());
                V8_CHECK_RETN(v8Obj->Get(ctx, V8::RGBA_GKey(isolate)).ToLocal(&g), "Failed to convert RGBA to MValue", core.CreateMValueNil());
                V8_CHECK_RETN(v8Obj->Get(ctx, V8::RGBA_BKey(isolate)).ToLocal(&b), "Failed to convert RGBA to MValue", core.CreateMValueNil());
                V8_CHECK_RETN(v8Obj->Get(ctx, V8::RGBA_AKey(isolate)).ToLocal(&a), "Failed to convert RGBA to MValue", core.CreateMValueNil());

                return core.CreateMValueRGBA(
                  alt::RGBA{ (uint8_t)r.As<v8::Number>()->Value(), (uint8_t)g.As<v8::Number>()->Value(), (uint8_t)b.As<v8::Number>()->Value(), (uint8_t)a.As<v8::Number>()->Value() });
            }
            else if(resource->IsBaseObject(v8Obj))
            {
                V8Entity* ent = V8Entity::Get(v8Obj);
                Log::Debug << "Instanceof BaseObject" << Log::Endl;

                V8_CHECK_RETN(ent, "Unable to convert base object to MValue because it was destroyed and is now invalid", core.CreateMValueNil());
                return core.CreateMValueBaseObject(ent->GetHandle());
            }
            else
            {
                alt::MValueDict dict = core.CreateMValueDict();
                v8::Local<v8::Array> keys;

                V8_CHECK_RETN(v8Obj->GetOwnPropertyNames(ctx).ToLocal(&keys), "Failed to convert object to MValue", core.CreateMValueNil());
                for(uint32_t i = 0; i < keys->Length(); ++i)
                {
                    v8::Local<v8::Value> v8Key;
                    V8_CHECK_RETN(keys->Get(ctx, i).ToLocal(&v8Key), "Failed to convert object to MValue", core.CreateMValueNil());
                    v8::Local<v8::Value> value;
                    V8_CHECK_RETN(v8Obj->Get(ctx, v8Key).ToLocal(&value), "Failed to convert object to MValue", core.CreateMValueNil());

                    if(value->IsUndefined()) continue;
                    std::string key = *v8::String::Utf8Value(isolate, v8Key);
                    dict->Set(key, V8ToMValue(value, allowFunction));
                }

                return dict;
            }
        }
    }

    return core.CreateMValueNone();
}

v8::Local<v8::Value> V8Helpers::MValueToV8(alt::MValueConst val)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> ctx = isolate->GetEnteredOrMicrotaskContext();

    switch(val->GetType())
    {
        case alt::IMValue::Type::NONE: return v8::Undefined(isolate);
        case alt::IMValue::Type::NIL: return V8::JSValue(nullptr);
        case alt::IMValue::Type::BOOL: return V8::JSValue(val.As<alt::IMValueBool>()->Value());
        case alt::IMValue::Type::INT:
        {
            int64_t _val = val.As<alt::IMValueInt>()->Value();

            if(_val >= INT_MIN && _val <= INT_MAX) return V8::JSValue((int32_t)_val);

            return V8::JSValue(_val);
        }
        case alt::IMValue::Type::UINT:
        {
            uint64_t _val = val.As<alt::IMValueUInt>()->Value();

            if(_val <= UINT_MAX) return V8::JSValue((uint32_t)_val);

            return V8::JSValue(_val);
        }
        case alt::IMValue::Type::DOUBLE: return V8::JSValue(val.As<alt::IMValueDouble>()->Value());
        case alt::IMValue::Type::STRING: return V8::JSValue(val.As<alt::IMValueString>()->Value());
        case alt::IMValue::Type::LIST:
        {
            alt::MValueListConst list = val.As<alt::IMValueList>();
            v8::Local<v8::Array> v8Arr = v8::Array::New(isolate, list->GetSize());

            for(uint32_t i = 0; i < list->GetSize(); ++i) v8Arr->Set(ctx, i, MValueToV8(list->Get(i)));

            return v8Arr;
        }
        case alt::IMValue::Type::DICT:
        {
            alt::MValueDictConst dict = val.As<alt::IMValueDict>();
            v8::Local<v8::Object> v8Obj = v8::Object::New(isolate);

            for(auto it = dict->Begin(); it; it = dict->Next())
            {
                v8Obj->Set(ctx, V8::JSValue(it->GetKey()), MValueToV8(it->GetValue()));
            }

            return v8Obj;
        }
        case alt::IMValue::Type::BASE_OBJECT:
        {
            alt::Ref<alt::IBaseObject> ref = val.As<alt::IMValueBaseObject>()->Value();
            return V8ResourceImpl::Get(ctx)->GetBaseObjectOrNull(ref);
        }
        case alt::IMValue::Type::FUNCTION:
        {
            alt::MValueFunctionConst fn = val.As<alt::IMValueFunction>();
            v8::Local<v8::External> extFn = v8::External::New(isolate, new alt::MValueFunctionConst(fn));

            v8::Local<v8::Function> func;
            V8_CHECK_RETN(v8::Function::New(ctx, V8Helpers::FunctionCallback, extFn).ToLocal(&func), "Failed to convert MValue to function", v8::Undefined(isolate));
            return func;
        }
        case alt::IMValue::Type::VECTOR3: return V8ResourceImpl::Get(ctx)->CreateVector3(val.As<alt::IMValueVector3>()->Value());
        case alt::IMValue::Type::VECTOR2: return V8ResourceImpl::Get(ctx)->CreateVector2(val.As<alt::IMValueVector2>()->Value());
        case alt::IMValue::Type::RGBA: return V8ResourceImpl::Get(ctx)->CreateRGBA(val.As<alt::IMValueRGBA>()->Value());
        case alt::IMValue::Type::BYTE_ARRAY:
        {
            alt::MValueByteArrayConst buffer = val.As<alt::IMValueByteArray>();
            // Check if the buffer is a raw JS value buffer
            v8::MaybeLocal<v8::Value> jsVal = RawBytesToV8(buffer);
            if(!jsVal.IsEmpty()) return jsVal.ToLocalChecked();

            v8::Local<v8::ArrayBuffer> v8Buffer = v8::ArrayBuffer::New(isolate, buffer->GetSize());
            std::memcpy(v8Buffer->GetBackingStore()->Data(), buffer->GetData(), buffer->GetSize());
            return v8Buffer;
        }
        default: Log::Warning << "V8::MValueToV8 Unknown MValue type " << (int)val->GetType() << Log::Endl;
    }

    return v8::Undefined(isolate);
}

void V8Helpers::MValueArgsToV8(alt::MValueArgs args, std::vector<v8::Local<v8::Value>>& v8Args)
{
    for(uint64_t i = 0; i < args.GetSize(); ++i) v8Args.push_back(MValueToV8(args[i]));
}

// Magic bytes to identify raw JS value buffers
static uint8_t magicBytes[] = { 'J', 'S', 'V', 'a', 'l' };

enum class RawValueType : uint8_t
{
    GENERIC,
    ENTITY,
    VECTOR3,
    VECTOR2,
    RGBA,
    INVALID
};

extern V8Class v8Entity;
static inline RawValueType GetValueType(v8::Local<v8::Context> ctx, v8::Local<v8::Value> val)
{
    V8ResourceImpl* resource = V8ResourceImpl::Get(ctx);
    bool result;
    if(val->InstanceOf(ctx, v8Entity.JSValue(ctx->GetIsolate(), ctx)).To(&result) && result) return RawValueType::ENTITY;
    if(resource->IsVector3(val)) return RawValueType::VECTOR3;
    if(resource->IsVector2(val)) return RawValueType::VECTOR2;
    if(resource->IsRGBA(val)) return RawValueType::RGBA;
    if(val->IsSharedArrayBuffer() || val->IsFunction()) return RawValueType::INVALID;
    else
        return RawValueType::GENERIC;
}

// Converts a JS value to a MValue byte array
alt::MValueByteArray V8Helpers::V8ToRawBytes(v8::Local<v8::Value> val)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> ctx = isolate->GetEnteredOrMicrotaskContext();
    std::vector<uint8_t> bytes;

    RawValueType type = GetValueType(ctx, val);
    if(type == RawValueType::INVALID) return alt::MValueByteArray();

    std::unique_ptr<SerializerDelegate> delegate = std::make_unique<SerializerDelegate>();
    v8::ValueSerializer serializer(isolate, delegate.get());
    serializer.WriteHeader();
    serializer.WriteRawBytes(&type, sizeof(uint8_t));

    switch(type)
    {
        case RawValueType::GENERIC:
        {
            bool result;
            if(!serializer.WriteValue(ctx, val).To(&result) || !result) return alt::MValueByteArray();
            break;
        }
        case RawValueType::ENTITY:
        {
            V8Entity* entity = V8Entity::Get(val);
            if(!entity) return alt::MValueByteArray();
            uint16_t id = entity->GetHandle().As<alt::IEntity>()->GetID();
            serializer.WriteRawBytes(&id, sizeof(id));
            break;
        }
        case RawValueType::VECTOR3:
        {
            alt::Vector3f vec;
            if(!V8::SafeToVector3(val, ctx, vec)) return alt::MValueByteArray();
            float x = vec[0];
            float y = vec[1];
            float z = vec[2];
            serializer.WriteRawBytes(&x, sizeof(float));
            serializer.WriteRawBytes(&y, sizeof(float));
            serializer.WriteRawBytes(&z, sizeof(float));
            break;
        }
        case RawValueType::VECTOR2:
        {
            alt::Vector2f vec;
            if(!V8::SafeToVector2(val, ctx, vec)) return alt::MValueByteArray();
            float x = vec[0];
            float y = vec[1];
            serializer.WriteRawBytes(&x, sizeof(float));
            serializer.WriteRawBytes(&y, sizeof(float));
            break;
        }
        case RawValueType::RGBA:
        {
            alt::RGBA rgba;
            if(!V8::SafeToRGBA(val, ctx, rgba)) return alt::MValueByteArray();
            serializer.WriteRawBytes(&rgba.r, sizeof(uint8_t));
            serializer.WriteRawBytes(&rgba.g, sizeof(uint8_t));
            serializer.WriteRawBytes(&rgba.b, sizeof(uint8_t));
            serializer.WriteRawBytes(&rgba.a, sizeof(uint8_t));
            break;
        }
    }

    std::pair<uint8_t*, size_t> serialized = serializer.Release();

    // Write the serialized value to the buffer
    bytes.assign(serialized.first, serialized.first + serialized.second);

    // Reserve size for the magic bytes
    bytes.reserve(bytes.size() + sizeof(magicBytes));

    // Write the magic bytes to the front of the buffer
    for(size_t i = 0; i < sizeof(magicBytes); ++i) bytes.insert(bytes.begin() + i, magicBytes[i]);

    // Copy the data, because it gets freed by the std::vector when this scope ends,
    // and the MValue byte array does not copy the data
    uint8_t* data = new uint8_t[bytes.size()];
    std::memcpy(data, bytes.data(), bytes.size());

    return alt::ICore::Instance().CreateMValueByteArray(data, bytes.size());
}

// Converts a MValue byte array to a JS value
v8::MaybeLocal<v8::Value> V8Helpers::RawBytesToV8(alt::MValueByteArrayConst rawBytes)
{
    // We copy the data here, because the std::vector frees the data when it goes out of scope
    uint8_t* data = new uint8_t[rawBytes->GetSize()];
    std::memcpy(data, rawBytes->GetData(), rawBytes->GetSize());
    std::vector<uint8_t> bytes(data, data + rawBytes->GetSize());

    // Check for magic bytes
    if(bytes.size() < sizeof(magicBytes)) return v8::MaybeLocal<v8::Value>();
    for(size_t i = 0; i < sizeof(magicBytes); ++i)
        if(bytes[i] != magicBytes[i]) return v8::MaybeLocal<v8::Value>();

    // Remove the magic bytes from the byte array
    bytes.erase(bytes.begin(), bytes.begin() + sizeof(magicBytes));

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> ctx = isolate->GetEnteredOrMicrotaskContext();

    v8::ValueDeserializer deserializer(isolate, bytes.data(), bytes.size());
    bool headerValid;
    if(!deserializer.ReadHeader(ctx).To(&headerValid) || !headerValid) return v8::MaybeLocal<v8::Value>();
    RawValueType* typePtr;
    if(!deserializer.ReadRawBytes(sizeof(uint8_t), (const void**)&typePtr)) return v8::MaybeLocal<v8::Value>();
    RawValueType type = *typePtr;

    // Deserialize the value
    V8ResourceImpl* resource = V8ResourceImpl::Get(ctx);
    v8::MaybeLocal<v8::Value> result;
    switch(type)
    {
        case RawValueType::GENERIC:
        {
            result = deserializer.ReadValue(ctx);
            break;
        }
        case RawValueType::ENTITY:
        {
            uint16_t* id;
            if(!deserializer.ReadRawBytes(sizeof(uint16_t), (const void**)&id)) return v8::MaybeLocal<v8::Value>();
            alt::Ref<alt::IEntity> entity = alt::ICore::Instance().GetEntityByID(*id);
            if(!entity) return v8::MaybeLocal<v8::Value>();
            result = V8ResourceImpl::Get(ctx)->GetOrCreateEntity(entity.Get(), "Entity")->GetJSVal(isolate);
            break;
        }
        case RawValueType::VECTOR3:
        {
            float* x;
            float* y;
            float* z;
            if(!deserializer.ReadRawBytes(sizeof(float), (const void**)&x) || !deserializer.ReadRawBytes(sizeof(float), (const void**)&y) ||
               !deserializer.ReadRawBytes(sizeof(float), (const void**)&z))
                return v8::MaybeLocal<v8::Value>();
            result = resource->CreateVector3({ *x, *y, *z });
            break;
        }
        case RawValueType::VECTOR2:
        {
            float* x;
            float* y;
            if(!deserializer.ReadRawBytes(sizeof(float), (const void**)&x) || !deserializer.ReadRawBytes(sizeof(float), (const void**)&y)) return v8::MaybeLocal<v8::Value>();
            result = resource->CreateVector2({ *x, *y });
            break;
        }
        case RawValueType::RGBA:
        {
            uint8_t* r;
            uint8_t* g;
            uint8_t* b;
            uint8_t* a;
            if(!deserializer.ReadRawBytes(sizeof(uint8_t), (const void**)&r) || !deserializer.ReadRawBytes(sizeof(uint8_t), (const void**)&g) ||
               !deserializer.ReadRawBytes(sizeof(uint8_t), (const void**)&b) || !deserializer.ReadRawBytes(sizeof(uint8_t), (const void**)&a))
                return v8::MaybeLocal<v8::Value>();
            result = resource->CreateRGBA({ *r, *g, *b, *a });
            break;
        }
    }

    return result;
}

void V8Helpers::SerializerDelegate::ThrowDataCloneError(v8::Local<v8::String> message)
{
    if(message.IsEmpty()) return;
    std::string messageStr = *v8::String::Utf8Value(v8::Isolate::GetCurrent(), message);
    if(messageStr.empty()) return;
    Log::Error << "[V8] Serialization error: " << messageStr << Log::Endl;
}
