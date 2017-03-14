/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2016 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_ip.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_errno.h>

static volatile bool force_quit;

/* MAC updating enabled by default */
static int mac_updating = 1;
uint64_t flow_table_id, rule_table_id;
#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1

#define NB_MBUF   8192

#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256
#define MAX_MULTIREAD 128

struct saved_packet {
        struct rte_mbuf *packet;
        uint8_t ttl;
        uint64_t key_hash;
        uint64_t table_id; // current table the packet reads from
} __attribute__((__packed__));

#define RING_SIZE 16384

#define SYN 2
#define ACK 16
#define SYN_ACK 18
#define FIN 17
#define FIN_ACK 27
struct entry_data{
	uint8_t flag;
};
struct entry_data * _returned_e_data;
/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 128
#define RTE_TEST_TX_DESC_DEFAULT 512
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

/* ethernet addresses of ports */
static struct ether_addr l2fwd_ports_eth_addr[RTE_MAX_ETHPORTS];

/* mask of enabled ports */
static uint32_t l2fwd_enabled_port_mask = 0;

/* list of enabled ports */
static uint32_t l2fwd_dst_ports[RTE_MAX_ETHPORTS];

static unsigned int l2fwd_rx_queue_per_lcore = 1;

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16

struct mbuf_table {
        unsigned len;
        struct rte_mbuf *m_table[MAX_PKT_BURST];
};

struct lcore_queue_conf {
	unsigned n_rx_port;
	unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
	struct mbuf_table tx_mbufs[RTE_MAX_ETHPORTS];
} __rte_cache_aligned;
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

static struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

static const struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 0, /**< IP checksum offload disabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 0, /**< CRC stripped by hardware */
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

struct rte_mempool * l2fwd_pktmbuf_pool = NULL;

/* Per-port statistics struct */
struct l2fwd_port_statistics {
	uint64_t tx;
	uint64_t rx;
	uint64_t dropped;
} __rte_cache_aligned;
struct l2fwd_port_statistics port_statistics[RTE_MAX_ETHPORTS];

#define MAX_TIMER_PERIOD 86400 /* 1 day max */
/* A tsc-based timer responsible for triggering statistics printout */
static uint64_t timer_period = 10; /* default period is 10 seconds */


/* Send the burst of packets on an output interface */
static int
l2fwd_send_burst(struct lcore_queue_conf *qconf, unsigned n, uint8_t port)
{
	//printf("send_burst entering.. \n");
	struct rte_mbuf **m_table;
	unsigned ret;
	unsigned queueid =0;

	m_table = (struct rte_mbuf **)qconf->tx_mbufs[port].m_table;

	ret = rte_eth_tx_burst(port, (uint16_t) queueid, m_table, (uint16_t) n);
	port_statistics[port].tx += ret;
	if (unlikely(ret < n)) {
		port_statistics[port].dropped += (n - ret);
		do {
			rte_pktmbuf_free(m_table[ret]);
		} while (++ret < n);
	}

	return 0;
}

/* Enqueue packets for TX and prepare them to be sent */
static int
l2fwd_send_packet(struct rte_mbuf *m, uint8_t port)
{
	//printf("send_packet entering..\n");
	unsigned lcore_id, len;
	struct lcore_queue_conf *qconf;
	lcore_id = rte_lcore_id();
	struct ether_hdr *eth_hdr;
        eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);

	//printf("send_packet:: just get lcore_id %d\n", lcore_id);

	qconf = &lcore_queue_conf[lcore_id];
	len = qconf->tx_mbufs[port].len;

	//printf("send_packet:: about to prepare tx_mbufs\n");
	qconf->tx_mbufs[port].m_table[len] = m;
	len++;

	//printf("send_packet:: configured core, about to enter loop to burst packets\n");
	/* enough pkts to be sent */
	if (unlikely(len == MAX_PKT_BURST)) {
		l2fwd_send_burst(qconf, MAX_PKT_BURST, port);
		len = 0;
	}
	else{
                if (eth_hdr->ether_type == rte_cpu_to_be_16(ETHER_TYPE_ARP)) {
                        printf("Forwarding ARP packet \n");
                        rte_eth_tx_burst(port, 0, &m, 1);
                }
        }


	qconf->tx_mbufs[port].len = len;
	return 0;
}



