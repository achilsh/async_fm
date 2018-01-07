# base_frameworks
# 分支:cnf_center_framewk ==> 完成配置中心，路由分配，自动摘机，手动摘机能力
# 分支:debug_access  ==>  完成tcp 接入能力测试,发现内部协议pb时，动态加载so问题(TODO)，通过命令字查询服务名能力
# 分支:thrift_proto_debug ===> 测试内部业务数据用thrift 序列化，用于解决debug_access 分支问题core问题，
2#      业务so 内部实现暂时不能包含全局静态变量动态加载so，因为会影响dlopen()多次加载生效(TODO)。
