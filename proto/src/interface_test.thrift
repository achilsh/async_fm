namespace cpp Test

struct ping
{
1:required i32           a;        //整数
2:required string        b;        //字符串
}

struct pang
{
1:required i32           retcode;  //每个rpc必须有个返回码
2:required i32           a;        //整数
3:required string        b;        //字符串
}

service demosvr
{
    //ping服务端是否有响应
    pang pingping(1:ping pi, 2:i32 iparam, 3:string sparam );
}

