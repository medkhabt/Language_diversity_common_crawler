/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _errcode_h_
#define _errcode_h_

#include "doc.h"

#define ERR_NOERROR             0

#define ERR_STORE_DOC           1       /*** error string document to local file ***/
#define ERR_FILE_OPEN           2       /*** unable to open local file ***/
#define ERR_DIR_URL             3       /*** FILE URL points to directory ***/
#define ERR_UNKNOWN             5
#define ERR_LOCKED              6       /*** document is locked by another pavuk instance ***/
#define ERR_READ                7       /*** error while reading from socket/file ***/
#define ERR_BIGGER              8       /*** file is bigger than max. allowed ***/
#define ERR_NOMIMET             9       /*** file MIME type is not allowed ***/
#define ERR_PROXY_CONNECT       10      /*** error while connecting to proxy server ***/
#define ERR_BREAK               11      /*** user break ***/
#define ERR_OUTTIME             12      /*** file modification time doesn't fit in allowed time interval ***/
#define ERR_SCRIPT_DISABLED     13      /*** disabled by uexit script ***/
#define ERR_SMALLER             14      /*** file is smaller than minimal allowed ***/
#define ERR_ZERO_SIZE           15      /*** if file have zero size ***/
#define ERR_PROCESSED           16      /*** document was allready processed ***/
#define ERR_UDISABLED           17      /*** user disables manualy the processing of this URL ***/
#define ERR_RDISABLED           18      /*** this document is disabled because of rules ***/
#define ERR_LOW_TRANSFER_RATE   19      /*** transfer rate lower than minimal requested ***/
#define ERR_QUOTA_FILE          20      /*** reached maximal allowed size of file ***/
#define ERR_QUOTA_TRANS         21      /*** quota for transfer exceeded ***/
#define ERR_QUOTA_FS            22      /*** low space on filesystem ***/
#define ERR_QUOTA_TIME          23      /*** maximal allowed running time exceeded ***/
#define ERR_BAD_CONTENT         24      /*** document contains at least one 'bad_content' string ***/
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
#define ERR_NOTHING_TO_SEND     25      /*** hammer only: replay record has no content ***/
#define ERR_UNEXPECTED_RESPONSE 26      /*** hammer mode only: web server returned an unexpected response ***/
#endif

#define ERR_FTP_UNKNOWN         1000    /*** unknown error ***/
#define ERR_FTP_NOREGET         1001    /*** server doesn't support reget ***/
#define ERR_FTP_BDIR            1002    /*** directory list error ***/
#define ERR_FTP_CONNECT         1003    /*** error connecting ***/
#define ERR_FTP_BUSER           1004    /*** USER error ***/
#define ERR_FTP_BPASS           1005    /*** PASS error ***/
#define ERR_FTP_DATACON         1006    /*** error seting up data connection ***/
#define ERR_FTP_GET             1007    /*** error when trying to transfer file or dir ***/
#define ERR_FTP_NODIR           1008    /*** directory doesn't exist or is not accesible ***/
#define ERR_FTP_TRUNC           1009    /*** truncated file ?? ***/
#define ERR_FTP_ACTUAL          1010    /*** ftp file is actual (no transfer) ***/
#define ERR_FTP_NOTRANSFER      1011    /*** nothing to transfer ***/
#define ERR_FTP_NOMDTM          1012    /*** server doesn't suport MDTM command ***/
#define ERR_FTP_DIRNO           1013    /*** directory , but not allowed ***/
#define ERR_FTP_BPROXYUSER      1014    /*** proxy USER error ***/
#define ERR_FTP_BPROXYPASS      1015    /*** proxy PASS error ***/
#define ERR_FTP_LOGIN_HANDSHAKE 1016    /*** failed custom login handshake ***/

