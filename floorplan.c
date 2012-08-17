//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdarg.h>

#include "model.h"
#include "control.h"
#include "floorplan.h"

void printf_version(FILE* f)
{
	fprintf(f, "fpga_floorplan_format 1\n");
}

#define PRINT_FLAG(fp, flag)	if ((tf) & (flag)) \
				  { fprintf (fp, " %s", #flag); tf &= ~(flag); }

int printf_tiles(FILE* f, struct fpga_model* model)
{
	struct fpga_tile* tile;
	int x, y;

	for (x = 0; x < model->x_width; x++) {
		fprintf(f, "\n");
		for (y = 0; y < model->y_height; y++) {
			tile = &model->tiles[y*model->x_width + x];

			if (tile->type != NA)
				fprintf(f, "tile y%02i x%02i name %s\n", y, x,
					fpga_tiletype_str(tile->type));
			if (tile->flags) {
				int tf = tile->flags;
				fprintf(f, "tile y%02i x%02i flags", y, x);

				PRINT_FLAG(f, TF_FABRIC_ROUTING_COL);
				PRINT_FLAG(f, TF_FABRIC_LOGIC_COL);
				PRINT_FLAG(f, TF_FABRIC_BRAM_VIA_COL);
				PRINT_FLAG(f, TF_FABRIC_MACC_VIA_COL);
				PRINT_FLAG(f, TF_FABRIC_BRAM_COL);
				PRINT_FLAG(f, TF_FABRIC_MACC_COL);
				PRINT_FLAG(f, TF_ROUTING_NO_IO);
				PRINT_FLAG(f, TF_BRAM_DEV);
				PRINT_FLAG(f, TF_MACC_DEV);
				PRINT_FLAG(f, TF_LOGIC_XL_DEV);
				PRINT_FLAG(f, TF_LOGIC_XM_DEV);
				PRINT_FLAG(f, TF_IOLOGIC_DELAY_DEV);
				PRINT_FLAG(f, TF_DCM_DEV);
				PRINT_FLAG(f, TF_PLL_DEV);
				PRINT_FLAG(f, TF_WIRED);

				if (tf) fprintf(f, " 0x%x", tf);
				fprintf(f, "\n");
			}
		}
	}
	return 0;
}

static void printf_wrap(FILE* f, char* line, int prefix_len,
	const char* fmt, ...)
{
	va_list list;
	int i;

	va_start(list, fmt);
	i = strlen(line);
	if (i >= 80) {
		line[i] = '\n';
		line[i+1] = 0;
		fprintf(f, line);
		line[prefix_len] = 0;
		i = prefix_len;
	}	
	line[i] = ' ';
	vsnprintf(&line[i+1], 256, fmt, list);
	va_end(list);
}

static int printf_IOB(FILE* f, struct fpga_model* model,
	int y, int x, int config_only)
{
	struct fpga_tile* tile;
	char line[1024];
	int type_count, plen, i;

	tile = YX_TILE(model, y, x);
	type_count = 0;
	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type != DEV_IOB)
			continue;
		if (config_only && !(tile->devs[i].instantiated)) {
			type_count++;
			continue;
		}
		snprintf(line, sizeof(line), "dev y%02i x%02i IOB %i",
			y, x, type_count);
		type_count++;
		plen = strlen(line);

		printf_wrap(f, line, plen, "type %s", 
			tile->devs[i].iob.subtype == IOBM
				? "M" : "S");
		if (tile->devs[i].iob.istandard[0])
			printf_wrap(f, line, plen, "istd %s", 
				tile->devs[i].iob.istandard);
		if (tile->devs[i].iob.ostandard[0])
			printf_wrap(f, line, plen, "ostd %s", 
				tile->devs[i].iob.ostandard);
		switch (tile->devs[i].iob.bypass_mux) {
			case BYPASS_MUX_I:
				printf_wrap(f, line, plen, "bypass_mux I");
				break;
			case BYPASS_MUX_O:
				printf_wrap(f, line, plen, "bypass_mux O");
				break;
			case BYPASS_MUX_T:
				printf_wrap(f, line, plen, "bypass_mux T");
				break;
			case 0: break; default: EXIT(1);
		}
		switch (tile->devs[i].iob.I_mux) {
			case IMUX_I_B:
				printf_wrap(f, line, plen, "imux I_B");
				break;
			case IMUX_I: 
				printf_wrap(f, line, plen, "imux I");
				break;
			case 0: break; default: EXIT(1);
		}
		if (tile->devs[i].iob.drive_strength)
			printf_wrap(f, line, plen, "strength %i",
				tile->devs[i].iob.drive_strength);
		switch (tile->devs[i].iob.slew) {
			case SLEW_SLOW:
				printf_wrap(f, line, plen, "slew SLOW");
				break;
			case SLEW_FAST:
				printf_wrap(f, line, plen, "slew FAST");
				break;
			case SLEW_QUIETIO:
				printf_wrap(f, line, plen, "slew QUIETIO");
				break;
			case 0: break; default: EXIT(1);
		}
		if (tile->devs[i].iob.O_used)
			printf_wrap(f, line, plen, "O_used");
		switch (tile->devs[i].iob.suspend) {
			case SUSP_LAST_VAL:
				printf_wrap(f, line, plen, "suspend DRIVE_LAST_VALUE");
				break;
			case SUSP_3STATE:
				printf_wrap(f, line, plen, "suspend 3STATE");
				break;
			case SUSP_3STATE_PULLUP:
				printf_wrap(f, line, plen, "suspend 3STATE_PULLUP");
				break;
			case SUSP_3STATE_PULLDOWN:
				printf_wrap(f, line, plen, "suspend 3STATE_PULLDOWN");
				break;
			case SUSP_3STATE_KEEPER:
				printf_wrap(f, line, plen, "suspend 3STATE_KEEPER");
				break;
			case SUSP_3STATE_OCT_ON:
				printf_wrap(f, line, plen, "suspend 3STATE_OCT_ON");
				break;
			case 0: break; default: EXIT(1);
		}
		switch (tile->devs[i].iob.in_term) {
			case ITERM_NONE: 
				printf_wrap(f, line, plen, "in_term NONE");
				break;
			case ITERM_UNTUNED_25:
				printf_wrap(f, line, plen, "in_term UNTUNED_SPLIT_25");
				break;
			case ITERM_UNTUNED_50:
				printf_wrap(f, line, plen, "in_term UNTUNED_SPLIT_50");
				break;
			case ITERM_UNTUNED_75:
				printf_wrap(f, line, plen, "in_term UNTUNED_SPLIT_75");
				break;
			case 0: break; default: EXIT(1);
		}
		switch (tile->devs[i].iob.out_term) {
			case OTERM_NONE: 
				printf_wrap(f, line, plen, "out_term NONE");
				break;
			case OTERM_UNTUNED_25:
				printf_wrap(f, line, plen, "out_term UNTUNED_25");
				break;
			case OTERM_UNTUNED_50:
				printf_wrap(f, line, plen, "out_term UNTUNED_50");
				break;
			case OTERM_UNTUNED_75:
				printf_wrap(f, line, plen, "out_term UNTUNED_75");
				break;
			case 0: break; default: EXIT(1);
		}
		strcat(line, "\n");
		fprintf(f, line);
	}
	return 0;
}

