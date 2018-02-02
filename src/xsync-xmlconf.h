/***********************************************************************
* Copyright (c) 2018 pepstack, pepstack.com
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*   claim that you wrote the original software. If you use this software
*   in a product, an acknowledgment in the product documentation would be
*   appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
*   misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
***********************************************************************/

/**
 * xsync-xsync-xmlconf.h
 *
 * author: master@pepstack.com
 *
 * refer:
 *   - https://www.systutorials.com/docs/linux/man/3-mxml/
 *
 * create: 2016-07-06
 * update: 2018-02-02
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
