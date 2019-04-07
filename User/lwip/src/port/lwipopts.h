/**
  ******************************************************************************
  * @file    lwipopts.h
  * @author  MCD Application Team & suozhang
  * @version V2.0.0
  * @date    2019年4月3日15:40:16
  * @brief   lwIP Options Configuration.
  *          This file is based on Utilities\lwip_v2.1.2\src\include\lwip\opt.h 
  *          and contains the lwIP configuration for the STM32H743 demonstration.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/**
 * NO_SYS==1: Provides VERY minimal functionality. Otherwise,
 * use lwIP facilities.
 */
#define NO_SYS                  0 /* 使用 FreeRTOS 驱动 lwip */ 

/**
 * SYS_LIGHTWEIGHT_PROT==1: enable inter-task protection (and task-vs-interrupt
 * protection) for certain critical regions during buffer allocation, deallocation
 * and memory allocation and deallocation.
 * ATTENTION: This is required when using lwIP from more than one context! If
 * you disable this, you must be sure what you are doing!
 */
#define SYS_LIGHTWEIGHT_PROT    1 /* 使用 rtos 的临界区保护 lwip 的线程，以及关键变量 */

/**
 * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
 */
#define LWIP_NETCONN            1 

/**
 * LWIP_IGMP==1: Turn on IGMP module.
 */
#define LWIP_IGMP               0

/**
 * LWIP_ICMP==1: Enable ICMP module inside the IP stack.
 * Be careful, disable that make your product non-compliant to RFC1122
 */
#define LWIP_ICMP               1 /* Enable ICMP module inside the IP stack.such as ping ... */ 

/**
 * LWIP_HAVE_LOOPIF==1: Support loop interface (127.0.0.1).
 * This is only needed when no real netifs are available. If at least one other
 * netif is available, loopback traffic uses this netif.
 */
#define LWIP_HAVE_LOOPIF        0 /* DisEnable loop interface (127.0.0.1). */ 

/** Define the byte order of the system.
 * Needed for conversion of network data to host byte order.
 * Allowed values: LITTLE_ENDIAN and BIG_ENDIAN
 */
#ifndef BYTE_ORDER
#define BYTE_ORDER  LITTLE_ENDIAN /* 小字节序、低字节序 */ 
#endif

/* ---------- Debug options ---------- */

#define LWIP_DEBUG

#define LWIP_DBG_TYPES_ON           (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_LEVEL_ALL) // LWIP_DBG_HALT 开启将导致 debug while(1)

#ifdef LWIP_DEBUG

	#define SYS_DEBUG                   LWIP_DBG_OFF
	#define ETHARP_DEBUG                LWIP_DBG_OFF
	#define PPP_DEBUG                   LWIP_DBG_OFF
	#define MEM_DEBUG                   LWIP_DBG_OFF
	#define MEMP_DEBUG                  LWIP_DBG_OFF
	#define PBUF_DEBUG                  LWIP_DBG_OFF
	#define API_LIB_DEBUG               LWIP_DBG_OFF
	#define API_MSG_DEBUG               LWIP_DBG_OFF
	#define TCPIP_DEBUG                 LWIP_DBG_OFF
	#define NETIF_DEBUG                 LWIP_DBG_OFF
	#define SOCKETS_DEBUG               LWIP_DBG_OFF
	#define DNS_DEBUG                   LWIP_DBG_OFF
	#define AUTOIP_DEBUG                LWIP_DBG_OFF
	#define DHCP_DEBUG                  LWIP_DBG_OFF
	#define IP_DEBUG                    LWIP_DBG_OFF
	#define IP_REASS_DEBUG              LWIP_DBG_OFF
	#define ICMP_DEBUG                  LWIP_DBG_OFF
	#define IGMP_DEBUG                  LWIP_DBG_OFF
	#define UDP_DEBUG                   LWIP_DBG_OFF
	#define TCP_DEBUG                   LWIP_DBG_OFF
	#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
	#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
	#define TCP_RTO_DEBUG               LWIP_DBG_OFF
	#define TCP_CWND_DEBUG              LWIP_DBG_OFF
	#define TCP_WND_DEBUG               LWIP_DBG_OFF
	#define TCP_FR_DEBUG                LWIP_DBG_OFF
	#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
	#define TCP_RST_DEBUG               LWIP_DBG_OFF