static void
l2fwd_simple_forward(struct rte_mbuf *m, unsigned portid, bool drop)
{
	//printf("simple_forward entering with portid %u \n", portid);
	struct ether_hdr *eth;
	void *tmp;
	unsigned dst_port;
	//struct ipv4_hdr *ipv4_h;
     //   void *d_addr_bytes;
        //unsigned char a, b, c, d;
 
	//uint32_t src_ip, dst_ip, hash_key;
	
	// getting to get 5-tuple data

	dst_port = l2fwd_dst_ports[portid];
	//printf("About to get eth info from packet\n");
	eth = rte_pktmbuf_mtod(m, struct ether_hdr *);

	tmp = &eth->d_addr.addr_bytes[0];
	 if(drop){
                printf("dropping packets\n");
                *((uint64_t *)tmp) = 0xa64e84bae299;
        }
        else{
	switch (eth->s_addr.addr_bytes[5]){
                case 148 :
                        //printf("Hey got packet from server with dst-MAC: %02X:%02X:%02X:%02X:%02X:%02X , forwarding to client\n",
                                //eth->d_addr.addr_bytes[0],
                                //eth->d_addr.addr_bytes[1],
                                //eth->d_addr.addr_bytes[2],
                                //eth->d_addr.addr_bytes[3],
                                //eth->d_addr.addr_bytes[4],
                                //eth->d_addr.addr_bytes[5]);
                        *((uint64_t *)tmp) = 0xa64e84bae290; // client-p5p1
                        //*((uint64_t *)tmp) = 0x956f8bbae290; //controller-p2p2

                        break;

                case 166 :
                        //printf("Hey got packet from client, forward to server\n");
                        *((uint64_t *)tmp) = 0x946f8bbae290;
                        break;

                default:
                        //printf("Hey got packet from uknown source with MAC %u\n", eth->s_addr.addr_bytes[5]);
			//*((uint64_t *)tmp) = 0xa64e84bae290;
                        break;

		/*case 166 : // from p5p1, forwarding to p5p2
                        //printf("Hey got packet from p5p1 with src-MAC: %02X:%02X:%02X:%02X:%02X:%02X \n",
                                //eth->s_addr.addr_bytes[0],
                                //eth->s_addr.addr_bytes[1],
                                //eth->s_addr.addr_bytes[2],
                                //eth->s_addr.addr_bytes[3],
                                //eth->s_addr.addr_bytes[4],
                                //eth->s_addr.addr_bytes[5]);
			//printf("MAC last digit %u\n", eth->s_addr.addr_bytes[5]);
			*((uint64_t *)tmp) = 0xa74e84bae290;
                        break;

		case 32 : // p6p1, forwarding to p6p2
			*((uint64_t *)tmp) = 0x214684bae290;
			break;

                default:
			
                        //printf("Hey got packet from uknown with src-MAC: %02X:%02X:%02X:%02X:%02X:%02X \n",
                        //        eth->s_addr.addr_bytes[0],
                        //        eth->s_addr.addr_bytes[1],
                        //        eth->s_addr.addr_bytes[2],
                        //        eth->s_addr.addr_bytes[3],
                        //        eth->s_addr.addr_bytes[4],
                        //        eth->s_addr.addr_bytes[5]);
			//printf("MAC last digit %u\n", eth->s_addr.addr_bytes[5]);
                        break;*/

        }}

	/* src addr */
	ether_addr_copy(&l2fwd_ports_eth_addr[dst_port], &eth->s_addr);
	//printf("About to forward packet\n");
	l2fwd_send_packet(m, (uint8_t) dst_port);
}


