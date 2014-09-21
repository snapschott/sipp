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
#include "gtest/gtest.h"

/* These xp_parser tests cannot be moved to xp_parser.c because that's
 * a C file and the gtest is for C++. */

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