static int read_IOB_attr(struct fpga_model* model, struct fpga_device* dev,
	const char* w1, int w1_len, const char* w2, int w2_len)
{
	// First the one-word attributes.
	if (!str_cmp(w1, w1_len, "O_used", ZTERM)) {
		dev->iob.O_used = 1;
		goto inst_1;
	}
	// The remaining attributes all require 2 words.
	if (w2_len < 1) return 0;
	if (!str_cmp(w1, w1_len, "type", ZTERM))
		return 2; // no reason for instantiation
	if (!str_cmp(w1, w1_len, "istd", ZTERM)) {
		memcpy(dev->iob.istandard, w2, w2_len);
		dev->iob.istandard[w2_len] = 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "ostd", ZTERM)) {
		memcpy(dev->iob.ostandard, w2, w2_len);
		dev->iob.ostandard[w2_len] = 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "bypass_mux", ZTERM)) {
		if (!str_cmp(w2, w2_len, "I", ZTERM))
			dev->iob.bypass_mux = BYPASS_MUX_I;
		else if (!str_cmp(w2, w2_len, "O", ZTERM))
			dev->iob.bypass_mux = BYPASS_MUX_O;
		else if (!str_cmp(w2, w2_len, "T", ZTERM))
			dev->iob.bypass_mux = BYPASS_MUX_T;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "imux", ZTERM)) {
		if (!str_cmp(w2, w2_len, "I_B", ZTERM))
			dev->iob.I_mux = IMUX_I_B;
		else if (!str_cmp(w2, w2_len, "I", ZTERM))
			dev->iob.I_mux = IMUX_I;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "strength", ZTERM)) {
		dev->iob.drive_strength = to_i(w2, w2_len);
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "slew", ZTERM)) {
		if (!str_cmp(w2, w2_len, "SLOW", ZTERM))
			dev->iob.slew = SLEW_SLOW;
		else if (!str_cmp(w2, w2_len, "FAST", ZTERM))
			dev->iob.slew = SLEW_FAST;
		else if (!str_cmp(w2, w2_len, "QUIETIO", ZTERM))
			dev->iob.slew = SLEW_QUIETIO;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "suspend", 7)) {
		if (!str_cmp(w2, w2_len, "DRIVE_LAST_VALUE", ZTERM))
			dev->iob.suspend = SUSP_LAST_VAL;
		else if (!str_cmp(w2, w2_len, "3STATE", ZTERM))
			dev->iob.suspend = SUSP_3STATE;
		else if (!str_cmp(w2, w2_len, "3STATE_PULLUP", ZTERM))
			dev->iob.suspend = SUSP_3STATE_PULLUP;
		else if (!str_cmp(w2, w2_len, "3STATE_PULLDOWN", ZTERM))
			dev->iob.suspend = SUSP_3STATE_PULLDOWN;
		else if (!str_cmp(w2, w2_len, "3STATE_KEEPER", ZTERM))
			dev->iob.suspend = SUSP_3STATE_KEEPER;
		else if (!str_cmp(w2, w2_len, "3STATE_OCT_ON", ZTERM))
			dev->iob.suspend = SUSP_3STATE_OCT_ON;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "in_term", ZTERM)) {
		if (!str_cmp(w2, w2_len, "NONE", ZTERM))
			dev->iob.in_term = ITERM_NONE;
		else if (!str_cmp(w2, w2_len, "UNTUNED_SPLIT_25", ZTERM))
			dev->iob.in_term = ITERM_UNTUNED_25;
		else if (!str_cmp(w2, w2_len, "UNTUNED_SPLIT_50", ZTERM))
			dev->iob.in_term = ITERM_UNTUNED_50;
		else if (!str_cmp(w2, w2_len, "UNTUNED_SPLIT_75", ZTERM))
			dev->iob.in_term = ITERM_UNTUNED_75;
		else return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "out_term", ZTERM)) {
		if (!str_cmp(w2, w2_len, "NONE", ZTERM))
			dev->iob.out_term = OTERM_NONE;
		else if (!str_cmp(w2, w2_len, "UNTUNED_25", ZTERM))
			dev->iob.out_term = OTERM_UNTUNED_25;
		else if (!str_cmp(w2, w2_len, "UNTUNED_50", ZTERM))
			dev->iob.out_term = OTERM_UNTUNED_50;
		else if (!str_cmp(w2, w2_len, "UNTUNED_75", ZTERM))
			dev->iob.out_term = OTERM_UNTUNED_75;
		else return 0;
		goto inst_2;
	}
	return 0;
