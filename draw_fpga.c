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
	NULL_T,
	LOGIC_XM,
	LOGIC_XL,
	BRAM,
	MACC,
};

struct fpga_tile
{
	enum fpga_tile_type type;
};

struct fpga_model* build_model(int rows, const char* columns)
{
	int tile_rows, tile_columns, i;

	tile_rows = 1 /* middle */ + (8+1+8)*rows
		+ 2+2 /* two extra tiles at top and bottom */;
	tile_columns = 5 /* left */ + 2 /* middle regs */ + 5 /* right */;
	for (i = 0; columns[i] != 0; i++) {
		tile_columns += 2; // 2 for logic blocks L/M or middle regs
		if (columns[i] == 'B' || columns[i] == 'D')
			tile_columns++; // 3 for bram or macc
	}
	return 0;
}

// columns
//  'L' = X+L logic block
//  'M' = X+M logic block
//  'B' = block ram
//  'D' = dsp (macc)
//  'R' = registers (middle)

#define XC6SLX9_ROWS	4
#define XC6SLX9_COLUMNS	"MLBMLDMLRMLMLBML"

static const xmlChar* empty_svg = (const xmlChar*)
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<svg\n"
	"   version=\"2.0\"\n"
	"   xmlns=\"http://www.w3.org/2000/svg\"\n"
	"   xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
	"   width=\"1000\" height=\"1000\"\n"
	"   id=\"root\">\n"
	"</svg>\n";

int main(int argc, char** argv)
{
	struct fpga_model* model = 0;

	//
	// build memory model
	//

// build_model(XC6SLX9_ROWS, XC6SLX9_COLUMNS);

	//
	// write svg
	//

// can't get indent formatting to work - use 'xmllint --pretty 1 -'
// on the output for now

	xmlDocPtr doc = 0;
	xmlXPathContextPtr xpathCtx = 0;
	xmlXPathObjectPtr xpathObj = 0;

	xmlInitParser();
	doc = xmlParseDoc(empty_svg);
	if (!doc) {
		fprintf(stderr, "Internal error %i.\n", __LINE__);
		goto fail_xml;
	}
	xpathCtx = xmlXPathNewContext(doc);
	if (!xpathCtx) {
		fprintf(stderr, "Cannot create XPath context.\n");
		goto fail_xml;
	}
	xmlXPathRegisterNs(xpathCtx, BAD_CAST "svg", BAD_CAST "http://www.w3.org/2000/svg");
	xpathObj = xmlXPathEvalExpression(BAD_CAST "//svg:*[@id='root']", xpathCtx);
	if (!xpathObj) {
		fprintf(stderr, "Cannot evaluate root expression.\n");
		goto fail_xml;
	}
	if (!xpathObj->nodesetval) {
		fprintf(stderr, "Cannot find root node.\n");
		goto fail_xml;
	}
	if (xpathObj->nodesetval->nodeNr != 1) {
		fprintf(stderr, "Found %i root nodes.\n", xpathObj->nodesetval->nodeNr);
		goto fail_xml;
	}

	{
		xmlNodePtr new_node;
		int i;

//<text x="6900" y="1058">NULL</text>
		for (i = 0; i < 10; i++) {
			new_node = xmlNewChild(xpathObj->nodesetval->nodeTab[0], 0 /* xmlNsPtr */, BAD_CAST "use", 0 /* content */);
			xmlSetProp(new_node, BAD_CAST "xlink:href", BAD_CAST "lib.svg#IOB");
			xmlSetProp(new_node, BAD_CAST "x", xmlXPathCastNumberToString(50+i*50));
			xmlSetProp(new_node, BAD_CAST "y", BAD_CAST "50");
			xmlSetProp(new_node, BAD_CAST "width", BAD_CAST "50");
			xmlSetProp(new_node, BAD_CAST "height", BAD_CAST "50");
		}
	}

	xmlDocFormatDump(stdout, doc, 1 /* format */);

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx); 
	xmlFreeDoc(doc); 
 	xmlCleanupParser();
	return EXIT_SUCCESS;

fail_xml:
	if (xpathObj) xmlXPathFreeObject(xpathObj);
	if (xpathCtx) xmlXPathFreeContext(xpathCtx); 
	if (doc) xmlFreeDoc(doc); 
 	xmlCleanupParser();
	return EXIT_FAILURE;
}
