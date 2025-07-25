#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#if defined(_WIN32)
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif
#ifdef USE_NLOHMANN_JSON
	#include <nlohmann/json.hpp>
#endif
namespace fs = std::filesystem;
using namespace std;

struct Cfg{string host,user,pass,rem,context; int port;};

Cfg load_cfg(){
	Cfg c; c.port=21;
#ifdef USE_NLOHMANN_JSON
	ifstream f("config.json");
	if(f){
		nlohmann::json j; f >> j;
		if(j.contains("host")) c.host=j["host"];
		if(j.contains("username")) c.user=j["username"];
		if(j.contains("password")) c.pass=j["password"];
		if(j.contains("remotePath")) c.rem=j["remotePath"];
		if(j.contains("context")) c.context=j["context"];
		if(j.contains("port")) c.port=j["port"];
	}
#else
	ifstream f("config.json"); string line,j;
	while(getline(f,line)) j+=line;
	auto val=[&](const string& k){
		auto p=j.find("\""+k+"\"");
		if(p==string::npos) return string();
		p=j.find(":",p); p=j.find_first_of("\"0123456789",p+1);
		if(j[p]=='"') p++;
		auto end=j.find_first_of(j[p-1]=='"' ? "\"" : ",}",p);
		return j.substr(p,end-p);
	};
	c.host=val("host"); c.user=val("username");
	c.pass=val("password"); c.rem=val("remotePath");
	c.context=val("context");
	string ps=val("port");
	if(!ps.empty()) c.port=stoi(ps);
#endif
	return c;
}

int connect_to(const char* host,int port){
	struct addrinfo h{},*r;
	h.ai_family=AF_INET; h.ai_socktype=SOCK_STREAM;
	if(getaddrinfo(host,to_string(port).c_str(),&h,&r)!=0) return -1;
	int s=socket(r->ai_family,r->ai_socktype,r->ai_protocol);
	if(s<0){ freeaddrinfo(r); return -2; }
	if(connect(s,r->ai_addr,r->ai_addrlen)!=0){ freeaddrinfo(r); return -3; }
	freeaddrinfo(r); return s;
}

string recv_line(int sock){
	char c; string r;
	while(recv(sock,&c,1,0)==1){
		r.push_back(c);
		if(r.size()>1 && r.substr(r.size()-2)=="\r\n") break;
	}
	return r;
}

bool send_cmd(int sock,const string& cmd){
	string s=cmd+"\r\n";
	return send(sock,s.c_str(),s.size(),0)>=0;
}

int pasv_data(int ctrl){
	if(!send_cmd(ctrl,"PASV")) return -1;
	auto r=recv_line(ctrl);
	int a=0,b=0,c=0,d=0,p1=0,p2=0;
	sscanf(r.c_str(),"%*[^0-9]%d,%d,%d,%d,%d,%d",&a,&b,&c,&d,&p1,&p2);
	string ip=to_string(a)+"."+to_string(b)+"."+to_string(c)+"."+to_string(d);
	return connect_to(ip.c_str(),p1*256+p2);
}

void list_remote(int ctrl){
	int d=pasv_data(ctrl);
	if(d<0){ cout<<"[!] PASV fail\n"; return; }
	if(!send_cmd(ctrl,"LIST")){ cout<<"[!] LIST fail\n"; return; }
	recv_line(ctrl);
	char buf[512]; int n;
	while((n=recv(d,buf,sizeof(buf)-1,0))>0){buf[n]=0; cout<<buf;}
#if defined(_WIN32)
	closesocket(d);
#else
	close(d);
#endif
	recv_line(ctrl);
}

bool upload_one(int ctrl,const fs::path& lp,const string& rp){
	int d=pasv_data(ctrl);
	if(d<0) return false;
	if(!send_cmd(ctrl,"STOR "+rp)){ closesocket(d); return false; }
	recv_line(ctrl);
	FILE* f=fopen(lp.string().c_str(),"rb");
	if(!f){ closesocket(d); return false; }
	char buf[1024]; size_t r;
	while((r=fread(buf,1,1024,f))>0) send(d,buf,r,0);
	fclose(f); closesocket(d); recv_line(ctrl);
	return true;
}

void sync_dir(int ctrl,const fs::path& loc,const string& rem,int& cnt){
	if(cnt>=128) return;
	for(auto& e: fs::directory_iterator(loc)){
		if(cnt>=128) break;
		auto fn=e.path().filename().string();
		if(fn==".fooname") continue;
		string sub=rem+"/"+fn;
		if(e.is_directory()){
			if(send_cmd(ctrl,"MKD "+sub)) recv_line(ctrl);
			sync_dir(ctrl,e.path(),sub,cnt);
		} else if(upload_one(ctrl,e.path(),sub)){
			cnt++;
		}
	}
}

int main(){
#if defined(_WIN32)
	WSADATA w; WSAStartup(MAKEWORD(2,2),&w);
#endif
	auto c=load_cfg();
	if(c.host.empty()||c.user.empty()||c.pass.empty()||c.rem.empty()||c.context.empty()){
		cout<<"[!] Config error\n"; return 1;
	}
	int ctrl=connect_to(c.host.c_str(),c.port);
	if(ctrl<0){ cout<<"[!] Connect fail\n"; return 2; }
	recv_line(ctrl);
	if(!send_cmd(ctrl,"USER "+c.user)){ cout<<"[!] USER fail\n"; return 3; }
	recv_line(ctrl);
	if(!send_cmd(ctrl,"PASS "+c.pass)){ cout<<"[!] PASS fail\n"; return 4; }
	recv_line(ctrl);
	string cmd; int cnt=0;
	while(cin>>cmd){
		if(cmd=="ls") list_remote(ctrl);
		else if(cmd=="sync") sync_dir(ctrl, fs::path(c.context), c.rem, cnt), cout<<"Uploaded "<<cnt<<"\n";
		else if(cmd=="quit"){ send_cmd(ctrl,"QUIT"); recv_line(ctrl); break;}
	}
#if defined(_WIN32)
	WSACleanup();
#endif
	return 0;
} 