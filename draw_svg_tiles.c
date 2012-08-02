//
// Author: Wolfgang Spraul
//
// This is free and unencumbered software released into the public domain.
// For details see the UNLICENSE file at the root of the source tree.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/stat.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "model.h"

#define VERT_TILE_SPACING	 45
#define HORIZ_TILE_SPACING	160

int main(int argc, char** argv)
{
	static const xmlChar* empty_svg = (const xmlChar*)
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<svg version=\"2.0\"\n"
		"   xmlns=\"http://www.w3.org/2000/svg\"\n"
		"   xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
		"   xmlns:fpga=\"http://qi-hw.com/fpga\"\n"
		"   id=\"root\">\n"
		"<style type=\"text/css\"><![CDATA[text{font-size:8pt;font-family:sans-serif;text-anchor:end;}]]></style>\n"
		"</svg>\n";

	xmlDocPtr doc = 0;
	xmlXPathContextPtr xpathCtx = 0;
	xmlXPathObjectPtr xpathObj = 0;
	xmlNodePtr new_node;
	struct fpga_model model = {0};
	char str[128];
	int i, j;

// can't get indent formatting to work - use 'xmllint --pretty 1 -'
// on the output for now

	xmlInitParser();
	if (fpga_build_model(&model, XC6SLX9_ROWS, XC6SLX9_COLUMNS,
			XC6SLX9_LEFT_WIRING, XC6SLX9_RIGHT_WIRING))
		goto fail;

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

	for (i = 0; i < model.y_height; i++) {
		for (j = 0; j < model.x_width; j++) {
			sprintf(str, "y%i x%i:", i, j);
			new_node = xmlNewChild(xpathObj->nodesetval->nodeTab[0],
			   0 /* xmlNsPtr */, BAD_CAST "text", BAD_CAST str);
			xmlSetProp(new_node, BAD_CAST "x",
				xmlXPathCastNumberToString(HORIZ_TILE_SPACING + j*HORIZ_TILE_SPACING));
			xmlSetProp(new_node, BAD_CAST "y",
				xmlXPathCastNumberToString(20
			  + VERT_TILE_SPACING + i*VERT_TILE_SPACING));

			strcpy(str, fpga_tiletype_str(
				model.tiles[i*model.x_width+j].type));
			new_node = xmlNewChild(xpathObj->nodesetval->nodeTab[0],
			   0 /* xmlNsPtr */, BAD_CAST "text", BAD_CAST str);
			xmlSetProp(new_node, BAD_CAST "x",
				xmlXPathCastNumberToString(HORIZ_TILE_SPACING + j*HORIZ_TILE_SPACING));
			xmlSetProp(new_node, BAD_CAST "y",
				xmlXPathCastNumberToString(20 + VERT_TILE_SPACING + i*VERT_TILE_SPACING + 14));
			xmlSetProp(new_node, BAD_CAST "fpga:tile_y", BAD_CAST xmlXPathCastNumberToString(i));
			xmlSetProp(new_node, BAD_CAST "fpga:tile_x", BAD_CAST xmlXPathCastNumberToString(j));
		}
	}
	xmlSetProp(xpathObj->nodesetval->nodeTab[0], BAD_CAST "width",
		BAD_CAST xmlXPathCastNumberToString(model.x_width * HORIZ_TILE_SPACING + HORIZ_TILE_SPACING/2));
	xmlSetProp(xpathObj->nodesetval->nodeTab[0], BAD_CAST "height",
		BAD_CAST xmlXPathCastNumberToString(20 + VERT_TILE_SPACING
		+ model.y_height * VERT_TILE_SPACING + 20));

	xmlDocFormatDump(stdout, doc, 1 /* format */);
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx); 
	xmlFreeDoc(doc); 
 	xmlCleanupParser();
	fpga_free_model(&model);
	return EXIT_SUCCESS;

fail:
	if (xpathObj) xmlXPathFreeObject(xpathObj);
	if (xpathCtx) xmlXPathFreeContext(xpathCtx); 
	if (doc) xmlFreeDoc(doc); 
 	xmlCleanupParser();
	fpga_free_model(&model);
	return EXIT_FAILURE;
}