/* display usage */
static void
l2fwd_usage(const char *prgname)
{
	printf("%s [EAL options] -- -p PORTMASK [-q NQ]\n"
	       "  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
	       "  -q NQ: number of queue (=ports) per lcore (default is 1)\n"
		   "  -T PERIOD: statistics will be refreshed each PERIOD seconds (0 to disable, 10 default, 86400 maximum)\n"
		   "  --[no-]mac-updating: Enable or disable MAC addresses updating (enabled by default)\n"
		   "      When enabled:\n"
		   "       - The source MAC address is replaced by the TX port MAC address\n"
		   "       - The destination MAC address is replaced by 02:00:00:00:00:TX_PORT_ID\n",
	       prgname);
}

static int
l2fwd_parse_portmask(const char *portmask)
{
	char *end = NULL;
	unsigned long pm;

	/* parse hexadecimal string */
	pm = strtoul(portmask, &end, 16);
	if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;

	if (pm == 0)
		return -1;

	return pm;
}

static unsigned int
l2fwd_parse_nqueue(const char *q_arg)
{
	char *end = NULL;
	unsigned long n;

	/* parse hexadecimal string */
	n = strtoul(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return 0;
	if (n == 0)
		return 0;
	if (n >= MAX_RX_QUEUE_PER_LCORE)
		return 0;

	return n;
}

static int
l2fwd_parse_timer_period(const char *q_arg)
{
	char *end = NULL;
	int n;

	/* parse number string */
	n = strtol(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;
	if (n >= MAX_TIMER_PERIOD)
		return -1;

	return n;
}

/* Parse the argument given in the command line of the application */
static int
l2fwd_parse_args(int argc, char **argv)
{
	int opt, ret, timer_secs;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];
	static struct option lgopts[] = {
		{ "mac-updating", no_argument, &mac_updating, 1},
		{ "no-mac-updating", no_argument, &mac_updating, 0},
		{NULL, 0, 0, 0}
	};

	argvopt = argv;

	while ((opt = getopt_long(argc, argvopt, "p:q:T:",
				  lgopts, &option_index)) != EOF) {

		switch (opt) {
		/* portmask */
		case 'p':
			l2fwd_enabled_port_mask = l2fwd_parse_portmask(optarg);
			if (l2fwd_enabled_port_mask == 0) {
				printf("invalid portmask\n");
				l2fwd_usage(prgname);
				return -1;
			}
			break;

		/* nqueue */
		case 'q':
			l2fwd_rx_queue_per_lcore = l2fwd_parse_nqueue(optarg);
			if (l2fwd_rx_queue_per_lcore == 0) {
				printf("invalid queue number\n");
				l2fwd_usage(prgname);
				return -1;
			}
			break;

		/* timer period */
		case 'T':
			timer_secs = l2fwd_parse_timer_period(optarg);
			if (timer_secs < 0) {
				printf("invalid timer period\n");
				l2fwd_usage(prgname);
				return -1;
			}
			timer_period = timer_secs;
			break;

		/* long options */
		case 0:
			break;

		default:
			l2fwd_usage(prgname);
			return -1;
		}
	}

	if (optind >= 0)
		argv[optind-1] = prgname;

	ret = optind-1;
	optind = 0; /* reset getopt lib */
	return ret;
}

/* Check the link status of all ports in up to 9s, and print them finally */
static void
check_all_ports_link_status(uint8_t port_num, uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint8_t portid, count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		if (force_quit)
			return;
		all_ports_up = 1;
		for (portid = 0; portid < port_num; portid++) {
			if (force_quit)
				return;
			if ((port_mask & (1 << portid)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			rte_eth_link_get_nowait(portid, &link);
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf("Port %d Link Up - speed %u "
						"Mbps - %s\n", (uint8_t)portid,
						(unsigned)link.link_speed,
				(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
					("full-duplex") : ("half-duplex\n"));
				else
					printf("Port %d Link Down\n",
						(uint8_t)portid);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == ETH_LINK_DOWN) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf("done\n");
		}
	}
}

static void
signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
	}
}

