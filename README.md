<!-- 原本的路径，是运行时的ip_pools.txt，源代码，makefile和程序都在一起的。这里提交时分开了，注意以下的部分：
* makefile编译后生成的程序仍然在当前目录下，同时会将整个vpn目录拷贝至docker容器
* 源码中证书的路径是../certification
* 源码中ip_pools.txt是相对路径 -->
# 2024 HUST CSE NetWorkSecurity Lab3
2024华中科技大学网络空间安全学院计算机网络安全实验，基于TUN/TAP和TLS/SSL的VPN实现。
支持多客户端与服务端自动ip分配

# 使用方法
## 网络环境
```bash
docker network create --subnet=10.0.2.0/24 --gateway=10.0.2.8 --opt "com.docker.network.bridge.name"="docker1" extranet

docker network create --subnet=192.168.60.0/24 --gateway=192.168.60.1 --opt "com.docker.network.bridge.name"="docker2" intranet
```
## 容器配置
```bash
#在 VM 上新开一个终端，创建并运行容器 HostU
docker run -it --name=HostU --hostname=HostU --net=extranet --ip=10.0.2.7 --privileged "seedubuntu" /bin/bash
docker run -it --name=HostU2 --hostname=HostU2 --net=extranet --ip=10.0.2.6 --privileged "seedubuntu" /bin/bash
#在 VM 上新开一个终端，创建并运行容器 HostV
docker run -it --name=HostV --hostname=HostV --net=intranet --ip=192.168.60.101 --privileged "seedubuntu" /bin/bash
#在容器 HostU 和 HostV 内分别删除掉默认路由
route del default
route del default
```
## 证书
**请修改相关证书为自己的证书，步骤参考实验手册和下方即可**
```bash
cp /usr/lib/ssl/openssl.cnf ./openssl.cnf
openssl req -new -x509 -keyout ca.key -out ca.crt -config openssl.cnf

Country Name (2 letter code) [AU]:CN
State or Province Name (full name) [Some-State]:HuBei
Locality Name (eg, city) []:WuHan
Organization Name (eg, company) [Internet Widgits Pty Ltd]:HUST
Organizational Unit Name (eg, section) []:CSE
Common Name (e.g. server FQDN or YOUR name) []:Zj_W1nd
Email Address []:zj_w1nd@163.com
```
服务器请求CA生成证书：
```bash
openssl genrsa -des3 -out server.key 2048 # 密码：server
openssl req -new -key server.key -out server.csr -config ../openssl.cnf
# Please enter the following 'extra' attributes to be sent with your certificate request
# A challenge password []:challenge
# An optional company name []:NetWorkSecurity
openssl ca -in server.csr -out server.crt -cert ../ca.crt -keyfile ../ca.key -config ../openssl.cnf
```
客户端生成证书：
```bash
# Please enter the following 'extra' attributes
# to be sent with your certificate request
# A challenge password []:challenge_client
# An optional company name []:NetWorkSecurity
openssl genrsa -des3 -out client.key 2048 # 密码：client
openssl req -new -key client.key -out client.csr -config ../openssl.cnf
openssl ca -in client.csr -out client.crt -cert ../ca.crt -keyfile ../ca.key -config ../openssl.cnf
```
## host配置
在HostU和HostU2中添加一条我们要访问的外网，和证书保持一致
```bash
echo "10.0.2.8 Zj_W1nd" >> /etc/hosts
```
## 使用
在source_code目录下直接make，保证ip_pools.txt，程序与证书的相对位置关系即可。

特别感谢[@shiftw041](https://github.com/shiftw041)学姐的[仓库](https://github.com/shiftw041/NetworkSecurity/tree/main/lab/%E5%AE%9E%E9%AA%8C%E4%B8%89)，个人在其基础上修改实现了自动ip分配并调整了项目结构。