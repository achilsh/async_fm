# base_frameworks
1# 分支:cnf_center_framewk ==> 完成配置中心，路由分配，自动摘机，手动摘机能力，热启动集群中任何so. 
2# 分支:debug_access  ==>  完成通过Access代理tcp接入能力,发现内部协议pb时，动态加载so问题(TODO)，提供通过命令字查询服务名能力. 
3# 分支:thrift_proto_debug ===> 测试内部业务数据用thrift 序列化，用于解决debug_access 分支问题core问题. 
3.1#      业务so 内部实现暂时不能包含全局静态变量动态加载so，因为会影响dlopen()多次加载生效(TODO)。