inst_1:
	dev->instantiated = 1;
	return 1;
inst_2:
	dev->instantiated = 2;
	return 2;
}

static int printf_LOGIC(FILE* f, struct fpga_model* model,
	int y, int x, int config_only)
{
	struct fpga_tile* tile;
	char line[1024];
	int type_count, plen, i;

	tile = YX_TILE(model, y, x);
	type_count = 0;
	for (i = 0; i < tile->num_devs; i++) {
		if (tile->devs[i].type != DEV_LOGIC)
			continue;
		if (config_only && !(tile->devs[i].instantiated)) {
			type_count++;
			continue;
		}
		snprintf(line, sizeof(line), "dev y%02i x%02i LOGIC %i",
			y, x, type_count);
		type_count++;
		plen = strlen(line);

		switch (tile->devs[i].logic.subtype) {
			case LOGIC_X:
				printf_wrap(f, line, plen, "type X");
				break;
			case LOGIC_L:
				printf_wrap(f, line, plen, "type L");
				break;
			case LOGIC_M:
				printf_wrap(f, line, plen, "type M");
				break;
			default: EXIT(1);
		}
		if (tile->devs[i].logic.A_used)
			printf_wrap(f, line, plen, "A_used");
		if (tile->devs[i].logic.B_used)
			printf_wrap(f, line, plen, "B_used");
		if (tile->devs[i].logic.C_used)
			printf_wrap(f, line, plen, "C_used");
		if (tile->devs[i].logic.D_used)
			printf_wrap(f, line, plen, "D_used");
		if (tile->devs[i].logic.A6_lut && tile->devs[i].logic.A6_lut[0])
			printf_wrap(f, line, plen, "A6_lut %s",
				tile->devs[i].logic.A6_lut);
		if (tile->devs[i].logic.B6_lut && tile->devs[i].logic.B6_lut[0])
			printf_wrap(f, line, plen, "B6_lut %s",
				tile->devs[i].logic.B6_lut);
		if (tile->devs[i].logic.C6_lut && tile->devs[i].logic.C6_lut[0])
			printf_wrap(f, line, plen, "C6_lut %s",
				tile->devs[i].logic.C6_lut);
		if (tile->devs[i].logic.D6_lut && tile->devs[i].logic.D6_lut[0])
			printf_wrap(f, line, plen, "D6_lut %s",
				tile->devs[i].logic.D6_lut);
		strcat(line, "\n");
		fprintf(f, line);
	}
	return 0;
}

