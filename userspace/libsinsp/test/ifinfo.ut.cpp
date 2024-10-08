// SPDX-License-Identifier: Apache-2.0
/*
Copyright (C) 2023 The Falco Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#include <libsinsp/sinsp.h>
#include <libsinsp/sinsp_int.h>
#include <libsinsp/ifinfo.h>

#include <gtest/gtest.h>

static uint32_t parse_ipv4_addr(const char *dotted_notation) {
	uint32_t a, b, c, d;
	sscanf(dotted_notation, "%d.%d.%d.%d", &a, &b, &c, &d);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return d << 24 | c << 16 | b << 8 | a;
#else
	return d | c << 8 | b << 16 | a << 24;
#endif
}

static uint32_t parse_ipv4_netmask(const char *dotted_notation) {
	return parse_ipv4_addr(dotted_notation);
}

static uint32_t parse_ipv4_broadcast(const char *dotted_notation) {
	return parse_ipv4_addr(dotted_notation);
}

static sinsp_ipv4_ifinfo make_ipv4_interface(const char *addr,
                                             const char *netmask,
                                             const char *broadcast,
                                             const char *name) {
	return sinsp_ipv4_ifinfo(parse_ipv4_addr(addr),
	                         parse_ipv4_netmask(netmask),
	                         parse_ipv4_broadcast(broadcast),
	                         name);
}

static sinsp_ipv4_ifinfo make_ipv4_localhost() {
	return make_ipv4_interface("127.0.0.1", "255.0.0.0", "127.0.0.1", "lo");
}

static void convert_to_string(char *dest, size_t len, uint32_t addr) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	snprintf(dest,
	         len,
	         "%d.%d.%d.%d",
	         (addr & 0xFF),
	         ((addr & 0xFF00) >> 8),
	         ((addr & 0xFF0000) >> 16),
	         ((addr & 0xFF000000) >> 24));
#else
	snprintf(dest,
	         len,
	         "%d.%d.%d.%d",
	         ((addr >> 24) & 0xFF),
	         ((addr >> 16) & 0xFF),
	         ((addr >> 8) & 0xFF),
	         (addr & 0xFF));
#endif
}

#define EXPECT_ADDR_EQ(dotted_notation, addr)      \
	{                                              \
		char buf[17];                              \
		convert_to_string(buf, sizeof(buf), addr); \
		EXPECT_STREQ(dotted_notation, buf);        \
	};

TEST(sinsp_network_interfaces, fd_is_of_wrong_type) {
	sinsp_fdinfo fd;
	fd.m_type = SCAP_FD_UNKNOWN;
	sinsp_network_interfaces interfaces;
	interfaces.update_fd(fd);
}

TEST(sinsp_network_interfaces, socket_is_of_wrong_type) {
	sinsp_fdinfo fd;
	fd.m_type = SCAP_FD_IPV4_SOCK;
	fd.m_sockinfo.m_ipv4info.m_fields.m_l4proto = SCAP_L4_TCP;
	sinsp_network_interfaces interfaces;
	interfaces.update_fd(fd);
}

TEST(sinsp_network_interfaces, sip_and_dip_are_not_zero) {
	sinsp_fdinfo fd;
	fd.m_type = SCAP_FD_IPV4_SOCK;
	fd.m_sockinfo.m_ipv4info.m_fields.m_l4proto = SCAP_L4_UDP;
	fd.m_sockinfo.m_ipv4info.m_fields.m_sip = 1;
	fd.m_sockinfo.m_ipv4info.m_fields.m_dip = 1;
	sinsp_network_interfaces interfaces;
	interfaces.update_fd(fd);
}

TEST(sinsp_network_interfaces, infer_finds_exact_match) {
	sinsp_network_interfaces interfaces;
	interfaces.get_ipv4_list()->push_back(make_ipv4_localhost());
	interfaces.get_ipv4_list()->push_back(
	        make_ipv4_interface("192.168.22.149", "255.255.255.0", "192.168.22.255", "eth0"));
	EXPECT_ADDR_EQ("127.0.0.1", interfaces.infer_ipv4_address(parse_ipv4_addr("127.0.0.1")));
	EXPECT_ADDR_EQ("192.168.22.149",
	               interfaces.infer_ipv4_address(parse_ipv4_addr("192.168.22.149")));
}

TEST(sinsp_network_interfaces, infer_finds_same_subnet) {
	sinsp_network_interfaces interfaces;
	interfaces.get_ipv4_list()->push_back(make_ipv4_localhost());
	interfaces.get_ipv4_list()->push_back(
	        make_ipv4_interface("192.168.22.149", "255.255.255.0", "192.168.22.255", "eth0"));
	EXPECT_ADDR_EQ("192.168.22.149",
	               interfaces.infer_ipv4_address(parse_ipv4_addr("192.168.22.11")));
}

TEST(sinsp_network_interfaces, infer_defaults_to_first_non_loopback) {
	sinsp_network_interfaces interfaces;
	interfaces.get_ipv4_list()->push_back(make_ipv4_localhost());
	interfaces.get_ipv4_list()->push_back(
	        make_ipv4_interface("192.168.22.149", "255.255.255.0", "192.168.22.255", "eth0"));
	interfaces.get_ipv4_list()->push_back(
	        make_ipv4_interface("192.168.22.150", "255.255.255.0", "192.168.22.255", "eth1"));
	EXPECT_ADDR_EQ("192.168.22.149",
	               interfaces.infer_ipv4_address(parse_ipv4_addr("193.168.22.11")));
}

TEST(sinsp_network_interfaces, ipv4_addr_to_string) {
	std::vector<std::pair<sinsp_ipv4_ifinfo, std::string>> ipv4_test_cases = {
	        {make_ipv4_localhost(), "127.0.0.1"},
	        {make_ipv4_interface("192.168.22.149", "255.255.255.0", "192.168.22.255", "eth0"),
	         "192.168.22.149"},
	        {make_ipv4_interface("192.168.22.150", "255.255.255.0", "192.168.22.255", "eth1"),
	         "192.168.22.150"}};

	for(const auto &ipv4_test_case : ipv4_test_cases) {
		std::string ip_str = ipv4_test_case.first.addr_to_string(ipv4_test_case.first.m_addr);
		ASSERT_EQ(ip_str, ipv4_test_case.second);
		ip_str = ipv4_test_case.first.addr_to_string();
		ASSERT_EQ(ip_str, ipv4_test_case.second);
	}
}

TEST(sinsp_network_interfaces, ipv6_addr_to_string) {
	sinsp_ipv6_ifinfo ifinfo;

	std::vector<std::pair<ipv6addr, std::string>> ipv6_test_cases = {
	        {ipv6addr("2001:0db8:85a3:0000:0000:8a2e:0370:7334"),
	         "2001:db8:85a3:0:0:8a2e:370:7334"},
	        {ipv6addr("fe80:0:0:0:2aa:ff:fe9a:4ca3"), "fe80:0:0:0:2aa:ff:fe9a:4ca3"},
	        {ipv6addr("0:0:0:0:0:0:0:0"), "0:0:0:0:0:0:0:0"},
	        {ipv6addr("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"),
	         "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"}};

	for(const auto &ipv6_test_case : ipv6_test_cases) {
		ifinfo.m_net = ipv6_test_case.first;
		std::string addr_str = ifinfo.addr_to_string();
		ASSERT_EQ(addr_str, ipv6_test_case.second);
	}
}
