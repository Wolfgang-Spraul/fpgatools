//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

struct fpga_model
{
	int tile_x_range, tile_y_range;
	struct fpga_tile* tiles;
};

enum fpga_tile_type
{
	NA,
	ROUTING,
	ROUTING_BRK,
	ROUTING_VIA,
	HCLK_ROUTING_XM,
	HCLK_ROUTING_XL,
	HCLK_LOGIC_XM,
	HCLK_LOGIC_XL,
	LOGIC_XM,
	LOGIC_XL,
	REGH_ROUTING_XM,
	REGH_ROUTING_XL,
	REGH_LOGIC_XM,
	REGH_LOGIC_XL,
	BRAM_ROUTING,
	BRAM_ROUTING_BRK,
	BRAM,
	HCLK_BRAM_ROUTING,
	HCLK_BRAM_ROUTING_VIA,
	HCLK_BRAM,
	REGH_BRAM_ROUTING,
	REGH_BRAM_ROUTING_VIA,
	REGH_BRAM_L,
	REGH_BRAM_R,
	MACC,
	HCLK_MACC_ROUTING,
	HCLK_MACC_ROUTING_VIA,
	HCLK_MACC,
	REGH_MACC_ROUTING,
	REGH_MACC_ROUTING_VIA,
	REGH_MACC_L,
	PLL_T,
	DCM_T,
	PLL_B,
	DCM_B,
	HCLK_REG_V,
	REG_V,
	REG_V_BRK,
	REG_V_TOP,
	REG_V_BOTTOM,
	REG_V_MIDBUF_TOP,
	REG_V_HCLKBUF_TOP,
	REG_V_HCLKBUF_BOTTOM,
	REG_V_MIDBUF_BOTTOM,
	REGC_ROUTING,
	REGC_LOGIC,
	REGC_CMT,
	CENTER, // unique center tile in the middle of the chip
};

const char* fpga_ttstr[] = // tile type strings
{
	[NA] = "NA",
	[ROUTING] = "ROUTING",
	[ROUTING_BRK] = "ROUTING_BRK",
	[ROUTING_VIA] = "ROUTING_VIA",
	[HCLK_ROUTING_XM] = "HCLK_ROUTING_XM",
	[HCLK_ROUTING_XL] = "HCLK_ROUTING_XL",
	[HCLK_LOGIC_XM] = "HCLK_LOGIC_XM",
	[HCLK_LOGIC_XL] = "HCLK_LOGIC_XL",
	[LOGIC_XM] = "LOGIC_XM",
	[LOGIC_XL] = "LOGIC_XL",
	[REGH_ROUTING_XM] = "REGH_ROUTING_XM",
	[REGH_ROUTING_XL] = "REGH_ROUTING_XL",
	[REGH_LOGIC_XM] = "REGH_LOGIC_XM",
	[REGH_LOGIC_XL] = "REGH_LOGIC_XL",
	[BRAM_ROUTING] = "BRAM_ROUTING",
	[BRAM_ROUTING_BRK] = "BRAM_ROUTING_BRK",
	[BRAM] = "BRAM",
	[HCLK_BRAM_ROUTING] = "HCLK_BRAM_ROUTING",
	[HCLK_BRAM_ROUTING_VIA] = "HCLK_BRAM_ROUTING_VIA",
	[HCLK_BRAM] = "HCLK_BRAM",
	[REGH_BRAM_ROUTING] = "REGH_BRAM_ROUTING",
	[REGH_BRAM_ROUTING_VIA] = "REGH_BRAM_ROUTING_VIA",
	[REGH_BRAM_L] = "REGH_BRAM_L",
	[REGH_BRAM_R] = "REGH_BRAM_R",
	[MACC] = "MACC",
	[HCLK_MACC_ROUTING] = "HCLK_MACC_ROUTING",
	[HCLK_MACC_ROUTING_VIA] = "HCLK_MACC_ROUTING_VIA",
	[HCLK_MACC] = "HCLK_MACC",
	[REGH_MACC_ROUTING] = "REGH_MACC_ROUTING",
	[REGH_MACC_ROUTING_VIA] = "REGH_MACC_ROUTING_VIA",
	[REGH_MACC_L] = "REGH_MACC_L",
	[PLL_T] = "PLL_T",
	[DCM_T] = "DCM_T",
	[PLL_B] = "PLL_B",
	[DCM_B] = "DCM_B",
	[HCLK_REG_V] = "HCLK_REG_V",
	[REG_V] = "REG_V",
	[REG_V_BRK] = "REG_V_BRK",
	[REG_V_TOP] = "REG_V_TOP",
	[REG_V_BOTTOM] = "REG_V_BOTTOM",
	[REG_V_MIDBUF_TOP] = "REG_V_MIDBUF_TOP",
	[REG_V_HCLKBUF_TOP] = "REG_V_HCLKBUF_TOP",
	[REG_V_HCLKBUF_BOTTOM] = "REG_V_HCLKBUF_BOTTOM",
	[REG_V_MIDBUF_BOTTOM] = "REG_V_MIDBUF_BOTTOM",
	[REGC_ROUTING] = "REGC_ROUTING",
	[REGC_LOGIC] = "REGC_LOGIC",
	[REGC_CMT] = "REGC_CMT",
	[CENTER] = "CENTER",
};