#define ERR_HTTP_UNKNOWN        2000    /*** unknown error ***/
#define ERR_HTTP_CONNECT        2001    /*** connection error ***/
#define ERR_HTTP_NOREGET        2002    /*** reget not supported ***/
#define ERR_HTTP_SNDREQ         2003    /*** error sending request ***/
#define ERR_HTTP_REDIR          2004    /*** redirect ***/
#define ERR_HTTP_TRUNC          2005    /*** truncated ***/
#define ERR_HTTP_CYCLIC         2006    /*** cyclic redirection ***/
#define ERR_HTTP_UNSUPREDIR     2007    /*** unsupported URL in redirection ***/
#define ERR_HTTP_SNDREQDATA     2008    /*** error sending request data ***/
#define ERR_HTTP_PROXY_CONN     2009    /*** error connecting to HTTP proxy ***/
#define ERR_HTTP_BADREDIRECT    2010    /*** after redirect Loacation: is missing ***/
#define ERR_HTTP_AUTH_NTLM      2011    /*** error doing NTLM authorization ***/
#define ERR_HTTP_AUTH_DIGEST    2012    /*** error doing HTTP Digest authorization ***/
#define ERR_HTTP_PROAUTH_NTLM   2013    /*** error doing NTLM proxy authorization ***/
#define ERR_HTTP_PROAUTH_DIGEST 2014    /*** error doing HTTP proxy Digest authorization ***/
#define ERR_HTTP_CLOSURE        2015    /*** persistent connection was closed ***/
#define ERR_HTTP_RCVRESP        2016    /*** error reading response ***/
#define ERR_HTTP_FAILREGET      2017    /*** bad answer on reget request ***/
/* [i_a] completed codes based on RFC2616 */
#define HTTP_BASE_ERRVAL        2000    /*** makes errorcode above to be HTTP response code + BASE_ERRVAL ***/

#define ERR_HTTP_CONTINUE       (HTTP_BASE_ERRVAL + HTTP_RSP_CONTINUE)
#define ERR_HTTP_SW_PCOLS       (HTTP_BASE_ERRVAL + HTTP_RSP_SW_PCOLS)
#define ERR_HTTP_OK             (HTTP_BASE_ERRVAL + HTTP_RSP_OK)
#define ERR_HTTP_CREATED        (HTTP_BASE_ERRVAL + HTTP_RSP_CREATED)
#define ERR_HTTP_ACCEPTED       (HTTP_BASE_ERRVAL + HTTP_RSP_ACCEPTED)
#define ERR_HTTP_NONAUTH_INFO   (HTTP_BASE_ERRVAL + HTTP_RSP_NONAUTH_INFO)
#define ERR_HTTP_NO_CONTENT     (HTTP_BASE_ERRVAL + HTTP_RSP_NO_CONTENT)
#define ERR_HTTP_RST_CONTENT    (HTTP_BASE_ERRVAL + HTTP_RSP_RST_CONTENT)
#define ERR_HTTP_PART_CONTENT   (HTTP_BASE_ERRVAL + HTTP_RSP_PART_CONTENT)
#define ERR_HTTP_MULTICHOICE    (HTTP_BASE_ERRVAL + HTTP_RSP_MULTICHOICE)
#define ERR_HTTP_MV_PERMANENT   (HTTP_BASE_ERRVAL + HTTP_RSP_MV_PERMANENT)
#define ERR_HTTP_FOUND          (HTTP_BASE_ERRVAL + HTTP_RSP_FOUND)
#define ERR_HTTP_SEE_OTHER      (HTTP_BASE_ERRVAL + HTTP_RSP_SEE_OTHER)
#define ERR_HTTP_ACTUAL         (HTTP_BASE_ERRVAL + HTTP_RSP_ACTUAL)
#define ERR_HTTP_USE_PROXY      (HTTP_BASE_ERRVAL + HTTP_RSP_USE_PROXY)
#define ERR_HTTP_306            (HTTP_BASE_ERRVAL + HTTP_RSP_306)
#define ERR_HTTP_REDIR2         (HTTP_BASE_ERRVAL + HTTP_RSP_REDIR2)
#define ERR_HTTP_BADRQ          (HTTP_BASE_ERRVAL + HTTP_RSP_BADRQ)
#define ERR_HTTP_AUTH           (HTTP_BASE_ERRVAL + HTTP_RSP_AUTH)
#define ERR_HTTP_PAY            (HTTP_BASE_ERRVAL + HTTP_RSP_PAY)
#define ERR_HTTP_FORB           (HTTP_BASE_ERRVAL + HTTP_RSP_FORB)
#define ERR_HTTP_NFOUND         (HTTP_BASE_ERRVAL + HTTP_RSP_NFOUND)
#define ERR_HTTP_NALLOW         (HTTP_BASE_ERRVAL + HTTP_RSP_NALLOW)
#define ERR_HTTP_NACCEPT        (HTTP_BASE_ERRVAL + HTTP_RSP_NACCEPT)
#define ERR_HTTP_PROXY_AUTH     (HTTP_BASE_ERRVAL + HTTP_RSP_PROXY_AUTH)
#define ERR_HTTP_TIMEOUT        (HTTP_BASE_ERRVAL + HTTP_RSP_TIMEOUT)
#define ERR_HTTP_CONFLICT       (HTTP_BASE_ERRVAL + HTTP_RSP_CONFLICT)
#define ERR_HTTP_GONE           (HTTP_BASE_ERRVAL + HTTP_RSP_GONE)
#define ERR_HTTP_MISSLEN        (HTTP_BASE_ERRVAL + HTTP_RSP_MISSLEN)
#define ERR_HTTP_PREC_FAIL      (HTTP_BASE_ERRVAL + HTTP_RSP_PREC_FAIL)
#define ERR_HTTP_TOO_LARGE      (HTTP_BASE_ERRVAL + HTTP_RSP_TOO_LARGE)
#define ERR_HTTP_LONG_URL       (HTTP_BASE_ERRVAL + HTTP_RSP_LONG_URL)
#define ERR_HTTP_UNSUP_MEDIA    (HTTP_BASE_ERRVAL + HTTP_RSP_UNSUP_MEDIA)
#define ERR_HTTP_BAD_RANGE      (HTTP_BASE_ERRVAL + HTTP_RSP_BAD_RANGE)
#define ERR_HTTP_EXPECT_FAIL    (HTTP_BASE_ERRVAL + HTTP_RSP_EXPECT_FAIL)
#define ERR_HTTP_SERV           (HTTP_BASE_ERRVAL + HTTP_RSP_SERV)
#define ERR_HTTP_NOT_IMPL       (HTTP_BASE_ERRVAL + HTTP_RSP_NOT_IMPL)
#define ERR_HTTP_BAD_GW         (HTTP_BASE_ERRVAL + HTTP_RSP_BAD_GW)
#define ERR_HTTP_SERV_UNAVAIL   (HTTP_BASE_ERRVAL + HTTP_RSP_SERV_UNAVAIL)
#define ERR_HTTP_GW_TIMEOUT     (HTTP_BASE_ERRVAL + HTTP_RSP_GW_TIMEOUT)
#define ERR_HTTP_VER_UNSUP      (HTTP_BASE_ERRVAL + HTTP_RSP_VER_UNSUP)