static int read_LOGIC_attr(struct fpga_model* model, struct fpga_device* dev,
	const char* w1, int w1_len, const char* w2, int w2_len)
{
	// First the one-word attributes.
	if (!str_cmp(w1, w1_len, "A_used", ZTERM)) {
		dev->logic.A_used = 1;
		goto inst_1;
	}
	if (!str_cmp(w1, w1_len, "B_used", ZTERM)) {
		dev->logic.B_used = 1;
		goto inst_1;
	}
	if (!str_cmp(w1, w1_len, "C_used", ZTERM)) {
		dev->logic.C_used = 1;
		goto inst_1;
	}
	if (!str_cmp(w1, w1_len, "D_used", ZTERM)) {
		dev->logic.D_used = 1;
		goto inst_1;
	}
	// The remaining attributes all require 2 words.
	if (w2_len < 1) return 0;
	if (!str_cmp(w1, w1_len, "type", ZTERM))
		return 2; // no reason for instantiation
	if (!str_cmp(w1, w1_len, "A6_lut", ZTERM)) {
		if (fpga_set_lut(model, dev, A6_LUT, w2, w2_len))
			return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "B6_lut", ZTERM)) {
		if (fpga_set_lut(model, dev, B6_LUT, w2, w2_len))
			return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "C6_lut", ZTERM)) {
		if (fpga_set_lut(model, dev, C6_LUT, w2, w2_len))
			return 0;
		goto inst_2;
	}
	if (!str_cmp(w1, w1_len, "D6_lut", ZTERM)) {
		if (fpga_set_lut(model, dev, D6_LUT, w2, w2_len))
			return 0;
		goto inst_2;
	}
	return 0;
inst_1:
	dev->instantiated = 1;
	return 1;
inst_2:
	dev->instantiated = 2;
	return 2;
}

