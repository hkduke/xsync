/***********************************************************************
* COPYRIGHT (C) 2018 PEPSTACK, PEPSTACK.COM
*
* THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTY. IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES
* ARISING FROM THE USE OF THIS SOFTWARE.
*
* PERMISSION IS GRANTED TO ANYONE TO USE THIS SOFTWARE FOR ANY PURPOSE,
* INCLUDING COMMERCIAL APPLICATIONS, AND TO ALTER IT AND REDISTRIBUTE IT
* FREELY, SUBJECT TO THE FOLLOWING RESTRICTIONS:
*
*  THE ORIGIN OF THIS SOFTWARE MUST NOT BE MISREPRESENTED; YOU MUST NOT
*  CLAIM THAT YOU WROTE THE ORIGINAL SOFTWARE. IF YOU USE THIS SOFTWARE
*  IN A PRODUCT, AN ACKNOWLEDGMENT IN THE PRODUCT DOCUMENTATION WOULD
*  BE APPRECIATED BUT IS NOT REQUIRED.
*
*  ALTERED SOURCE VERSIONS MUST BE PLAINLY MARKED AS SUCH, AND MUST NOT
*  BE MISREPRESENTED AS BEING THE ORIGINAL SOFTWARE.
*
*  THIS NOTICE MAY NOT BE REMOVED OR ALTERED FROM ANY SOURCE DISTRIBUTION.
***********************************************************************/

/**
 * @file: xsync-xmlconf.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 2018-05-20 22:04:08
 *
 * @create: 2018-01-29
 *
 * @update: 2018-05-20 22:04:08
 */

#ifndef XSYNC_XML_CONF_H_INCLUDED
#define XSYNC_XML_CONF_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <mxml.h>


/**
 * xmlconf_find_element_node(root, "xs:application", node);
 */
#define xmlconf_find_element_node(root_node, subnode_name, subnode_var) \
    do { \
        subnode_var = mxmlFindElement(root_node, root_node, subnode_name, 0, 0, MXML_DESCEND); \
        if (! subnode_var) { \
            LOGGER_ERROR("mxml element '%s' not found", subnode_name); \
            goto error_exit; \
        } \
    } while(0)


/**
 * xmlconf_element_attr_check(node, "version", "0.0.1", attr);
 */
#define xmlconf_element_attr_check(element_node, attr_name, attr_value_expected, attr_value_var) \
    do { \
        attr_value_var = mxmlElementGetAttr(element_node, attr_name); \
        if (! attr_value_var) { \
            LOGGER_ERROR("mxml attr '%s' not found", attr_name); \
            goto error_exit; \
        } \
        if (strcmp(attr_value_var, attr_value_expected)) { \
            LOGGER_ERROR("mxml attr '%s'='%s' error. '%s' (required)", attr_name, attr_value_var, attr_value_expected); \
            goto error_exit; \
        } \
    } while(0)



#define xmlconf_element_attr_read(element_node, attr_name, attr_value_default, attr_value_var) \
    do { \
        const char * attr_value = 0; \
        attr_value_var = attr_value_default; \
        attr_value = mxmlElementGetAttr(element_node, attr_name); \
        if (attr_value) { \
            attr_value_var = attr_value; \
        } \
        LOGGER_TRACE("mxml read attr '%s'='%s'", attr_name, attr_value_var ? attr_value_var : "(null)"); \
    } while(0)



typedef int (*xmlconf_list_node_cb_t) (mxml_node_t *node, void *data);

__attribute__((used))
static int xmlconf_list_mxml_nodes (mxml_node_t *parent, const char * child_name, xmlconf_list_node_cb_t list_node_cb, void *data)
{
    mxml_node_t *child;

    for (child = mxmlFindElement(parent, parent, child_name, 0, 0, MXML_DESCEND);
        child != 0;
        child = mxmlFindElement(child, parent, child_name, 0, 0, MXML_DESCEND)) {

        if (! list_node_cb(child, data)) {
            return 0;
        }
    }

    return 1;
}


#if defined(__cplusplus)
}
#endif

#endif /* XSYNC_XML_CONF_H_INCLUDED */
