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

static const xmlChar* empty_svg = (const xmlChar*)
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<svg\n"
	"   xmlns=\"http://www.w3.org/2000/svg\"\n"
	"   version=\"2.0\"\n"
	"   xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
	"   viewBox=\"0 0 1000 1000\" width=\"1000\" height=\"1000\">\n"
	"   <rect width=\"100%\" height=\"100%\" style=\"fill:black;\"/>\n"
        "   <g id=\"root\" transform=\"translate(0,1000) scale(1,-1)\">\n"
        "   </g>\n"
	"</svg>\n";

int main(int argc, char** argv)
{
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

		for (i = 0; i < 10; i++) {
			new_node = xmlNewChild(xpathObj->nodesetval->nodeTab[0], 0 /* xmlNsPtr */, BAD_CAST "use", 0 /* content */);
			xmlSetProp(new_node, BAD_CAST "xlink:href", BAD_CAST "lib.svg#IOB");
			xmlSetProp(new_node, BAD_CAST "x", xmlXPathCastNumberToString(50+i*50));
			xmlSetProp(new_node, BAD_CAST "y", BAD_CAST "50");
			xmlSetProp(new_node, BAD_CAST "width", BAD_CAST "50");
			xmlSetProp(new_node, BAD_CAST "height", BAD_CAST "50");
		}
	}

	xmlDocDump(stdout, doc);

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