#define ERR_GOPHER_UNKNOWN      3000    /*** unknown error ***/
#define ERR_GOPHER_CONNECT      3001    /*** error connecting ***/

#define ERR_HTTPS_CONNECT       4001    /*** error connecting to HTTPS server ***/

#define ERR_FTPS_CONNECT        5001
#define ERR_FTPS_UNSUPPORTED    5002
#define ERR_FTPS_DATASSLCONNECT 5003

extern void report_error(doc *, char *);
extern char *doc_strerror(DBGdecl(doc * docp));
#define doc_strerror(d)         doc_strerror(DBGvars(d))
extern char *nw_strerror(DBGdecl(doc * docp), int errcode);
#define nw_strerror(d, e)         nw_strerror(DBGvars(d), e)
extern const char *errcodetype(int ecode);


/*
   HTTP response codes according to RFC2616
 */
#define HTTP_RSP_CONTINUE       100    /*** RFC2616: Section 10.1.1: Continue ***/
#define HTTP_RSP_SW_PCOLS       101    /*** RFC2616: Section 10.1.2: Switching Protocols ***/
#define HTTP_RSP_OK             200    /*** RFC2616: Section 10.2.1: OK ***/
#define HTTP_RSP_CREATED        201    /*** RFC2616: Section 10.2.2: Created ***/
#define HTTP_RSP_ACCEPTED       202    /*** RFC2616: Section 10.2.3: Accepted ***/
#define HTTP_RSP_NONAUTH_INFO   203    /*** RFC2616: Section 10.2.4: Non-Authoritative Information ***/
#define HTTP_RSP_NO_CONTENT     204    /*** RFC2616: Section 10.2.5: No Content ***/
#define HTTP_RSP_RST_CONTENT    205    /*** RFC2616: Section 10.2.6: Reset Content ***/
#define HTTP_RSP_PART_CONTENT   206    /*** RFC2616: Section 10.2.7: Partial Content ***/
#define HTTP_RSP_MULTICHOICE    300    /*** RFC2616: Section 10.3.1: Multiple Choices ***/
#define HTTP_RSP_MV_PERMANENT   301    /*** RFC2616: Section 10.3.2: Moved Permanently ***/
#define HTTP_RSP_FOUND          302    /*** RFC2616: Section 10.3.3: Found ***/
#define HTTP_RSP_SEE_OTHER      303    /*** RFC2616: Section 10.3.4: See Other ***/
#define HTTP_RSP_ACTUAL         304    /*** RFC2616: Section 10.3.5: Not Modified ***/
#define HTTP_RSP_USE_PROXY      305    /*** RFC2616: Section 10.3.6: Use Proxy ***/
#define HTTP_RSP_306            306    /*** 306 Unsupported by HTTP/1.1; obsolete ***/
#define HTTP_RSP_REDIR2         307    /*** RFC2616: Section 10.3.8: Temporary Redirect ***/
#define HTTP_RSP_BADRQ          400    /*** RFC2616: Section 10.4.1: Bad Request ***/
#define HTTP_RSP_AUTH           401    /*** RFC2616: Section 10.4.2: Unauthorized ***/
#define HTTP_RSP_PAY            402    /*** RFC2616: Section 10.4.3: Payment Required ***/
#define HTTP_RSP_FORB           403    /*** RFC2616: Section 10.4.4: Forbidden ***/
#define HTTP_RSP_NFOUND         404    /*** RFC2616: Section 10.4.5: Not Found ***/
#define HTTP_RSP_NALLOW         405    /*** RFC2616: Section 10.4.6: Method Not Allowed ***/
#define HTTP_RSP_NACCEPT        406    /*** RFC2616: Section 10.4.7: Not Acceptable ***/
#define HTTP_RSP_PROXY_AUTH     407    /*** RFC2616: Section 10.4.8: Proxy Authentication Required ***/
#define HTTP_RSP_TIMEOUT        408    /*** RFC2616: Section 10.4.9: Request Time-out ***/
#define HTTP_RSP_CONFLICT       409    /*** RFC2616: Section 10.4.10: Conflict ***/
#define HTTP_RSP_GONE           410    /*** RFC2616: Section 10.4.11: Gone ***/
#define HTTP_RSP_MISSLEN        411    /*** RFC2616: Section 10.4.12: Length Required ***/
#define HTTP_RSP_PREC_FAIL      412    /*** RFC2616: Section 10.4.13: Precondition Failed ***/
#define HTTP_RSP_TOO_LARGE      413    /*** RFC2616: Section 10.4.14: Request Entity Too Large ***/
#define HTTP_RSP_LONG_URL       414    /*** RFC2616: Section 10.4.15: Request-URI Too Large ***/
#define HTTP_RSP_UNSUP_MEDIA    415    /*** RFC2616: Section 10.4.16: Unsupported Media Type ***/
#define HTTP_RSP_BAD_RANGE      416    /*** RFC2616: Section 10.4.17: Requested range not satisfiable ***/
#define HTTP_RSP_EXPECT_FAIL    417    /*** RFC2616: Section 10.4.18: Expectation Failed ***/
#define HTTP_RSP_SERV           500    /*** RFC2616: Section 10.5.1: Internal Server Error ***/
#define HTTP_RSP_NOT_IMPL       501    /*** RFC2616: Section 10.5.2: Not Implemented ***/
#define HTTP_RSP_BAD_GW         502    /*** RFC2616: Section 10.5.3: Bad Gateway ***/
#define HTTP_RSP_SERV_UNAVAIL   503    /*** RFC2616: Section 10.5.4: Service Unavailable ***/
#define HTTP_RSP_GW_TIMEOUT     504    /*** RFC2616: Section 10.5.5: Gateway Time-out ***/
#define HTTP_RSP_VER_UNSUP      505    /*** RFC2616: Section 10.5.6: HTTP Version not supported ***/