int printf_devices(FILE* f, struct fpga_model* model, int config_only)
{
	int x, y, i, rc;
	struct fpga_tile* tile;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {

			rc = printf_IOB(f, model, y, x, config_only);
			if (rc) goto fail;
			rc = printf_LOGIC(f, model, y, x, config_only);
			if (rc) goto fail;

			tile = YX_TILE(model, y, x);
			for (i = 0; i < tile->num_devs; i++) {
				if (config_only && !(tile->devs[i].instantiated))
					continue;
				switch (tile->devs[i].type) {
					case DEV_MACC:
						fprintf(f, "dev y%02i x%02i DSP48A1\n", y, x);
						break;
					case DEV_TIEOFF:
						fprintf(f, "dev y%02i x%02i TIEOFF\n", y, x);
						break;
					case DEV_ILOGIC:
						fprintf(f, "dev y%02i x%02i ILOGIC\n", y, x);
						break;
					case DEV_OLOGIC:
						fprintf(f, "dev y%02i x%02i OLOGIC\n", y, x);
						break;
					case DEV_IODELAY:
						fprintf(f, "dev y%02i x%02i IODELAY\n", y, x);
						break;
					case DEV_BRAM16:
						fprintf(f, "dev y%02i x%02i RAMB16\n", y, x);
						break;
					case DEV_BRAM8:
						fprintf(f, "dev y%02i x%02i RAMB8\n", y, x);
						break;
					case DEV_BUFH:
						fprintf(f, "dev y%02i x%02i BUFH\n", y, x);
						break;
					case DEV_BUFIO:
						fprintf(f, "dev y%02i x%02i BUFIO2\n", y, x);
						break;
					case DEV_BUFIO_FB:
						fprintf(f, "dev y%02i x%02i BUFIO2FB\n", y, x);
						break;
					case DEV_BUFPLL:
						fprintf(f, "dev y%02i x%02i BUFPLL\n", y, x);
						break;
					case DEV_BUFPLL_MCB:
						fprintf(f, "dev y%02i x%02i BUFPLL_MCB\n", y, x);
						break;
					case DEV_BUFGMUX:
						fprintf(f, "dev y%02i x%02i BUFGMUX\n", y, x);
						break;
					case DEV_BSCAN:
						fprintf(f, "dev y%02i x%02i BSCAN\n", y, x);
						break;
					case DEV_DCM:
						fprintf(f, "dev y%02i x%02i DCM\n", y, x);
						break;
					case DEV_PLL:
						fprintf(f, "dev y%02i x%02i PLL\n", y, x);
						break;
					case DEV_ICAP:
						fprintf(f, "dev y%02i x%02i ICAP\n", y, x);
						break;
					case DEV_POST_CRC_INTERNAL:
						fprintf(f, "dev y%02i x%02i POST_CRC_INTERNAL\n", y, x);
						break;
					case DEV_STARTUP:
						fprintf(f, "dev y%02i x%02i STARTUP\n", y, x);
						break;
					case DEV_SLAVE_SPI:
						fprintf(f, "dev y%02i x%02i SLAVE_SPI\n", y, x);
						break;
					case DEV_SUSPEND_SYNC:
						fprintf(f, "dev y%02i x%02i SUSPEND_SYNC\n", y, x);
						break;
					case DEV_OCT_CALIBRATE:
						fprintf(f, "dev y%02i x%02i OCT_CALIBRATE\n", y, x);
						break;
					case DEV_SPI_ACCESS:
						fprintf(f, "dev y%02i x%02i SPI_ACCESS\n", y, x);
						break;
					case DEV_IOB:
					case DEV_LOGIC:
					case DEV_NONE:
						// to suppress compiler warning
						break;
				}
			}
		}
	}
	return 0;
fail:
	return rc;
}

int printf_ports(FILE* f, struct fpga_model* model)
{
	struct fpga_tile* tile;
	const char* conn_point_name_src;
	int x, y, i, conn_point_dests_o, num_dests_for_this_conn_point;
	int first_port_printed;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			tile = &model->tiles[y*model->x_width + x];

			first_port_printed = 0;
			for (i = 0; i < tile->num_conn_point_names; i++) {
				conn_point_dests_o = tile->conn_point_names[i*2];
				if (i < tile->num_conn_point_names-1)
					num_dests_for_this_conn_point = tile->conn_point_names[(i+1)*2] - conn_point_dests_o;
				else
					num_dests_for_this_conn_point = tile->num_conn_point_dests - conn_point_dests_o;
				if (num_dests_for_this_conn_point)
					// ports is only for connection-less endpoints
					continue;
				conn_point_name_src = strarray_lookup(&model->str, tile->conn_point_names[i*2+1]);
				if (!conn_point_name_src) {
					fprintf(stderr, "Cannot lookup src conn point name index %i, x%i y%i i%i\n",
						tile->conn_point_names[i*2+1], x, y, i);
					continue;
				}
				if (!first_port_printed) {
					first_port_printed = 1;
					fprintf(f, "\n");
				}
				fprintf(f, "port y%02i x%02i %s\n",
					y, x, conn_point_name_src);
			}
		}
	}
	return 0;
}

