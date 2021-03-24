/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static const struct rte_eth_conf port_conf_default = {
	.rxmode = {
		.max_rx_pkt_len = RTE_ETHER_MAX_LEN,
	},
};

/* basicfwd.c: Basic DPDK skeleton forwarding example. */

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;
	// struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf txconf;

	// if (!rte_eth_dev_is_valid_port(port))
	// 	return -1;

	// retval = rte_eth_dev_info_get(port, &dev_info);
	// if (retval != 0) {
	// 	printf("Error during getting device (port %u) info: %s\n",
	// 			port, strerror(-retval));
	// 	return retval;
	// }

	// if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
	// 	port_conf.txmode.offloads |=
	// 		DEV_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	// retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	// if (retval != 0)
	// 	return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	// txconf = dev_info.default_txconf;
	// txconf.offloads = port_conf.txmode.offloads;
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
				rte_eth_dev_socket_id(port), &txconf);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	// struct rte_ether_addr addr;
	// retval = rte_eth_macaddr_get(port, &addr);
	// if (retval != 0)
	// 	return retval;

	// printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
	// 		   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
	// 		port,
	// 		addr.addr_bytes[0], addr.addr_bytes[1],
	// 		addr.addr_bytes[2], addr.addr_bytes[3],
	// 		addr.addr_bytes[4], addr.addr_bytes[5]);

	/* Enable RX in promiscuous mode for the Ethernet device. */
	// retval = rte_eth_promiscuous_enable(port);
	// if (retval != 0)
	// 	return retval;

	return 0;
}

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
void
lcore_main(struct rte_mempool *mbuf_pool)
{
	struct rte_mbuf * bufs[BURST_SIZE];
	int i;
	for(i = 0; i < BURST_SIZE; i++) {
		bufs[i] = rte_pktmbuf_alloc(mbuf_pool);
		struct rte_ether_hdr* eth_hdr = rte_pktmbuf_mtod(bufs[i], struct rte_ether_hdr *);
		struct rte_ipv4_hdr* ip_hdr = (struct rte_ipv4_hdr* )(rte_pktmbuf_mtod(bufs[i], char *) + sizeof(struct rte_ether_hdr));
		struct rte_udp_hdr* udp_hdr = (struct rte_udp_hdr* )(rte_pktmbuf_mtod(bufs[i], char *) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
		char* data = (char* )(rte_pktmbuf_mtod(bufs[i], char *) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr));
		struct rte_ether_addr s_addr = {{0x14,0x02,0xEC,0x89,0x8D,0x24}};
		struct rte_ether_addr d_addr =  {{0x14,0x02,0xEC,0x89,0xED,0x54}};
		eth_hdr->d_addr = d_addr;
		eth_hdr->s_addr = s_addr;
		eth_hdr->ether_type = 0x0008;
		ip_hdr-> version_ihl = RTE_IPV4_VHL_DEF;
		ip_hdr-> type_of_service = RTE_IPV4_HDR_DSCP_MASK;
		ip_hdr-> total_length = 0x2800;
		ip_hdr-> packet_id = 0;
		ip_hdr-> fragment_offset = 0;
		ip_hdr-> time_to_live = 100;
		ip_hdr-> src_addr = 0;
		ip_hdr-> next_proto_id = 17;
		ip_hdr-> dst_addr= 0;
		ip_hdr-> hdr_checksum = rte_ipv4_cksum(ip_hdr);
		udp_hdr->src_port = 80;	
		udp_hdr->dst_port = 8080;
		udp_hdr->dgram_len = 0x1400;
		udp_hdr-> dgram_cksum = 1;
		memcpy(data, "it's Shangqy", 12);
		bufs[i]->data_len = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr)+sizeof(struct rte_udp_hdr) + 12;
		bufs[i]->pkt_len = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr)+sizeof(struct rte_udp_hdr) + 12;
	}

	uint16_t nb_tx = rte_eth_tx_burst(0, 0, bufs, BURST_SIZE);
	printf("发送成功%d个包\n", nb_tx);
	
	for(i = 0; i < BURST_SIZE; i++) rte_pktmbuf_free(bufs[i]);
	return;
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool;
	unsigned nb_ports = 1;
	uint16_t portid = 0;

	/* Initialize the Environment Abstraction Layer (EAL). */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	argc -= ret;
	argv += ret;

	/* Check that there is an even number of ports to send/receive on. */
	// nb_ports = rte_eth_dev_count_avail();
	// if (nb_ports < 2 || (nb_ports & 1))
	// 	rte_exit(EXIT_FAILURE, "Error: number of ports must be even\n");

	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/* Initialize all ports. */
	// RTE_ETH_FOREACH_DEV(portid)
	if (port_init(portid, mbuf_pool) != 0)
		rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n",
				portid);

	// if (rte_lcore_count() > 1)
	// 	printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	/* Call lcore_main on the main core only. */
	lcore_main(mbuf_pool);

	return 0;
}
