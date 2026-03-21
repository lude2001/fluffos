#include <net/socket.h>
#include <socket_err.h>

inherit F_DBASE;

// 定义常量
#define HTTP_PORT 80
#define BUFFER_SIZE 1024
#define REQUEST_TIMEOUT 30

private nosave mapping sockets = ([]);
private nosave mapping callbacks = ([]);
private nosave mapping buffers = ([]);

void create()
{
    seteuid(getuid());
}

// 发送HTTP GET请求
varargs int http_get(string host, string path, function callback, mapping headers, int port)
{
    int sock;
    string request;
    
    if (!host || !path)
        return 0;

    // 如果没有指定端口，使用默认端口
    if (!port) port = HTTP_PORT;
        
    sock = socket_create(STREAM, "read_callback", "close_callback");
    if (sock < 0)
        return 0;
        
    sockets[sock] = ([
        "host": host,
        "path": path,
        "method": "GET"
    ]);
    
    callbacks[sock] = callback;
    buffers[sock] = "";
    
    // 构建HTTP请求头
    request = sprintf("GET %s HTTP/1.1\r\n", path);
    request += sprintf("Host: %s\r\n", host);
    request += "Connection: close\r\n";
    
    // 添加自定义请求头
    if (mapp(headers)) {
        foreach (string key, string value in headers) {
            request += sprintf("%s: %s\r\n", key, value);
        }
    }
    
    request += "\r\n";
    
    // 连接服务器并发送请求
    if (socket_connect(sock, host + " " + port, "read_callback", "write_callback") < 0) {
        socket_close(sock);
        return 0;
    }
    
    sockets[sock]["request"] = request;
    return 1;
}

// 发送HTTP POST请求
varargs int http_post(string host, string path, string data, function callback, mapping headers, int port)
{
    int sock;
    string request;
    
    if (!host || !path)
        return 0;
    
    // 如果没有指定端口，使用默认端口
    if (!port) port = HTTP_PORT;
        
    sock = socket_create(STREAM, "read_callback", "close_callback");
    if (sock < 0)
        return 0;
        
    sockets[sock] = ([
        "host": host,
        "path": path,
        "method": "POST",
        "data": data,
        "port": port
    ]);
    
    callbacks[sock] = callback;
    buffers[sock] = "";
    
    // 构建HTTP请求头
    request = sprintf("POST %s HTTP/1.1\r\n", path);
    request += sprintf("Host: %s:%d\r\n", host, port);  // 在Host头中包含端口
    request += "Connection: close\r\n";

    if (!mapp(headers))
    {
        request += sprintf("Content-Length: %d\r\n", sizeof(string_encode(data, "utf-8")));
        request += "Content-Type: application/json; charset=utf-8\r\n";
    }
    
    // 添加自定义请求头
    if (mapp(headers)) {
        foreach (string key, string value in headers) {
            request += sprintf("%s: %s\r\n", key, value);
        }
    }
    
    request += "\r\n" + data;
    
    // 连接服务器时使用指定的端口
    if (socket_connect(sock, host + " " + port, "read_callback", "write_callback") < 0) {
        socket_close(sock);
        return 0;
    }
    
    sockets[sock]["request"] = request;
    return 1;
}

// 写入回调函数
void write_callback(int sock)
{
    if (!sockets[sock] || !sockets[sock]["request"])
        return;
        
    socket_write(sock, sockets[sock]["request"]);
}

// 改进读取回调函数,增加响应解析
void read_callback(int sock, mixed message)
{
    string response;
    buffers[sock] += message;
    
    // 检查是否收到完整的HTTP响应
    if (strsrch(buffers[sock], "\r\n\r\n") != -1)
    {
        // 分离HTTP头和正文
        string *parts = explode(buffers[sock], "\r\n\r\n");
        string headers = parts[0];

        
        // 解析状态码
        string *header_lines = explode(headers, "\r\n");
        string status_line = header_lines[0];
        response = sizeof(parts) > 1 ? parts[1] : "";
        // 存储解析后的信息
        sockets[sock]["parsed_response"] = ([
            "status": status_line,
            "headers": headers,
            "body": response
        ]);
    }
}

// 改进关闭回调函数,返回结构化数据
void close_callback(int sock)
{
    mapping response_data;
    
    if (callbacks[sock])
    {
        response_data = sockets[sock]["parsed_response"];
        if (!response_data)
            response_data = (["body": buffers[sock]]); // 如果没有解析成功,返回原始数据
            
        evaluate(callbacks[sock], response_data);
    }
    
    // 清理资源
    map_delete(sockets, sock);
    map_delete(callbacks, sock);
    map_delete(buffers, sock);
}

// 示例使用方法
void test_request()
{
    // GET请求示例
    http_get("api.example.com", "/data", 
        function(mapping response) {
            write("状态: " + response["status"] + "\n");
            write("响应内容: " + response["body"] + "\n");
        },
        ([ "User-Agent": "MUD Client" ])
    );
    
    // POST请求示例
    http_post("api.example.com", "/submit", 
        "name=test&value=123",
        function(mapping response) {
            write("状态: " + response["status"] + "\n");
            write("响应内容: " + response["body"] + "\n");
        },
        ([ "User-Agent": "MUD Client" ])
    );
}

// 返回所有接收到的数据
mapping get_all_data()
{
    return sockets;
}