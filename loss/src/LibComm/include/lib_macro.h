#ifndef __LIB_MACRO_H__
#define __LIB_MACRO_H__ 

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "lib_err.h"

namespace LIB_COMM {

#ifndef likely
#define likely(x)   __builtin_expect(!!(x),1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x),0)
#endif


#define FILE_SIZE_K     1024
#define FILE_SIZE_M     1048576         // 1024 * 1024
#define FILE_SIZE_G     1073741824      // 1024 * 1024 * 1024

#define SIZE_K          1024
#define SIZE_10K          10240
#define SIZE_100K          102400


#define IF_RET(cmd, ret)                                                       \
    do{                                                                        \
        if(cmd)                                                                \
        {                                                                      \
            return ret;                                                        \
        }                                                                      \
    } while(0)

#define IFNOT_RET(cmd, ret)                                                    \
    do{                                                                        \
        int iRet = (cmd);                                                      \
        if(iRet != ret)                                                        \
        {                                                                      \
            return iRet;                                                       \
        }                                                                      \
    } while(0)

#define CLASS_MEMBER_OBJ(type, name)                                           \
    public:                                                                    \
        void set_##name(const type& name) { name##_ = name; }                  \
        const type& name() const { return name##_; }                           \
    protected:                                                                 \
        type name##_;
#define CLASS_MEMBER_VAR(type, name)                                           \
    public:                                                                    \
        void set_##name(type name) { name##_ = name; }                         \
        type name() const { return name##_; }                                  \
    protected:                                                                 \
        type name##_;

#define NO_NEGATIVE(name)  ((name > 0) ? name:0)

#define BE_NEGATIVE(name)  ((name < 0) ? true:false)

#define SET_PROTOBUF_FIELD(obj, pb_field, value)                               \
    if (obj.has_##pb_field()) {                                                \
        obj.set_##pb_field(value);                                             \
    }

#define RETURN(ret)                                                            \
    do{                                                                        \
        LOG_TRACE("Call:%s||ret:%d",__FUNCTION__,ret);                         \
        return ret;                                                            \
    }while(0)

#define RETURN_IF_NOT_ZERO(cmd)                                                \
    do{                                                                        \
        int ret = (cmd);                                                       \
        if( ret != 0)                                                          \
        {                                                                      \
            RETURN(ret);                                                       \
        }                                                                      \
    }while(0)

#define DO_REPLY_IF_ERROR(cmd,status)                                          \
    do{                                                                        \
        int ret = (cmd);                                                       \
        if(ret != 0)                                                           \
        {                                                                      \
            DoReply(status);                                                   \
        }                                                                      \
    }while(0)

#define DO_RETURN_IF_ERROR(cmd,status)                                         \
    do{                                                                        \
        int ret = (cmd);                                                       \
        if(ret != 0)                                                           \
        {                                                                      \
            DoReply(status);                                                   \
            RETURN(ret);                                                       \
        }                                                                      \
    }while(0)

#define IS_JSON_INT(root, member_name, type, condition)                        \
    (root.isMember(member_name) && root[member_name].isIntegral() && root[member_name].as##type() condition)

#define IS_JSON_INT2(root, member_name, type, condition1, condition2)          \
    (root.isMember(member_name) && root[member_name].isIntegral() && root[member_name].as##type() condition1 && root[member_name].as##type() condition2)

#define IS_JSON_STR(root, member_name, condition)                              \
    (root.isMember(member_name) && root[member_name].isString() && root[member_name].asString() condition)

#define IS_JSON_OBJECT(root, member_name)                                       \
    (root.isMember(member_name) && root[member_name].isObject())

#define IS_JSON_ARRAY(root, member_name)                                        \
    (root.isMember(member_name) && root[member_name].isArray() && root[member_name].size() > 0)

#define JSON_REPLACE(root, src, dst)                                            \
    do{                                                                         \
        root[dst] = root[src];                                                  \
        root.removeMember(src);                                                 \
    }while(0)

#define JSON_REPLACE2(root_src, src, root_dst, dst)                             \
    do{                                                                         \
        root_dst[dst] = root_src[src];                                          \
        root_src.removeMember(src);                                             \
    }while(0)

#define JSON_COPY_IF_EXIST(json_src, str_src, json_dst, str_dst)            \
    do{                                                                     \
        if (json_src.isMember(str_src))                                     \
        {                                                                   \
            json_dst[str_dst] = json_src[str_src];                          \
        }                                                                   \
    }while(0)

#define IS_JSON_OBJ(str,j_obj)    \
do{                               \
    Json::Reader reader;          \
    reader.parse(str,j_obj);      \
    if(!j_obj.isObject())         \
        return -1;                \
}while(0);

#define JSON_CAST(src, des)                   \
do {                                        \
    std::stringstream ss;                   \
    ss << src;                              \
    ss >> des;                              \
    if (ss.fail()) {                        \
        LOG_TRACE("invlaid types:%s", #src);\
    }                                       \
} while (0);

#define JSON_CAST_DEF(src, des, def)      \
do {                                    \
    std::stringstream ss;               \
    ss << src;                          \
    ss >> des;                          \
    if (ss.fail()) {                    \
        des = def;                      \
    }                                   \
} while (0);

#define JSON_GET_DEF(json, destype, value, def)                         \
destype value = def;                                                    \
do {                                                                    \
    if (!json.isMember(#value)) break;                                  \
    const Json::Value &jsonValue = json[#value];                        \
    if (jsonValue.empty()) break;                                       \
    switch (jsonValue.type()) {                                         \
    case Json::nullValue:                                               \
        JSON_CAST_DEF("", value, def); break;                             \
    case Json::intValue:                                                \
        JSON_CAST_DEF(jsonValue.asInt64(), value, def); break;            \
    case Json::uintValue:                                               \
        JSON_CAST_DEF(jsonValue.asUInt64(), value, def); break;           \
    case Json::realValue:                                               \
        JSON_CAST_DEF(jsonValue.asDouble(), value, def); break;           \
    case Json::stringValue:                                             \
        JSON_CAST_DEF(jsonValue.asString(), value, def); break;           \
    case Json::booleanValue:                                            \
        JSON_CAST_DEF(jsonValue.asBool(), value, def); break;             \
    default: break;}                                                    \
} while (0);

#define JSON_GET(json, key, value)                             \
do {                                                           \
    if (!json.isMember(#key)) break;                           \
    const Json::Value &jsonValue = json[#key];                 \
    if (jsonValue.empty()) break;                              \
    switch (jsonValue.type()) {                                \
    case Json::nullValue:                                      \
        JSON_CAST("", value); break;                             \
    case Json::intValue:                                       \
        JSON_CAST(jsonValue.asInt64(), value); break;            \
    case Json::uintValue:                                      \
        JSON_CAST(jsonValue.asUInt64(), value); break;           \
    case Json::realValue:                                      \
        JSON_CAST(jsonValue.asDouble(), value); break;           \
    case Json::stringValue:                                    \
        JSON_CAST(jsonValue.asString(), value); break;           \
    case Json::booleanValue:                                   \
        JSON_CAST(jsonValue.asBool(), value); break;             \
    default: break;}                                           \
} while (0);

#define CHECK_JSON_LAYER2(root, mem, mem_child, ret) \
do {  \
    if (root.isMember(mem)) \
    { \
        if (!root[mem].isObject()) \
        { \
            LOG_ERROR("json[%s] is not object", #mem); \
            ret = -1; \
            break; \
        } \
        else \
        { \
            if (!root[mem].isMember(mem_child)) \
            { \
                LOG_INFO("json[%s][%s] not exist", #mem, #mem_child); \
                ret = 2; \
                break; \
            } \
            ret = 0; \
            break; \
        } \
    } \
    else \
    { \
        LOG_INFO("json[%s] not exist", #mem); \
        ret = 1; \
        break; \
    } \
} while (0);

//pos从1开始递增
#define IS_BIT_SET(num, pos) ((num) & (1 << (pos))) > 0 ? true : false
#define SET_BIT(num, pos) (num) |=  (1 << pos)

}
#endif 