#endif

/* ---------- Memory options ---------- */
/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
   lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
   byte alignment -> define MEM_ALIGNMENT to 2. */
#define MEM_ALIGNMENT           4  /* 字节对其，32位CPU 必须 为4 */

/* if MEMP_OVERFLOW_CHECK is turned on, we reserve some bytes at the beginning
 * and at the end of each element, initialize them as 0xcd and check
 * them later. */
/* If MEMP_OVERFLOW_CHECK is >= 2, on every call to memp_malloc or memp_free,
 * every single element in each pool is checked!
 * This is VERY SLOW but also very helpful. */
#define MEMP_OVERFLOW_CHECK         1 /* 开启 动态内存池pool 的校验检查 */

/**
 * Set this to 1 if you want to free PBUF_RAM pbufs (or call mem_free()) from
 * interrupt context (or another context that doesn't allow waiting for a
 * semaphore).
 * If set to 1, mem_malloc will be protected by a semaphore and SYS_ARCH_PROTECT,
 * while mem_free will only use SYS_ARCH_PROTECT. mem_malloc SYS_ARCH_UNPROTECTs
 * with each loop so that mem_free can run.
 *
 * ATTENTION: As you can see from the above description, this leads to dis-/
 * enabling interrupts often, which can be slow! Also, on low memory, mem_malloc
 * can need longer.
 *
 * If you don't want that, at least for NO_SYS=0, you can still use the following
 * functions to enqueue a deallocation call which then runs in the tcpip_thread
 * context:
 * - pbuf_free_callback(p);
 * - mem_free_callback(m);
 */
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 1 /* 使用RTOS的信号量和临界区保护内存的分配以及释放等等 */ 

/**
 * MEMP_MEM_MALLOC==1: Use mem_malloc/mem_free instead of the lwip pool allocator.
 * Especially useful with MEM_LIBC_MALLOC but handle with care regarding execution
 * speed (heap alloc can be much slower than pool alloc) and usage from interrupts
 * (especially if your netif driver allocates PBUF_POOL pbufs for received frames
 * from interrupt)!
 * ATTENTION: Currently, this uses the heap for ALL pools (also for private pools,
 * not only for internal pools defined in memp_std.h)!
 */
#define MEMP_MEM_MALLOC             0 /* 使用动态内存池 pool 的方式 给 TCP 以及 UDP 控制块，内核在初始化的时候为每个数据结构初始化好了一定的pool */ 

/**
 * suozhang thinking 2018年1月23日11:10:42 参考 老衲五木 嵌入式网络那些事 P74
 * MEM_LIBC_MALLOC = 1，直接使用C库中的malloc、free来分配动态内存；否则使用LWIP自带的mem_malloc、mem_free等函数。
 * MEMP_MEM_MALLOC = 1，则 memp.c 中的全部内容也不会被编译，即动态内存池pool分配策略不会使用
 * MEM_USE_POOLS   = 1，则 mem.c 中 的全部内容也不会被编译，这种方式需要用内存池的方式实现，需要用户自己实现，比较麻烦，这里没有用这个方法
 * 
 * 这里使用了 lwip 默认的 动态内存池（pool）和 动态内存堆 mem 的方式，MEM_SIZE 就是动态内存堆的大小
 * LWIP 消耗的 内存 就是 MEM_SIZE 动态内存堆 + MEMP 动态内存池pool 消耗的内存
 * 
*/

