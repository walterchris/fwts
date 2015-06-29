/*
 * Copyright (C) 2010-2015 Canonical
 *
 * Portions of this code original from the Linux-ready Firmware Developer Kit
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "fwts.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>

static fwts_acpi_table_info *table;

static int tcpa_init(fwts_framework *fw)
{
	if (fwts_acpi_find_table(fw, "TCPA", 0, &table) != FWTS_OK) {
		fwts_log_error(fw, "Cannot load ACPI table");
		return FWTS_ERROR;
	}
	if (table == NULL) {
		fwts_log_error(fw, "ACPI TCPA table does not exist, skipping test");
		return FWTS_ERROR;
	}

	return FWTS_OK;
}

static int tcpa_client_test(fwts_framework *fw, fwts_acpi_table_tcpa *tcpa)
{
	bool passed = true;

	if (tcpa->header.length != 50) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadSize",
			"TCPA size is incorrect, expecting 0x32 bytes, "
			"instead got 0x%8.8" PRIx32 " bytes",
			tcpa->header.length);
	}

	if (tcpa->header.revision != 2) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadRevision",
			"TCPA revision is incorrect, expecting 0x02, "
			"instead got 0x%2.2" PRIx8,
			tcpa->header.revision);
	}

	fwts_log_info_verbatum(fw, "TCPA Table:");
	fwts_log_info_verbatum(fw, "  Platform Class:                  0x%4.4"   PRIx16, tcpa->platform_class);
	fwts_log_info_verbatum(fw, "  Log Area Minimum Length:         0x%8.8"   PRIx32, tcpa->client.log_zone_length);
	fwts_log_info_verbatum(fw, "  Log Area Start Address:          0x%16.16" PRIx64, tcpa->client.log_zone_addr);

	return passed;
}

static int tcpa_server_test(fwts_framework *fw, fwts_acpi_table_tcpa *tcpa)
{
	bool passed = true;
	uint32_t reserved2;

	if (tcpa->header.length != 100) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadSize",
			"TCPA size is incorrect, expecting 0x64 bytes, "
			"instead got 0x%8.8" PRIx32 " bytes",
			tcpa->header.length);
	}

	if (tcpa->header.revision != 2) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadRevision",
			"TCPA revision is incorrect, expecting 0x02, "
			"instead got 0x%2.2" PRIx8,
			tcpa->header.revision);
	}

	reserved2 = tcpa->server.reserved2[0] + (tcpa->server.reserved2[1] << 4) + (tcpa->server.reserved2[2] << 8);

	fwts_log_info_verbatum(fw, "TCPA Table:");
	fwts_log_info_verbatum(fw, "  Platform Class:                  0x%4.4"   PRIx16, tcpa->platform_class);
	fwts_log_info_verbatum(fw, "  Reserved:	                       0x%4.4"   PRIx16, tcpa->server.reserved);
	fwts_log_info_verbatum(fw, "  Log Area Minimum Length:         0x%16.16" PRIx64, tcpa->server.log_zone_length);
	fwts_log_info_verbatum(fw, "  Log Area Start Address:          0x%16.16" PRIx64, tcpa->server.log_zone_addr);
	fwts_log_info_verbatum(fw, "  Specification Revision:          0x%4.4"   PRIx16, tcpa->server.spec_revision);
	fwts_log_info_verbatum(fw, "  Device Flags:                    0x%2.2"   PRIx16, tcpa->server.device_flag);
	fwts_log_info_verbatum(fw, "  Interrupt Flags:                 0x%2.2"   PRIx16, tcpa->server.interrupt_flag);
	fwts_log_info_verbatum(fw, "  GPE:                             0x%2.2"   PRIx16, tcpa->server.gpe);
	fwts_log_info_verbatum(fw, "  Reserved:                        0x%8.8"   PRIx32, reserved2);
	fwts_log_info_verbatum(fw, "  Global System Interrupt:         0x%8.8"   PRIx32, tcpa->server.global_sys_interrupt);
	fwts_log_info_verbatum(fw, "  Base Address:");
	fwts_log_info_verbatum(fw, "    Address Space ID:              0x%2.2"   PRIx8, tcpa->server.base_addr.address_space_id);
	fwts_log_info_verbatum(fw, "    Register Bit Width             0x%2.2"   PRIx8, tcpa->server.base_addr.register_bit_width);
	fwts_log_info_verbatum(fw, "    Register Bit Offset            0x%2.2"   PRIx8, tcpa->server.base_addr.register_bit_offset);
	fwts_log_info_verbatum(fw, "    Access Size                    0x%2.2"   PRIx8, tcpa->server.base_addr.access_width);
	fwts_log_info_verbatum(fw, "    Address                        0x%16.16" PRIx64, tcpa->server.base_addr.address);
	fwts_log_info_verbatum(fw, "  Reserved:                        0x%8.8"   PRIx32, tcpa->server.reserved3);
	fwts_log_info_verbatum(fw, "  Configuration Address:");
	fwts_log_info_verbatum(fw, "    Address Space ID:              0x%2.2"   PRIx8, tcpa->server.config_addr.address_space_id);
	fwts_log_info_verbatum(fw, "    Register Bit Width             0x%2.2"   PRIx8, tcpa->server.config_addr.register_bit_width);
	fwts_log_info_verbatum(fw, "    Register Bit Offset            0x%2.2"   PRIx8, tcpa->server.config_addr.register_bit_offset);
	fwts_log_info_verbatum(fw, "    Access Size                    0x%2.2"   PRIx8, tcpa->server.config_addr.access_width);
	fwts_log_info_verbatum(fw, "    Address                        0x%16.16" PRIx64, tcpa->server.config_addr.address);
	fwts_log_info_verbatum(fw, "  PCI Segment Group:               0x%2.2"   PRIx8, tcpa->server.pci_seg_number);
	fwts_log_info_verbatum(fw, "  PCI Bus:                         0x%2.2"   PRIx8, tcpa->server.pci_bus_number);
	fwts_log_info_verbatum(fw, "  PCI Device:                      0x%2.2"   PRIx8, tcpa->server.pci_dev_number);
	fwts_log_info_verbatum(fw, "  PCI Function:                    0x%2.2"   PRIx8, tcpa->server.pci_func_number);

	if (tcpa->server.reserved != 0) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"TCPAReservedNonZero",
			"TCPA first reserved field must be zero, got "
			"0x%2.2" PRIx8 " instead", tcpa->server.reserved);
	}

	if (reserved2 != 0) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"TCPAReservedNonZero",
			"TCPA second reserved field must be zero, got "
			"0x%2.2" PRIx8 " instead", reserved2);
	}

	if (tcpa->server.reserved3 != 0) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_LOW,
			"TCPAReservedNonZero",
			"TCPA third reserved field must be zero, got "
			"0x%2.2" PRIx8 " instead", tcpa->server.reserved3);
	}

	if (tcpa->server.device_flag & 1) {
		if (!(tcpa->server.interrupt_flag & 2)) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"TCPABadInterruptPolarity",
				"TCPA interrupt Polarity should be one, got zero");
		}

		if (tcpa->server.interrupt_flag & 1) {
			passed = false;
			fwts_failed(fw, LOG_LEVEL_HIGH,
				"TCPABadInterruptMode",
				"TCPA interrupt mode should be zero, got one");
		}
	}

	if (tcpa->server.base_addr.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY &&
	    tcpa->server.base_addr.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadAddressID",
			"TCPA base address ID must be 1 or zero, got "
			"0x%2.2" PRIx8 " instead", tcpa->server.base_addr.address_space_id);
	}

	if (tcpa->server.config_addr.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_MEMORY &&
	    tcpa->server.config_addr.address_space_id != FWTS_GAS_ADDR_SPACE_ID_SYSTEM_IO) {
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadAddressID",
			"TCPA configuration address ID must be 1 or zero, got "
			"0x%2.2" PRIx8 " instead", tcpa->server.config_addr.address_space_id);
	}

	return passed;
}

/*
 * TCPA table
 *   available @ https://www.trustedcomputinggroup.org/files/static_page_files/5DB17390-1A4B-B294-D029166C91F3512B/TCG_D-RTM_Architecture_v1%200_Published_06172013.pdf
 */
