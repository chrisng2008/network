#ifndef _INETHEADER_H
#define _INETHEADER_H

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#pragma pack(1) 
typedef struct ether_header_t{
    BYTE des_hw_addr[6];    //目的地址
    BYTE src_hw_addr[6];    //源地址
    WORD frametype;         //帧格式
} ether_header_t;   

//define arp hearder
typedef struct arp_header_t{
    WORD hw_type;           //硬件地址
    WORD prot_type;         //协议类型
    BYTE hw_addr_len;       //硬件地址长度
    BYTE prot_addr_len;     //协议地址长度
    WORD flag;              //操作类型
    BYTE send_hw_addr[6];   //发送端MAC地址
    DWORD send_prot_addr;   //发送端IP地址
    BYTE des_hw_addr[6];    //目标MAC地址
    DWORD des_prot_addr;    //目标IP地址
} arp_header_t;

// 硬件类型：表示硬件地址的类型，值为1表示以太网地址
// 协议类型：表示要映射的协议地址类型。它的值为0x0800表示IP地址类型
// 硬件地址长度和协议地址长度以字节为单位，对于以太网上的IP地址的ARP请求或应答来说，他们的值分别为6和4
// 操作类型,在报文中占2个字节,1表示ARP请求,2表示ARP应答,3表示RARP请求,4表示RARP应答
// 发送端MAC地址：发送方设备的硬件地址；
// 发送端IP地址：发送方设备的IP地址；
// 目标MAC地址：接收方设备的硬件地址。
// 目标IP地址：接收方设备的IP地址。

//define ip hearder
typedef struct ip_header_t{
    BYTE hlen_ver;          
    BYTE tos;               
    WORD total_len;         
    WORD id;                
    WORD flag;              
    BYTE ttl;               
    BYTE protocol;              
    WORD checksum;          
    DWORD src_ip;           
    DWORD des_ip;           
} ip_header_t;

//define udp hearder
typedef struct udp_header_t{
    WORD src_port;          
    WORD des_port;          
    WORD len;               
    WORD checksum;         
} udp_header_t;

//define tcp hearder
typedef struct tcp_header_t{
    WORD src_port;          
    WORD des_port;         
    DWORD seq;              
    DWORD ack;              
    BYTE len_res;           
    BYTE flag;               
    WORD window;            
    WORD checksum;          
    WORD urp;                
} tcp_header_t;

//define icmp hearder
typedef struct icmp_header_t{
    BYTE type;                  
    BYTE code;              
    WORD checksum;          
    WORD id;                   
    WORD seq;               
} icmp_header_t;

//arp数据包结构体
typedef struct arp_packet_t{
    ether_header_t etherheader;     //帧首部结构体
    arp_header_t arpheader;         //arp报文首部
} arp_packet_t;

typedef struct ip_packet_t{
    ether_header_t etherheader;     
    ip_header_t ipheader;
} ip_packet_t;

typedef struct tcp_packet_t{
    ether_header_t etherheader;
    ip_header_t ipheader;
    tcp_header_t tcpheader;
} tcp_packet_t;

typedef struct udp_packet_t{
    ether_header_t etherheader;
    ip_header_t ipheader;
    udp_header_t udpheader;
} udp_packet_t;

typedef struct icmp_packet_t{
    ether_header_t etherheader;
    ip_header_t ipheader;
    icmp_header_t icmpheader;
} icmp_packet_t;

#pragma pack()

#endif