/**
 * This thread receives mbufs from the port and then passed to 
 * the tx-threads via the rx_to_workers ring.
 */
static int
rx_thread(struct rte_ring *ring_out)
{
     //   const uint8_t nb_ports = rte_eth_dev_count();
    //    uint32_t seqn = 0;
        uint16_t i, ret = 0;
        uint16_t nb_rx_pkts;
        uint8_t port_id = 0;
        struct rte_mbuf *pkts[MAX_PKT_BURST];

        RTE_LOG(INFO, L2FWD, "%s() started on lcore %u\n", __func__,
                                                        rte_lcore_id());

        while (!force_quit) {
		/* receive packets */
	        nb_rx_pkts = rte_eth_rx_burst(port_id, 0, pkts, MAX_PKT_BURST);
	        if (nb_rx_pkts == 0) {
			//RTE_LOG(DEBUG, L2FWD, "():Received zero packets\n");
	                continue;
	        }
	        /* enqueue to rx_to_workers ring */
	        ret = rte_ring_enqueue_burst(ring_out, (void *const*)pkts, nb_rx_pkts);
	        /*app_stats.rx.enqueue_pkts += ret;
	        if (unlikely(ret < nb_rx_pkts)) {
			//app_stats.rx.enqueue_failed_pkts +=
	                //(nb_rx_pkts-ret);
	                pktmbuf_free_bulk(&pkts[ret], nb_rx_pkts - ret);
	        }*/
	}
        return 0;
}

/**
 * Dequeue mbufs from the workers_to_tx ring, do multiread, then transmit them
 */
static int
tx_thread(struct rte_ring *ring_in)
{
        uint32_t i, dqnum;
        uint8_t outp;
	_returned_e_data = (struct entry_data*) malloc(sizeof(struct entry_data));
        //struct rte_mbuf *mbufs1[MAX_PKT_BURST], *mbufs2[MAX_PKT_BURST], *mbufs3[MAX_PKT_BURST], *m;
	
        struct output_buffer *outbuf;
	unsigned portid = 0;
        uint64_t b_time, e_time, bc_time, ec_time;
	int mbuf_index=0;
	struct rte_mbuf *mbufs1[MAX_MULTIREAD], *m;
	struct saved_packet saved_packets[MAX_MULTIREAD];
	uint64_t pkt_key=0;
	bool max_reached = false;

        RTE_LOG(INFO, L2FWD, "%s() started on lcore %u\n", __func__,
                                                        rte_lcore_id());
        while (!force_quit) {


		if(rte_ring_dequeue(ring_in, (void **)&m) != 0){
			continue;
		}
		if (RTE_ETH_IS_IPV4_HDR(m->packet_type)) {
			struct ether_hdr *eth_h;
			eth_h = rte_pktmbuf_mtod(m, struct ether_hdr *);
			if (eth_h->ether_type == rte_cpu_to_be_16(ETHER_TYPE_ARP)) {
	                        printf("Received ARP packet in tx thread \n");
				l2fwd_simple_forward(m, 0, false);
				continue;
			}

			struct ipv4_hdr *ipv4_h;
			uint32_t src_ip, dst_ip;

                	// Handle IPv4 headers to calculate hash key.
	                ipv4_h = (struct ipv4_hdr *)(rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, sizeof(struct ether_hdr)));
			// check source MAC to determine flow direction
			src_ip = (uint32_t)ipv4_h->src_addr;
                        dst_ip = (uint32_t)ipv4_h->dst_addr;

			switch (eth_h->s_addr.addr_bytes[5]){
				case 148 : // packet coming from server, swap
				pkt_key = ((size_t)(dst_ip) * 59) ^
	                                ((size_t)(src_ip)) ^
					((size_t)(rte_bswap16(*((uint16_t *)(ipv4_h + 1) +1))) << 16) ^
                	                ((size_t)(rte_bswap16(*(uint16_t *)(ipv4_h + 1)))) ^
                        	        ((size_t)(ipv4_h->next_proto_id));
        	                break;

		                case 166 :
				pkt_key = ((size_t)(src_ip) * 59) ^
	                                ((size_t)(dst_ip)) ^
        	                        ((size_t)(rte_bswap16(*(uint16_t *)(ipv4_h + 1))) << 16) ^
                	                ((size_t)(rte_bswap16(*((uint16_t *)(ipv4_h + 1) + 1)))) ^
                        	        ((size_t)(ipv4_h->next_proto_id));
        	                break;

		                default:
				pkt_key = ((size_t)(src_ip) * 59) ^
                                        ((size_t)(dst_ip)) ^
                                        ((size_t)(rte_bswap16(*(uint16_t *)(ipv4_h + 1))) << 16) ^
                                        ((size_t)(rte_bswap16(*((uint16_t *)(ipv4_h + 1) + 1)))) ^
                                        ((size_t)(ipv4_h->next_proto_id));
				break;
        		}
        	        saved_packets[mbuf_index].packet = m;
			saved_packets[mbuf_index].key_hash = pkt_key;
			saved_packets[mbuf_index].table_id = flow_table_id;
//			local_read(saved_packets[mbuf_index]);
			mbuf_index++;
		}
		else{
			l2fwd_simple_forward(m, 0, false);
		}
		if (mbuf_index == MAX_MULTIREAD)
			mbuf_index = 0;
        }

        return 0;
}

