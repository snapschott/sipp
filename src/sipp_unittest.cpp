/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Author : Richard GAYRAUD - 04 Nov 2003
 *           Marc LAMBERTON
 *           Olivier JACQUES
 *           Herve PELLAN
 *           David MANSUTTI
 *           Francois-Xavier Kowalski
 *           Gerard Lyonnaz
 *           Francois Draperi (for dynamic_id)
 *           From Hewlett Packard Company.
 *           F. Tarek Rogers
 *           Peter Higginson
 *           Vincent Luba
 *           Shriram Natarajan
 *           Guillaume Teissier from FTR&D
 *           Clement Chen
 *           Wolfgang Beck
 *           Charles P Wright from IBM Research
 *           Martin Van Leeuwen
 *           Andy Aicken
 *           Michael Hirschbichler
 */
#define GLOBALS_FULL_DEFINITION

#include "sipp.hpp"
#include "auth.hpp"
#include "gtest/gtest.h"

TEST(GetHostAndPort, IPv6) {
    int port_result = -1;
    char host_result[255];
    get_host_and_port("fe80::92a4:deff:fe74:7af5", host_result, &port_result);
    EXPECT_EQ(0, port_result);
    EXPECT_STREQ("fe80::92a4:deff:fe74:7af5", host_result);
}

TEST(GetHostAndPort, IPv6Brackets) {
    int port_result = -1;
    char host_result[255];
    get_host_and_port("[fe80::92a4:deff:fe74:7af5]", host_result, &port_result);
    EXPECT_EQ(0, port_result);
    EXPECT_STREQ("fe80::92a4:deff:fe74:7af5", host_result);
}

TEST(GetHostAndPort, IPv6BracketsAndPort) {
    int port_result = -1;
    char host_result[255];
    get_host_and_port("[fe80::92a4:deff:fe74:7af5]:999", host_result, &port_result);
    EXPECT_EQ(999, port_result);
    EXPECT_STREQ("fe80::92a4:deff:fe74:7af5", host_result);
}

TEST(GetHostAndPort, IPv4) {
    int port_result = -1;
    char host_result[255];
    get_host_and_port("127.0.0.1", host_result, &port_result);
    EXPECT_EQ(0, port_result);
    EXPECT_STREQ("127.0.0.1", host_result);
}

TEST(GetHostAndPort, IPv4AndPort) {
    int port_result = -1;
    char host_result[255];
    get_host_and_port("127.0.0.1:999", host_result, &port_result);
    EXPECT_EQ(999, port_result);
    EXPECT_STREQ("127.0.0.1", host_result);
}

TEST(GetHostAndPort, IgnorePort) {
    char host_result[255];
    get_host_and_port("127.0.0.1", host_result, NULL);
    EXPECT_STREQ("127.0.0.1", host_result);
}

TEST(GetHostAndPort, DNS) {
    int port_result = -1;
    char host_result[255];
    get_host_and_port("sipp.sf.net", host_result, &port_result);
    EXPECT_EQ(0, port_result);
    EXPECT_STREQ("sipp.sf.net", host_result);
}

TEST(GetHostAndPort, DNSAndPort) {
    int port_result = -1;
    char host_result[255];
    get_host_and_port("sipp.sf.net:999", host_result, &port_result);
    EXPECT_EQ(999, port_result);
    EXPECT_STREQ("sipp.sf.net", host_result);
}

TEST(DigestAuth, nonce) {
    char nonce[40];
    getAuthParameter("nonce", " Authorization: Digest cnonce=\"c7e1249f\",nonce=\"a6ca2bf13de1433183f7c48781bd9304\"", nonce, sizeof(nonce));
    EXPECT_STREQ("a6ca2bf13de1433183f7c48781bd9304", nonce);
    getAuthParameter("nonce", " Authorization: Digest nonce=\"a6ca2bf13de1433183f7c48781bd9304\", cnonce=\"c7e1249f\"", nonce, sizeof(nonce));
    EXPECT_STREQ("a6ca2bf13de1433183f7c48781bd9304", nonce);
}

TEST(DigestAuth, cnonce) {
    char cnonce[10];
    getAuthParameter("cnonce", " Authorization: Digest cnonce=\"c7e1249f\",nonce=\"a6ca2bf13de1433183f7c48781bd9304\"", cnonce, sizeof(cnonce));
    EXPECT_STREQ("c7e1249f", cnonce);
    getAuthParameter("cnonce", " Authorization: Digest nonce=\"a6ca2bf13de1433183f7c48781bd9304\", cnonce=\"c7e1249f\"", cnonce, sizeof(cnonce));
    EXPECT_STREQ("c7e1249f", cnonce);
}

TEST(DigestAuth, MissingParameter) {
    char cnonce[10];
    getAuthParameter("cnonce", " Authorization: Digest nonce=\"a6ca2bf13de1433183f7c48781bd9304\"", cnonce, sizeof(cnonce));
    EXPECT_EQ('\0', cnonce[0]);
}

