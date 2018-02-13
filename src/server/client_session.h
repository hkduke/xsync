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
 *                     XS_server_t
 *
 *                      +---------------+
 *                      |     hlist     |
 *                      +---------------+
 *                      | session_id 01 | ->  client_sessionA::token1
 *                      +---------------+
 *                      | session_id 02 | ->  client_sessionB::token2
 *                      +---------------+
 *                      |     ...       |
 *                      +---------------+
 *
 */

#ifndef CLIENT_SESSION_H_INCLUDED
#define CLIENT_SESSION_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "file_entry.h"

/**
 * xs_client_session_t
 *
 *   为每个客户端连接建立一个会话, 会话过期后自动删除
 */
typedef struct xs_client_session_t
{
    EXTENDS_REFOBJECT_TYPE();

    /**
     * 是否处于使用中
     *
     *   1: in use
     *   0: not in use
     */
    int volatile in_use;

    /**
     * clientid
     *
     *   客户ID用于唯一标识一个xsync-client。任何2个xsync-client不能使用同一个clientid。
     */
    char clientid[XSYNC_CLIENTID_MAXLEN + 1];

    /**
     * 4 bytes: token used by server to authenticate client
     *
     *   用户连接请求建立之后，服务端给客户端发放的令牌，用于验证客户随后的访问
     */
    char token[4];

    /**
     * 客户端的ip地址端口等信息
     */

    /**
     * hlist node in client_hlist of XS_server
     */
    struct hlist_node i_hash;

    /**
     * hlist for file_entry: key is entryid
     */
    struct hlist_head entry_hlist[XSYNC_FILE_ENTRY_HASHMAX + 1];

    /**
     * 客户端的文件在服务器上保存的目录前缀, 所有该客户端上传的文件保存到这个路径下
     */
    char path_prefix[XSYNC_PATHFILE_MAXLEN + 1];

} * XS_client_session, xs_client_session_t;


__no_warning_unused(static)
inline void session_clear_file_entry (XS_client_session client)
{
    int hash;
    struct hlist_node *hp, *hn;

    LOGGER_TRACE("hlist clear");

    for (hash = 0; hash <= XSYNC_FILE_ENTRY_HASHMAX; hash++) {
        hlist_for_each_safe(hp, hn, &client->entry_hlist[hash]) {
            struct xs_file_entry_t *entry = hlist_entry(hp, struct xs_file_entry_t, i_hash);

            hlist_del(&entry->i_hash);

            XS_file_entry_release(&entry);
        }
    }
}


__no_warning_unused(static)
inline void xs_client_session_delete (void *pv)
{
    XS_client_session client = (XS_client_session) pv;

    LOGGER_TRACE0();

    session_clear_file_entry(client);

    free(pv);
}


extern XS_RESULT XS_client_session_create (const char *clientid, XS_client_session *outSession);

extern XS_VOID XS_client_session_release (XS_client_session * inSession);

extern XS_BOOL XS_client_session_not_in_use (XS_client_session session);


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_SESSION_H_INCLUDED */