/* MEM_SIZE: the size of the heap memory. If the application will send
a lot of data that needs to be copied, this should be set high. */
#define MEM_SIZE                (10*1024) /* 应用程序 发送大量数据 需要复制的，这个值应该设置大一点 */

/* Relocate the LwIP RAM heap pointer */
#define LWIP_RAM_HEAP_POINTER    (0x30044000)

/* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
   sends a lot of data out of ROM (or other static memory), this
   should be set high. */
#define MEMP_NUM_PBUF           16 /* 应用程序 发送大量数据 是存在ROM 的，比如 网页，这个值应该设置大一点 */

/**
 * MEMP_NUM_NETCONN: the number of struct netconns.
 * (only needed if you use the sequential API, like api_lib.c)
 */
#define MEMP_NUM_NETCONN        5 /* 上层API 可以使用 NETCONN 的个数，包括 UDP 和TCP */

/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
   per active UDP "connection". */
#define MEMP_NUM_UDP_PCB        3 /* 上层API 可以使用 UDP 的个数，UDP 连接较多时 应该增大该值 */

/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP
   connections. */
#define MEMP_NUM_TCP_PCB        6 /* 上层API 可以使用 TCP 的个数，UDP 连接较多时 应该增大该值 */

/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP
   connections. */
#define MEMP_NUM_TCP_PCB_LISTEN 3 /* 上层API 可以使用 TCP 监听的个数，TCP 服务器较多时 应该增大该值 */

/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
   segments. */
#define MEMP_NUM_TCP_SEG        TCP_SND_QUEUELEN /* TCP内核的缓冲报文段个数，当应用数据较多时应该增加该值 */

/**
 * MEMP_NUM_SYS_TIMEOUT: the number of simultaneously active timeouts.
 * The default number of timeouts is calculated here for all enabled modules.
 * The formula expects settings to be either '0' or '1'.
 */
#define MEMP_NUM_SYS_TIMEOUT    10 /* 同时激活的 超时数量 */


/* ---------- Pbuf options ---------- */

/* 当申请 struct pbuf 类型的pbuf就会用到，内核不会使用这种pbuf。应用或者网卡驱动移植的时候可以使用，它能快速分配和是释放 */

/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#define PBUF_POOL_SIZE          10 /* pool 类型的 PUBF 的个数，建议接收驱动中使用该类型的PBUF数据包，并加大该值  */

/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. */
#define PBUF_POOL_BUFSIZE       LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)

/* PBUF_LINK_HLEN: the number of bytes that should be allocated for a
   link level header. */
	 //#define PBUF_LINK_HLEN              16 /* 暂时不知道 为什么 暂时屏蔽 2018年1月23日13:42:14 */

/* LWIP_SUPPORT_CUSTOM_PBUF == 1: to pass directly MAC Rx buffers to the stack 
   no copy is needed */
#define LWIP_SUPPORT_CUSTOM_PBUF      1

/* ---------- TCP options ---------- */

/**
 * LWIP_TCP==1: Turn on TCP.
 */
#define LWIP_TCP                1

/**
 * TCP_TTL: Default Time-To-Live value.
 */
#define TCP_TTL                 IP_DEFAULT_TTL /* IP 数据包中的TTL 的值 */

/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#define TCP_QUEUE_OOSEQ         ( LWIP_TCP )  /* TCP是否缓冲接收到的无序报文段 */

/* TCP Maximum segment size. */
#define TCP_MSS                 (1500 - 40)	  /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) TCP最大报文段大小 */

/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF             (4*TCP_MSS)   /* TCP 发送缓冲区大小，增大该值可以提升TCP性能 */

/* TCP receive window. */
#define TCP_WND                 (4*TCP_MSS)   /* TCP 发送窗口大小，增大改值可以提升TCP性能 */


/* ---------- DHCP options ---------- */
/* Define LWIP_DHCP to 1 if you want DHCP configuration of
   interfaces. DHCP is not implemented in lwIP 0.5.1, however, so
   turning this on does currently not work. */