struct fpga_tile
{
	enum fpga_tile_type type;
};

// columns
//  'L' = X+L logic block
//  'M' = X+M logic block
//  'B' = block ram
//  'D' = dsp (macc)
//  'R' = registers and center IO/logic column

#define XC6SLX9_ROWS	4
#define XC6SLX9_COLUMNS	"MLBMLDMRMLMLBML"

struct fpga_model* build_model(int fpga_rows, const char* columns);
void printf_model(struct fpga_model* model);

int main(int argc, char** argv)
{
	struct fpga_model* model = 0;

	//
	// build memory model
	//

	model = build_model(XC6SLX9_ROWS, XC6SLX9_COLUMNS);
	if (!model) goto fail;

	//
	// write svg
	//

	printf_model(model);
	return EXIT_SUCCESS;
fail:
	return EXIT_FAILURE;
}

struct fpga_model* build_model(int fpga_rows, const char* columns)
{
	int tile_rows, tile_columns, i, j, k, l, row_top_y, center_row, left_side;
	struct fpga_model* model;

	tile_rows = 1 /* middle */ + (8+1+8)*fpga_rows + 2+2 /* two extra tiles at top and bottom */;
	tile_columns = 5 /* left */ + 5 /* right */;
	for (i = 0; columns[i] != 0; i++) {
		tile_columns += 2; // 2 for logic blocks L/M and minimum for others
		if (columns[i] == 'B' || columns[i] == 'D')
			tile_columns++; // 3 for bram or macc
		else if (columns[i] == 'R')
			tile_columns+=2; // 2+2 for middle IO+logic+PLL/DCM
	}
	model = calloc(1 /* nelem */, sizeof(struct fpga_model));
	if (!model) {
		fprintf(stderr, "%i: Out of memory.\n", __LINE__);
		return 0;
	}
	model->tile_x_range = tile_columns;
	model->tile_y_range = tile_rows;
	model->tiles = calloc(tile_columns * tile_rows, sizeof(struct fpga_tile));
	if (!model->tiles) {
		fprintf(stderr, "%i: Out of memory.\n", __LINE__);
		free(model);
		return 0;
	}
	for (i = 0; i < tile_rows * tile_columns; i++)
		model->tiles[i].type = NA;
	if (!(tile_rows % 2))
		fprintf(stderr, "Unexpected even number of tile rows (%i).\n", tile_rows);
	i = 5; // left IO columns
	center_row = 2 /* top IO files */ + (fpga_rows/2)*(8+1/*middle of row clock*/+8);
	left_side = 1; // turn off (=right side) when reaching the 'R' middle column
	for (j = 0; columns[j]; j++) {
		switch (columns[j]) {
			case 'L':
			case 'M':
				for (k = fpga_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (fpga_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(fpga_rows/2)) row_top_y++; // middle system tiles (center row)
					for (l = ((k == fpga_rows-1) ? 2 : 0); l < ((k > 0) ? 16 : 14); l++) {
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i].type = (l < 15) ? ROUTING : ROUTING_BRK;
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 1].type
							= (columns[j] == 'L') ? LOGIC_XL : LOGIC_XM;
					}
					if (columns[j] == 'L') {
						model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XL;
						model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XL;
					} else {
						model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XM;
						model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XM;
					}
				}
				if (columns[j] == 'L') {
					model->tiles[center_row*tile_columns + i].type = REGH_ROUTING_XL;
					model->tiles[center_row*tile_columns + i + 1].type = REGH_LOGIC_XL;
				} else {
					model->tiles[center_row*tile_columns + i].type = REGH_ROUTING_XM;
					model->tiles[center_row*tile_columns + i + 1].type = REGH_LOGIC_XM;
				}
				i += 2;
				break;
			case 'B':
				for (k = fpga_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (fpga_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(fpga_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i].type = (l < 15) ? BRAM_ROUTING : BRAM_ROUTING_BRK;
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 1].type = ROUTING_VIA;
						if (!(l%4))
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].type = BRAM;
					}
					model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_BRAM_ROUTING;
					model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_BRAM_ROUTING_VIA;
					model->tiles[(row_top_y+8)*tile_columns + i + 2].type = HCLK_BRAM;
				}
				model->tiles[center_row*tile_columns + i].type = REGH_BRAM_ROUTING;
				model->tiles[center_row*tile_columns + i + 1].type = REGH_BRAM_ROUTING_VIA;
				model->tiles[center_row*tile_columns + i + 2].type = left_side ? REGH_BRAM_L : REGH_BRAM_R;
				i += 3;
				break;
			case 'D':
				for (k = fpga_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (fpga_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(fpga_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i].type = (l < 15) ? ROUTING : ROUTING_BRK;
						model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 1].type = ROUTING_VIA;
						if (!(l%4))
							model->tiles[(row_top_y+3+(l<8?l:l+1))*tile_columns + i + 2].type = MACC;
					}
					model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_MACC_ROUTING;
					model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_MACC_ROUTING_VIA;
					model->tiles[(row_top_y+8)*tile_columns + i + 2].type = HCLK_MACC;
				}
				model->tiles[center_row*tile_columns + i].type = REGH_MACC_ROUTING;
				model->tiles[center_row*tile_columns + i + 1].type = REGH_MACC_ROUTING_VIA;
				model->tiles[center_row*tile_columns + i + 2].type = REGH_MACC_L;
				i += 3;
				break;
			case 'R':
				left_side = 0;
				for (k = fpga_rows-1; k >= 0; k--) {
					row_top_y = 2 /* top IO tiles */ + (fpga_rows-1-k)*(8+1/*middle of row clock*/+8);
					if (k<(fpga_rows/2)) row_top_y++; // middle system tiles
					for (l = 0; l < 16; l++) {
						if (l == 7) {
							if (k%2) // odd
								model->tiles[(row_top_y+l)*tile_columns + i + 2].type = (k<(fpga_rows/2)) ? PLL_B : PLL_T;
							else // even
								model->tiles[(row_top_y+l)*tile_columns + i + 2].type = (k<(fpga_rows/2)) ? DCM_B : DCM_T;
						}
						// four midbuf tiles, in the middle of the top and bottom halves
						if (l == 15) {
							if (k == fpga_rows*3/4)
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REG_V_MIDBUF_TOP;
							else if (k == fpga_rows/4)
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REG_V_HCLKBUF_BOTTOM;
							else
								model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = REG_V_BRK;
						} else if (l == 0 && k == fpga_rows*3/4-1) {
							model->tiles[(row_top_y+l)*tile_columns + i + 3].type = REG_V_HCLKBUF_TOP;
						} else if (l == 0 && k == fpga_rows/4-1) {
							model->tiles[(row_top_y+l)*tile_columns + i + 3].type = REG_V_MIDBUF_BOTTOM;
						} else if (l == 8) {
							model->tiles[(row_top_y+l+1)*tile_columns + i + 3].type = (k<fpga_rows/2) ? REG_V_BOTTOM : REG_V_TOP;
						} else
							model->tiles[(row_top_y+(l<8?l:l+1))*tile_columns + i + 3].type = REG_V;
					}
					model->tiles[(row_top_y+8)*tile_columns + i].type = HCLK_ROUTING_XL;
					model->tiles[(row_top_y+8)*tile_columns + i + 1].type = HCLK_LOGIC_XL;
					model->tiles[(row_top_y+8)*tile_columns + i + 3].type = HCLK_REG_V;
				}
				model->tiles[center_row*tile_columns + i].type = REGC_ROUTING;
				model->tiles[center_row*tile_columns + i + 1].type = REGC_LOGIC;
				model->tiles[center_row*tile_columns + i + 2].type = REGC_CMT;
				model->tiles[center_row*tile_columns + i + 3].type = CENTER;
				i += 4;
				break;
			default:
				fprintf(stderr, "Unexpected column identifier '%c'\n", columns[i]);
					break;
		}
	}
	return model;
}

