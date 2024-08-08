#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>

using namespace std;
const int port = 5851;

string handle_get(string url)
{
	

	string file_name = ".." + url;
	ifstream file(file_name);
	if (!file)
	{
		// 文件打开失败，可能是文件不存在或者没有足够的权限
		cerr << "Error opening file " << file_name << endl;
		string response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 13\r\n\r\n404 Not Found";
		return "response";
	}

	int is_html = 0;
	string response = "HTTP/1.1 200 OK\r\n";
	if (url.rfind(".jpg") != string::npos)
	{
		response += "Content-Type: image/jpeg\r\n";
	}
	else if (url.rfind(".html") != string::npos)
	{
		response += "Content-Type: text/html\r\n";
		is_html = 1;
	}
	else
	{
		response += "Content-Type: text/plain\r\n";
	}
	if (is_html)
	{
		string content = "";
		string line;
		while (getline(file, line))
		{
			int pos = line.find("<img src=\"");
			if (pos != string::npos)
			{
				// 找到<img src="
				string prefix = line.substr(0, pos + 10); // 包括<img src="
				string suffix = line.substr(pos + 10);    // 剩余部分
				// 将路径替换为绝对路径
				suffix = "http://localhost:5851/" + suffix;
				line = prefix + suffix;
			}
			content += line;
		}
		response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
		response += "\r\n";
		response += content;
	}
	else
	{
		string content((istreambuf_iterator<char>(file)),
						istreambuf_iterator<char>());
		response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
		response += "\r\n";
		response += content;
	}
	file.close();
	return response;
}
string handle_post(char* request,string url){
	if(url.find("/dopost") == string::npos){
		cerr << "Error post path " << url << endl;
		string response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 13\r\n\r\n404 Not Found";
		return "response";
	}
	stringstream request_stream(request);
	string line;
	string login="", pass="";
	bool flag = false;
	while (getline(request_stream, line))
	{
		
		if (line == "\r")
		{
			flag = true;
			continue;
		}
		if (!flag)
		{
			continue;
		}
		// 解析login和pass
		stringstream data_stream(line);
		string key, value;
		int pos = line.find("login=");
		int pos1 = line.find("&");
		int pos2 = line.find("pass=");
		if (pos != string::npos && pos1 != string::npos && pos2 != string::npos)
		{
			login = line.substr(pos + 6, pos1-pos-6);
			pass = line.substr(pos2+5);
			
		}
	}
	
	string message;
	if (login == "3200105851" && pass == "5851")
	{
		message = "登录成功";
	}
	else
	{
		message = "登录失败";
	}

	// 将响应消息封装成 html 格式
	string response = "<html><body>" + message + "</body></html>";
	string response_str = "HTTP/1.1 200 OK\r\n";
	response_str += "Content-Type: text/html; charset=utf-8\r\n";
	response_str += "Content-Length: " + std::to_string(response.size()) + "\r\n";
	response_str += "\r\n";
	response_str += response;
	return response_str;
}
void* handle_request(int client_fd)
{
    
    char request[1024],temp_request[1024];
    recv(client_fd, request, 1024, 0);
    request[strlen(request) + 1] = '\0';
    // 使用 stringstream 将请求报文解析为单独的行
    stringstream request_stream(request);
    string line;

    // 读取第一行，解析出请求方法、URL、协议版本
    getline(request_stream, line);
    string method, url, version;
    stringstream first_line_stream(line);
    first_line_stream >> method >> url >> version;

    if (method == "GET")
    {
        string response = handle_get(url);
        int s = send(client_fd, response.c_str(), response.size(), 0); // 发送响应
        close(client_fd);
    }
    else if(method == "POST")
    {
        string response = handle_post(request,url);
        int s = send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
    }
}

int main()
{
    int sock;
    int client_fd;
    struct sockaddr_in sever_address;
    bzero(&sever_address, sizeof(sever_address));
    sever_address.sin_family = PF_INET;
    sever_address.sin_addr.s_addr = htons(INADDR_ANY);
    sever_address.sin_port = htons(5851);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int ret = bind(sock, (struct sockaddr *)&sever_address, sizeof(sever_address));
    assert(ret != -1);
    ret = listen(sock, 1);
    assert(ret != -1);
	cout << "Listening..." << endl;
	while (1)
	{
        struct sockaddr_in client;
        socklen_t client_addrlength = sizeof(client);
        client_fd = accept(sock, (struct sockaddr *)&client, &client_addrlength);
        if (client_fd < 0)
        {
            printf("errno\n");
        }
        else
        {
            // 创建一个新的线程来处理客户端的请求
            
            std::thread t(handle_request, client_fd);
        // 将线程设置为分离状态，这样就不用手动回收线程资源
            t.detach();
        }
        }
        return 0;
}