#define LWIP_DHCP               0


/* ---------- UDP options ---------- */
#define LWIP_UDP                1
#define UDP_TTL                 255


/* ---------- Statistics options ---------- */
#define LWIP_STATS 0
#define LWIP_PROVIDE_ERRNO 1

/* ---------- link callback options ---------- */
/* LWIP_NETIF_LINK_CALLBACK==1: Support a callback function from an interface
 * whenever the link changes (i.e., link down)
 */
#define LWIP_NETIF_LINK_CALLBACK        1


/* 
The STM32F4x7 allows computing and verifying the IP, UDP, TCP and ICMP checksums by hardware:
 - To use this feature let the following define uncommented.
 - To disable it and process by CPU comment the  the checksum.
*/
#define CHECKSUM_BY_HARDWARE 


#ifdef CHECKSUM_BY_HARDWARE
  /* CHECKSUM_GEN_IP==0: Generate checksums by hardware for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP                 0
  /* CHECKSUM_GEN_UDP==0: Generate checksums by hardware for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP                0
  /* CHECKSUM_GEN_TCP==0: Generate checksums by hardware for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP                0 
  /* CHECKSUM_CHECK_IP==0: Check checksums by hardware for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP               0
  /* CHECKSUM_CHECK_UDP==0: Check checksums by hardware for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP              0
  /* CHECKSUM_CHECK_TCP==0: Check checksums by hardware for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP              0
  /* CHECKSUM_CHECK_ICMP==0: Check checksums by hardware for incoming ICMP packets.*/  
  #define CHECKSUM_GEN_ICMP               0
#else
  /* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP                 1
  /* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP                1
  /* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP                1
  /* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP               1
  /* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP              1
  /* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP              1
  /* CHECKSUM_CHECK_ICMP==1: Check checksums by hardware for incoming ICMP packets.*/  
  #define CHECKSUM_GEN_ICMP               1
#endif


#define ETHARP_TRUST_IP_MAC     0
#define IP_REASSEMBLY           0
#define IP_FRAG                 0
#define ARP_QUEUEING            0



/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
*/
/**
 * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
 */
#define LWIP_SOCKET                     0



/**
 * LWIP_TCP_KEEPALIVE==1: Enable TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPCNT
 * options processing. Note that TCP_KEEPIDLE and TCP_KEEPINTVL have to be set
 * in seconds. (does not require sockets.c, and will affect tcp.c)
 * SuoZhang,2017年4月21日13:36:45，add
 */
#define  TCP_KEEPIDLE_DEFAULT     5000UL  	 // 5秒内连接双方都无数据，则发起保活探测（该值默认为2小时）
#define  TCP_KEEPINTVL_DEFAULT    1000UL		 // 每1秒发送一次保活探测（该值默认为75S）
//保活机制启动后，一共发送5次保活探测包，如果这5个包对方均无回应，则表示连接异常，内核关闭连接，并发送err回调到用户程序
#define  TCP_KEEPCNT_DEFAULT      5UL  			 
#define  TCP_MAXIDLE  TCP_KEEPCNT_DEFAULT * TCP_KEEPINTVL_DEFAULT



#define MQTT_DEBUG                  LWIP_DBG_OFF


/*
   ---------------------------------
   ---------- OS options ----------
   ---------------------------------
*/


#define TCPIP_THREAD_NAME              "tcpip_thread"
#define TCPIP_THREAD_STACKSIZE          512
#define TCPIP_MBOX_SIZE                 12
#define DEFAULT_UDP_RECVMBOX_SIZE       12
#define DEFAULT_TCP_RECVMBOX_SIZE       12
#define DEFAULT_ACCEPTMBOX_SIZE         12
#define DEFAULT_THREAD_STACKSIZE        512
#define TCPIP_THREAD_PRIO               (4)


#endif /* __LWIPOPTS_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