/*
   RFC959 FTP response codes (Updated by RFC2228, RFC2640, RFC2773, RFC3659)
 */

#define FTP_RSP_ERR100          100    /* The requested action is being initiated, expect another reply before proceeding with a new command */
#define FTP_RSP_ERR110          110    /* Restart marker reply */
#define FTP_RSP_ERR120          120    /* Service ready in nnn minutes */
#define FTP_RSP_ERR125          125    /* Data connection already open, transfer starting */
#define FTP_RSP_ERR150          150    /* File status okay, about to open data connection */
#define FTP_RSP_ERR200          200    /* The requested action has been successfully completed */
#define FTP_RSP_ERR202          202    /* Command not implemented, superflous at this site */
#define FTP_RSP_ERR211          211    /* System status, or system help reply */
#define FTP_RSP_ERR212          212    /* Directory status */
#define FTP_RSP_ERR213          213    /* File status */
#define FTP_RSP_ERR214          214    /* Help message */
#define FTP_RSP_ERR215          215    /* NAME system type (Where NAME is an official system name from the list in the Assigned Numbers document) */
#define FTP_RSP_ERR220          220    /* Service ready for new user */
#define FTP_RSP_ERR221          221    /* Service closing control connection. Logged out if appropriate */
#define FTP_RSP_ERR225          225    /* Data connection open; no transfer in progress */
#define FTP_RSP_ERR226          226    /* Closing data connection. Requested file action successful (for example, file transfer or file abort) */
#define FTP_RSP_ERR227          227    /* Entering Passive Mode */
#define FTP_RSP_ERR230          230    /* User logged in, proceed */
#define FTP_RSP_ERR234          234    /* SFTP (Secure FTP) is not supported (SafeGossip server) */
#define FTP_RSP_ERR250          250    /* Requested file action okay, completed */
#define FTP_RSP_ERR257          257    /* 'PATHNAME' created */
#define FTP_RSP_ERR300          300    /* The command has been accepted, but the requested action is being held in abeyance, pending receipt of further information */
#define FTP_RSP_ERR331          331    /* User name okay, need password */
#define FTP_RSP_ERR332          332    /* Need account for login */
#define FTP_RSP_ERR334          334    /* SFTP (Secure FTP) is not supported */
#define FTP_RSP_ERR350          350    /* Requested file action pending further information */
#define FTP_RSP_ERR400          400    /* The command was not accepted and the requested action did not take place, but the error condition is temporary and the action may be requested again */
#define FTP_RSP_ERR421          421    /* Service not available, closing control connection. This may be a reply to any command if the service knows it must shut down */
#define FTP_RSP_ERR425          425    /* Can't open data connection */
#define FTP_RSP_ERR426          426    /* Connection closed; transfer aborted */
#define FTP_RSP_ERR450          450    /* Requested file action not taken. File unavailable (e.g., file busy) */
#define FTP_RSP_ERR451          451    /* Requested action aborted: local error in processing */
#define FTP_RSP_ERR452          452    /* Requested action not taken. Insufficient storage space in system */
#define FTP_RSP_ERR500          500    /* Syntax error, command unrecognized. This may include errors such as command line too long */
#define FTP_RSP_ERR501          501    /* Syntax error in parameters or arguments */
#define FTP_RSP_ERR502          502    /* Command not implemented */
#define FTP_RSP_ERR503          503    /* Bad sequence of commands */
#define FTP_RSP_ERR504          504    /* Command not implemented for that parameter */
#define FTP_RSP_ERR530          530    /* Not logged in */
#define FTP_RSP_ERR532          532    /* Need account for storing files */
#define FTP_RSP_ERR550          550    /* Requested action not taken. File unavailable (e.g., file not found, no access) */
#define FTP_RSP_ERR552          552    /* Requested file action aborted. Exceeded storage allocation (for current directory or dataset) */
#define FTP_RSP_ERR553          553    /* Requested action not taken. File name not allowed */


#endif