int
main(int argc, char **argv)
{
	struct lcore_queue_conf *qconf;
	struct rte_eth_dev_info dev_info;
	int ret;
	uint8_t nb_ports;
	uint8_t nb_ports_available;
	uint8_t portid, last_port;
	unsigned lcore_id, rx_lcore_id;
	unsigned nb_ports_in_mask = 0;

	/* init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
	argc -= ret;
	argv += ret;

	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* parse application arguments (after the EAL ones) */
	ret = l2fwd_parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid L2FWD arguments\n");

	printf("MAC updating %s\n", mac_updating ? "enabled" : "disabled");

	/* convert to number of cycles */
	timer_period *= rte_get_timer_hz();

	/* create the mbuf pool */
	l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF,
		MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
		rte_socket_id());
	if (l2fwd_pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

	nb_ports = rte_eth_dev_count();
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

	/* reset l2fwd_dst_ports */
	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++)
		l2fwd_dst_ports[portid] = 0;
	last_port = 0;

	/*
	 * Each logical core is assigned a dedicated TX queue on each port.
	 */
	for (portid = 0; portid < nb_ports; portid++) {
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;

		if (nb_ports_in_mask % 2) {
			l2fwd_dst_ports[portid] = last_port;
			l2fwd_dst_ports[last_port] = portid;
		}
		else
			last_port = portid;

		nb_ports_in_mask++;

		rte_eth_dev_info_get(portid, &dev_info);
	}
	if (nb_ports_in_mask % 2) {
		printf("Notice: odd number of ports in portmask.\n");
		l2fwd_dst_ports[last_port] = last_port;
	}

	rx_lcore_id = 0;
	qconf = NULL;

	/* Initialize the port/queue configuration of each logical core */
	for (portid = 0; portid < nb_ports; portid++) {
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;

		/* get the lcore_id for this port */
		while (rte_lcore_is_enabled(rx_lcore_id) == 0 ||
		       lcore_queue_conf[rx_lcore_id].n_rx_port ==
		       l2fwd_rx_queue_per_lcore) {
			rx_lcore_id++;
			if (rx_lcore_id >= RTE_MAX_LCORE)
				rte_exit(EXIT_FAILURE, "Not enough cores\n");
		}

		if (qconf != &lcore_queue_conf[rx_lcore_id])
			/* Assigned a new logical core in the loop above. */
			qconf = &lcore_queue_conf[rx_lcore_id];

		qconf->rx_port_list[qconf->n_rx_port] = portid;
		qconf->n_rx_port++;
		printf("Lcore %u: RX port %u\n", rx_lcore_id, (unsigned) portid);
	}

	nb_ports_available = nb_ports;

	/* Initialise each port */
	for (portid = 0; portid < nb_ports; portid++) {
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0) {
			printf("Skipping disabled port %u\n", (unsigned) portid);
			nb_ports_available--;
			continue;
		}
		/* init port */
		printf("Initializing port %u... ", (unsigned) portid);
		fflush(stdout);
		ret = rte_eth_dev_configure(portid, 1, 1, &port_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
				  ret, (unsigned) portid);

		rte_eth_macaddr_get(portid,&l2fwd_ports_eth_addr[portid]);

		/* init one RX queue */
		fflush(stdout);
		ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
					     rte_eth_dev_socket_id(portid),
					     NULL,
					     l2fwd_pktmbuf_pool);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
				  ret, (unsigned) portid);

		/* init one TX queue on each port */
		fflush(stdout);
		ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
				rte_eth_dev_socket_id(portid),
				NULL);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
				ret, (unsigned) portid);

		/* Initialize TX buffers */
		tx_buffer[portid] = rte_zmalloc_socket("tx_buffer",
				RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
				rte_eth_dev_socket_id(portid));
		if (tx_buffer[portid] == NULL)
			rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
					(unsigned) portid);

		rte_eth_tx_buffer_init(tx_buffer[portid], MAX_PKT_BURST);

		ret = rte_eth_tx_buffer_set_err_callback(tx_buffer[portid],
				rte_eth_tx_buffer_count_callback,
				&port_statistics[portid].dropped);
		if (ret < 0)
				rte_exit(EXIT_FAILURE, "Cannot set error callback for "
						"tx buffer on port %u\n", (unsigned) portid);

		/* Start device */
		ret = rte_eth_dev_start(portid);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
				  ret, (unsigned) portid);

		printf("done: \n");

		rte_eth_promiscuous_enable(portid);

		printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
				(unsigned) portid,
				l2fwd_ports_eth_addr[portid].addr_bytes[0],
				l2fwd_ports_eth_addr[portid].addr_bytes[1],
				l2fwd_ports_eth_addr[portid].addr_bytes[2],
				l2fwd_ports_eth_addr[portid].addr_bytes[3],
				l2fwd_ports_eth_addr[portid].addr_bytes[4],
				l2fwd_ports_eth_addr[portid].addr_bytes[5]);

		/* initialize port stats */
		memset(&port_statistics, 0, sizeof(port_statistics));
	}

	if (!nb_ports_available) {
		rte_exit(EXIT_FAILURE,
			"All available ports are disabled. Please set portmask.\n");
	}

	check_all_ports_link_status(nb_ports, l2fwd_enabled_port_mask);

	printf("Creating a ring queue between RX and RAMCloud worker. Socket ID used: %u  \n", rte_socket_id());
        struct rte_ring *rx_to_ramcloud_worker_ring;
        rx_to_ramcloud_worker_ring = rte_ring_create("rx_to_ramcloud_worker", RING_SIZE, rte_socket_id(),
                        RING_F_SP_ENQ);
        if (rx_to_ramcloud_worker_ring == NULL)
                rte_exit(EXIT_FAILURE, "%s\n", rte_strerror(rte_errno));

	printf("Master core ID: %u \n", rte_get_master_lcore());

	/* Start RAMCloud worker */
        unsigned int rc_lcore_id = 1; //FIXME this should be taken from application arguments 

	if (rte_lcore_is_enabled(rc_lcore_id) && rc_lcore_id != rte_get_master_lcore()){
                rte_eal_remote_launch((lcore_function_t *)tx_thread, rx_to_ramcloud_worker_ring, rc_lcore_id);
        }
        else{
                printf("Error:: Cannot start RAMCloud worker on lcore: %u \n", rc_lcore_id);
                return 0;
        }

    rx_thread(rx_to_ramcloud_worker_ring);


	for (portid = 0; portid < nb_ports; portid++) {
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("Closing port %d...", portid);
		rte_eth_dev_stop(portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}
	printf("Bye...\n");

	return ret;
}