void printf_model(struct fpga_model* model)
{
	static const xmlChar* empty_svg = (const xmlChar*)
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<svg version=\"2.0\"\n"
		"   xmlns=\"http://www.w3.org/2000/svg\"\n"
		"   xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
		"   xmlns:fpga=\"http://qi-hw.com/fpga\"\n"
		"   id=\"root\">\n"
		"<style type=\"text/css\"><![CDATA[text{font-size:6pt;text-anchor:end;}]]></style>\n"
		"</svg>\n";

	xmlDocPtr doc = 0;
	xmlXPathContextPtr xpathCtx = 0;
	xmlXPathObjectPtr xpathObj = 0;
	xmlNodePtr new_node;
	char str[128];
	int i, j;

// can't get indent formatting to work - use 'xmllint --pretty 1 -'
// on the output for now

	xmlInitParser();
	doc = xmlParseDoc(empty_svg);
	if (!doc) {
		fprintf(stderr, "Internal error %i.\n", __LINE__);
		goto fail;
	}
	xpathCtx = xmlXPathNewContext(doc);
	if (!xpathCtx) {
		fprintf(stderr, "Cannot create XPath context.\n");
		goto fail;
	}
	xmlXPathRegisterNs(xpathCtx, BAD_CAST "svg", BAD_CAST "http://www.w3.org/2000/svg");
	xpathObj = xmlXPathEvalExpression(BAD_CAST "//svg:*[@id='root']", xpathCtx);
	if (!xpathObj) {
		fprintf(stderr, "Cannot evaluate root expression.\n");
		goto fail;
	}
	if (!xpathObj->nodesetval) {
		fprintf(stderr, "Cannot find root node.\n");
		goto fail;
	}
	if (xpathObj->nodesetval->nodeNr != 1) {
		fprintf(stderr, "Found %i root nodes.\n", xpathObj->nodesetval->nodeNr);
		goto fail;
	}

	for (i = 0; i < model->tile_y_range; i++) {
		for (j = 0; j < model->tile_x_range; j++) {
			strcpy(str, fpga_ttstr[model->tiles[i*model->tile_x_range+j].type]);
			new_node = xmlNewChild(xpathObj->nodesetval->nodeTab[0], 0 /* xmlNsPtr */, BAD_CAST "text", BAD_CAST str);
			xmlSetProp(new_node, BAD_CAST "x", xmlXPathCastNumberToString(130 + j*130));
			xmlSetProp(new_node, BAD_CAST "y", xmlXPathCastNumberToString(40 + i*20));
			xmlSetProp(new_node, BAD_CAST "fpga:tile_y", BAD_CAST xmlXPathCastNumberToString(i));
			xmlSetProp(new_node, BAD_CAST "fpga:tile_x", BAD_CAST xmlXPathCastNumberToString(j));
		}
	}
	xmlSetProp(xpathObj->nodesetval->nodeTab[0], BAD_CAST "width", BAD_CAST xmlXPathCastNumberToString(model->tile_x_range * 130 + 65));
	xmlSetProp(xpathObj->nodesetval->nodeTab[0], BAD_CAST "height", BAD_CAST xmlXPathCastNumberToString(model->tile_y_range * 20 + 60));

	xmlDocFormatDump(stdout, doc, 1 /* format */);
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx); 
	xmlFreeDoc(doc); 
 	xmlCleanupParser();
	return;

fail:
	if (xpathObj) xmlXPathFreeObject(xpathObj);
	if (xpathCtx) xmlXPathFreeContext(xpathCtx); 
	if (doc) xmlFreeDoc(doc); 
 	xmlCleanupParser();
}