int printf_conns(FILE* f, struct fpga_model* model)
{
	struct fpga_tile* tile;
	char tmp_line[512];
	const char* conn_point_name_src, *other_tile_connpt_str;
	uint16_t other_tile_connpt_str_i;
	int x, y, i, j, k, conn_point_dests_o, num_dests_for_this_conn_point;
	int other_tile_x, other_tile_y, first_conn_printed;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			tile = &model->tiles[y*model->x_width + x];

			first_conn_printed = 0;
			for (i = 0; i < tile->num_conn_point_names; i++) {
				conn_point_dests_o = tile->conn_point_names[i*2];
				if (i < tile->num_conn_point_names-1)
					num_dests_for_this_conn_point = tile->conn_point_names[(i+1)*2] - conn_point_dests_o;
				else
					num_dests_for_this_conn_point = tile->num_conn_point_dests - conn_point_dests_o;
				if (!num_dests_for_this_conn_point)
					continue;
				conn_point_name_src = strarray_lookup(&model->str, tile->conn_point_names[i*2+1]);
				if (!conn_point_name_src) {
					fprintf(stderr, "Cannot lookup src conn point name index %i, x%i y%i i%i\n",
						tile->conn_point_names[i*2+1], x, y, i);
					continue;
				}
				for (j = 0; j < num_dests_for_this_conn_point; j++) {
					other_tile_x = tile->conn_point_dests[(conn_point_dests_o+j)*3];
					other_tile_y = tile->conn_point_dests[(conn_point_dests_o+j)*3+1];
					other_tile_connpt_str_i = tile->conn_point_dests[(conn_point_dests_o+j)*3+2];

					other_tile_connpt_str = strarray_lookup(&model->str, other_tile_connpt_str_i);
					if (!other_tile_connpt_str) {
						fprintf(stderr, "Lookup err line %i, dest pt %i, dest x%i y%i, from x%i y%i j%i num_dests %i src_pt %s\n",
							__LINE__, other_tile_connpt_str_i, other_tile_x, other_tile_y, x, y, j, num_dests_for_this_conn_point, conn_point_name_src);
						continue;
					}

					if (!first_conn_printed) {
						first_conn_printed = 1;
						fprintf(f, "\n");
					}
					sprintf(tmp_line, "conn y%02i x%02i %s ",
						y, x, conn_point_name_src);
					k = strlen(tmp_line);
					while (k < 45)
						tmp_line[k++] = ' ';
					sprintf(&tmp_line[k], "y%02i x%02i %s\n",
						other_tile_y, other_tile_x, other_tile_connpt_str);
					fprintf(f, tmp_line);
				}
			}
		}
	}
	return 0;
}

int printf_switches(FILE* f, struct fpga_model* model, int enabled_only)
{
	struct fpga_tile* tile;
	int x, y, i, from_connpt_o, to_connpt_o, from_str_i, to_str_i;
	int first_switch_printed, is_bidirectional, is_on;
	const char* from_str, *to_str;

	for (x = 0; x < model->x_width; x++) {
		for (y = 0; y < model->y_height; y++) {
			tile = YX_TILE(model, y, x);

			first_switch_printed = 0;
			for (i = 0; i < tile->num_switches; i++) {
				from_connpt_o = (tile->switches[i] & 0x3FFF8000) >> 15;
				to_connpt_o = tile->switches[i] & 0x00007FFF;
				is_bidirectional = (tile->switches[i] & SWITCH_BIDIRECTIONAL) != 0;
				is_on = (tile->switches[i] & SWITCH_ON) != 0;

				from_str_i = tile->conn_point_names[from_connpt_o*2+1];
				to_str_i = tile->conn_point_names[to_connpt_o*2+1];

				from_str = strarray_lookup(&model->str, from_str_i);
				to_str = strarray_lookup(&model->str, to_str_i);
				if (!from_str || !to_str) {
					fprintf(stderr, "Internal error in %s:%i, cannot lookup from_i %i or to_i %i\n",
						__FILE__, __LINE__, from_str_i, to_str_i);
					continue;
				}
				if (enabled_only && !is_on)
					continue;
				if (!first_switch_printed) {
					first_switch_printed = 1;	
					fprintf(f, "\n");
				}
				fprintf(f, "sw y%02i x%02i %s %s %s\n",
					y, x, from_str, is_bidirectional
					? "<->" : "->", to_str);
			}
		}
	}
	return 0;
}