static int tcpa_test1(fwts_framework *fw)
{
	fwts_acpi_table_tcpa *tcpa = (fwts_acpi_table_tcpa*)table->data;
	bool passed = true;

	switch (tcpa->platform_class) {
	case 0:
		passed = tcpa_client_test(fw, tcpa);
		break;
	case 1:
		passed = tcpa_server_test(fw, tcpa);
		break;
	default:
		passed = false;
		fwts_failed(fw, LOG_LEVEL_HIGH,
			"TCPABadPlatformClass",
			"TCPA's platform class must be zero or one, got 0x%" PRIx16,
			tcpa->platform_class);
		break;
	}

	if (passed)
		fwts_passed(fw, "No issues found in TCPA table.");

	return FWTS_OK;
}

static fwts_framework_minor_test tcpa_tests[] = {
	{ tcpa_test1, "Validate TCPA table." },
	{ NULL, NULL }
};

static fwts_framework_ops tcpa_ops = {
	.description = "Trusted Computing Platform Alliance Capabilities Table test.",
	.init        = tcpa_init,
	.minor_tests = tcpa_tests
};

FWTS_REGISTER("tcpa", &tcpa_ops, FWTS_TEST_ANYTIME, FWTS_FLAG_BATCH | FWTS_FLAG_TEST_ACPI)