TEST(DigestAuth, BasicVerification) {
    char* header = strdup(("Digest \r\n"
                           " realm=\"testrealm@host.com\",\r\n"
                           " nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\"\r\n,"
                           " opaque=\"5ccc069c403ebaf9f0171e9517f40e41\""));
    char result[255];
    createAuthHeader("testuser", "secret", "REGISTER", "sip:example.com", "hello world", header, NULL, NULL, NULL, result);
    EXPECT_STREQ("Digest username=\"testuser\",realm=\"testrealm@host.com\",uri=\"sip:sip:example.com\",nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\",response=\"db94e01e92f2b09a52a234eeca8b90f7\",algorithm=MD5,opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"", result);
    EXPECT_EQ(1, verifyAuthHeader("testuser", "secret", "REGISTER", result, "hello world"));
    free(header);
}

TEST(DigestAuth, qop) {
    char result[1024];
    char* header = strdup(("Digest \r\n"
                           "\trealm=\"testrealm@host.com\",\r\n"
                           "\tqop=\"auth,auth-int\",\r\n"
                           "\tnonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\"\r\n,"
                           "\topaque=\"5ccc069c403ebaf9f0171e9517f40e41\""));
    createAuthHeader("testuser",
                     "secret",
                     "REGISTER",
                     "sip:example.com",
                     "hello world",
                     header,
                     NULL,
                     NULL,
                     NULL,
                     result);
    EXPECT_EQ(1, verifyAuthHeader("testuser", "secret", "REGISTER", result, "hello world"));
    free(header); 
}

TEST(xp_parser, set_xml_buffer_from_string__good) {
    int res;
    int i;
    const char *buffers[] = {
        ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
         "<!DOCTYPE scenario SYSTEM \"sipp.dtd\">\r\n"
         "<!-- quick comment.. -->\r\n"
         "<scenario name=\"Some Scenario\">\e\n"
         "  <send retrans=\"500\"/>\r\n"
         "</scenario>\r\n"), // 1
        ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
         "<!DOCTYPE scenario SYSTEM \"sipp.dtd\">"
         "<!-- quick comment.. -->"
         "<scenario name=\"Some Scenario\">"
         "  <send retrans=\"500\"/>"
         "</scenario>"), // 2
        ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
         "<!DOCTYPE scenario SYSTEM \"sipp.dtd\">"
         "<scenario name=\"Some Scenario\">"
         "  <send retrans=\"500\"/>"
         "</scenario>"), // 3
        ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
         "<scenario name=\"Some Scenario\">"
         "  <send retrans=\"500\"/>"
         "</scenario>"), // 4
        NULL
    };

    for (i = 0; buffers[i]; ++i) {
        const char *elem, *prop;

        res = xp_set_xml_buffer_from_string(buffers[i]);
        EXPECT_EQ(i + 1, res * (i + 1));  // res == 1
        if (!res)
            continue;

        elem = xp_open_element(0);
        EXPECT_STREQ("scenario", elem);

        prop = xp_get_value("name");
        EXPECT_STREQ("Some Scenario", prop);
    }
}

TEST(xp_parser, set_xml_buffer_from_string__bad) {
    int res;
    int i;
    const char *buffers[] = {
        // No <?xml
        ("<!DOCTYPE scenario SYSTEM \"sipp.dtd\">"
         "<!-- quick comment.. -->"
         "<scenario name=\"Some Scenario\">"
         "  <send retrans=\"500\"/>"
         "</scenario>"), // -1
        // Missing ?>
        ("<?xml version=\"1.0\" encoding=\"UTF-8\""
         "<!DOCTYPE scenario SYSTEM \"sipp.dtd\">"
         "<scenario name=\"Some Scenario\">"
         "  <send retrans=\"500\"/>"
         "</scenario>"), // -2
        // Not even a DOCTYPE.
        ("<scenario name=\"Some Scenario\">"
         "  <send retrans=\"500\"/>"
         "</scenario>"), // -3
        NULL
    };

    for (i = 0; buffers[i]; ++i) {
        const char *elem, *prop;

        res = xp_set_xml_buffer_from_string(buffers[i]);
        EXPECT_EQ(-1 - i, (res - 1) * (i + 1)); // res == 0
        if (!res)
            continue;

        elem = xp_open_element(0);
        EXPECT_STREQ("scenario", elem);

        prop = xp_get_value("name");
        EXPECT_STREQ("Some Scenario", prop);
    }
}

TEST(Parser, internal_find_header) {
    char data[] = "OPTIONS sip:server SIP/2.0\r\n"
"Took: abc1\r\n"
"To k: abc2\r\n"
"To\t :\r\n abc3\r\n"
"From: def\r\n"
"\r\n";
    const char *eq = strstr(data, "To\t :");
    EXPECT_STREQ(eq, internal_find_header(data, "To", "t", false));
    EXPECT_STREQ(eq + 8, internal_find_header(data, "To", "t", true));
}

TEST(Parser, get_peer_tag__notag) {
    EXPECT_STREQ(NULL, get_peer_tag("...\r\nTo: <abc>\r\n;tag=notag\r\n\r\n"));
}

TEST(Parser, get_peer_tag__normal) {
    EXPECT_STREQ("normal", get_peer_tag("...\r\nTo: <abc>;t2=x;tag=normal;t3=y\r\n\r\n"));
}

TEST(Parser, get_peer_tag__normal_2) {
    EXPECT_STREQ("normal2", get_peer_tag("...\r\nTo: abc;tag=normal2\r\n\r\n"));
}

TEST(Parser, get_peer_tag__folded) {
    EXPECT_STREQ("folded", get_peer_tag("...\r\nTo: <abc>\r\n ;tag=folded\r\n\r\n"));
}

TEST(Parser, get_peer_tag__space) {
    EXPECT_STREQ("space", get_peer_tag("...\r\nTo: <abc> ;tag=space\r\n\r\n"));
}

TEST(Parser, get_peer_tag__space_2) {
    EXPECT_STREQ("space2", get_peer_tag("...\r\nTo \t:\r\n abc\r\n ;tag=space2\r\n\r\n"));
}