static enum fpgadev_type to_type(const char* s, int len)
{
	if (!str_cmp(s, len, "IOB", 3)) return DEV_IOB;
	if (!str_cmp(s, len, "LOGIC", 5)) return DEV_LOGIC;
	return DEV_NONE;
}

int read_floorplan(struct fpga_model* model, FILE* f)
{
	char line[1024];
	int beg, end;

	while (fgets(line, sizeof(line), f)) {
		next_word(line, 0, &beg, &end);
		if (end == beg) continue;

		if (!str_cmp(&line[beg], end-beg, "dev", 3)) {
			int y_beg, y_end, x_beg, x_end;
			int y_coord, x_coord;
			int type_beg, type_end, idx_beg, idx_end;
			enum fpgadev_type dev_type;
			int dev_idx, words_consumed;
			struct fpga_device* dev_ptr;
			int next_beg, next_end, second_beg, second_end;

			next_word(line, end, &y_beg, &y_end);
			next_word(line, y_end, &x_beg, &x_end);
			if (y_end < y_beg+2 || x_end < x_beg+2
			    || line[y_beg] != 'y' || line[x_beg] != 'x'
			    || !all_digits(&line[y_beg+1], y_end-y_beg-1)
			    || !all_digits(&line[x_beg+1], x_end-x_beg-1)) {
				fprintf(stderr, "error %i: %s", __LINE__, line);
				continue;
			}
			y_coord = to_i(&line[y_beg+1], y_end-y_beg-1);
			x_coord = to_i(&line[x_beg+1], x_end-x_beg-1);

			next_word(line, x_end, &type_beg, &type_end);
			next_word(line, type_end, &idx_beg, &idx_end);

			if (type_end == type_beg || idx_end == idx_beg
			    || !all_digits(&line[idx_beg], idx_end-idx_beg)) {
				fprintf(stderr, "error %i: %s", __LINE__, line);
				continue;
			}
			dev_type = to_type(&line[type_beg], type_end-type_beg);
			dev_idx = to_i(&line[idx_beg], idx_end-idx_beg);
			dev_ptr = fpga_dev(model, y_coord, x_coord, dev_type, dev_idx);
			if (!dev_ptr) {
				fprintf(stderr, "error %i: %s", __LINE__, line);
				continue;
			}

			next_end = idx_end;
			while (next_word(line, next_end, &next_beg, &next_end),
				next_end > next_beg) {
				next_word(line, next_end, &second_beg, &second_end);
				switch (dev_type) {
					case DEV_IOB:
						words_consumed = read_IOB_attr(
							model, dev_ptr,
							&line[next_beg],
							next_end-next_beg,
							&line[second_beg],
							second_end-second_beg);
						break;
					case DEV_LOGIC:
						words_consumed = read_LOGIC_attr(
							model, dev_ptr,
							&line[next_beg],
							next_end-next_beg,
							&line[second_beg],
							second_end-second_beg);
						break;
					default:
						fprintf(stderr, "error %i: %s",
							__LINE__, line);
						goto next_line;
				}
				if (!words_consumed)
					fprintf(stderr,
						"error %i w1 %.*s w2 %.*s: %s",
						__LINE__, next_end-next_beg,
						&line[next_beg],
						second_end-second_beg,
						&line[second_beg],
						line);
				else if (words_consumed == 2)
					next_end = second_end;
			}
next_line: ;
		}
	}
	return 0;
}

int write_floorplan(FILE* f, struct fpga_model* model, int flags)
{
	if (!(flags & FP_BITS_ONLY))
		printf_version(f);
	return 0;
}
